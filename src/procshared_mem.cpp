/**
 * @file procshared_mem.cpp
 * @author PFA03027@nifty.com
 * @brief async open/close shared memory object
 * @version 0.1
 * @date 2023-09-03
 *
 * @copyright Copyright (c) 2023, PFA03027@nifty.com
 *
 */

#include <atomic>
#include <stdexcept>
#include <string>

#include <string.h>

#include <fcntl.h> /* For O_* constants */
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <limits.h> /* for NAME_MAX */

#include "procshared_mem.hpp"

#define TMP_DIR_FOR_ID_FILE "/tmp"

using type_of_errno = typename std::remove_reference<decltype( errno )>::type;

procshared_mem_defer_t procshared_mem_defer;

std::string make_strerror( type_of_errno e_v )
{
#if ( _POSIX_C_SOURCE >= 200112L ) && !_GNU_SOURCE
	// int strerror_r(int errnum, char *buf, size_t buflen);
	//             /* XSI-compliant */
	char str_buff[128];
	int  ret = strerror_r( e_v, str_buff, 128 );
	if ( ret == 0 ) {
		return std::string( str_buff );
	} else {
		return std::string( "errno=" ) + std::to_string( e_v ) + std::string( " (strerror_r() is fail)" );
	}
#else
	// char *strerror_r(int errnum, char *buf, size_t buflen);
	//             /* GNU-specific */
	char        str_buff[128];
	const char* p_ret = strerror_r( e_v, str_buff, 128 );
	if ( p_ret != NULL ) {
		return std::string( p_ret ) + std::string( "(errno=" ) + std::to_string( e_v ) + std::string( ")" );
	} else {
		return std::string( "errno=" ) + std::to_string( e_v ) + std::string( " (strerror_r() is fail)" );
	}
#endif
}

class procshared_mem_error : public std::runtime_error {
public:
	procshared_mem_error( type_of_errno e_v )
	  : std::runtime_error( make_strerror( e_v ) )
	{
	}
	procshared_mem_error( type_of_errno e_v, const std::string& additional_error_str )
	  : std::runtime_error( make_strerror( e_v ) + additional_error_str )
	{
	}
	procshared_mem_error( const char* p_error_str )
	  : std::runtime_error( p_error_str )
	{
	}
	procshared_mem_error( const std::string& error_str )
	  : std::runtime_error( error_str )
	{
	}
};

class procshared_mem_retry : public std::runtime_error {
public:
	procshared_mem_retry( void )
	  : std::runtime_error( "retry procshared_mem construction" )
	{
	}
};

class semaphore_post_guard_adopt_acquire_t {};
constexpr semaphore_post_guard_adopt_acquire_t semaphore_post_guard_adopt_acquire;
class semaphore_post_guard_defer_acquire_t {};
constexpr semaphore_post_guard_defer_acquire_t semaphore_post_guard_defer_acquire;
class semaphore_post_guard_try_to_acquire_t {};
constexpr semaphore_post_guard_try_to_acquire_t semaphore_post_guard_try_to_acquire;

/**
 * @brief owns_acquire() == trueの場合に、デストラクタでsemaphoreにpostを行うクラス
 *
 */
class semaphore_post_guard {
public:
	constexpr semaphore_post_guard( void )
	  : p_sem_( SEM_FAILED )
	  , owns_acquire_flag_( false )
	{
	}

	/**
	 * @brief sem_wait()することなく、owns_acquire() == true の状態で初期化するコンストラクタ
	 *
	 */
	constexpr semaphore_post_guard( sem_t* p_sem_arg, const semaphore_post_guard_adopt_acquire_t& )
	  : p_sem_( p_sem_arg )
	  , owns_acquire_flag_( true )
	{
	}

	/**
	 * @brief sem_wait()することなく、owns_acquire() == false の状態で初期化するコンストラクタ
	 *
	 */
	constexpr semaphore_post_guard( sem_t* p_sem_arg, const semaphore_post_guard_defer_acquire_t& )
	  : p_sem_( p_sem_arg )
	  , owns_acquire_flag_( false )
	{
	}

	/**
	 * @brief sem_trywait()を行って初期化するコンストラクタ
	 *
	 * sem_trywait()が成功した場合、owns_acquire() == true となる。そうでない場合は、owns_acquire() == false となる。
	 *
	 */
	semaphore_post_guard( sem_t* p_sem_arg, const semaphore_post_guard_try_to_acquire_t& )
	  : p_sem_( p_sem_arg )
	  , owns_acquire_flag_( false )
	{
		call_sem_trywait();
	}

	/**
	 * @brief sem_wait()を行い、owns_acquire() == true の状態で初期化するコンストラクタ
	 *
	 */
	semaphore_post_guard( sem_t* p_sem_arg )
	  : p_sem_( p_sem_arg )
	  , owns_acquire_flag_( false )
	{
		call_sem_wait();
	}

	semaphore_post_guard( semaphore_post_guard&& orig )
	  : p_sem_( orig.p_sem_ )
	  , owns_acquire_flag_( orig.owns_acquire_flag_ )
	{
		orig.p_sem_             = SEM_FAILED;
		orig.owns_acquire_flag_ = false;
	}

	semaphore_post_guard& operator=( semaphore_post_guard&& orig )
	{
		p_sem_             = orig.p_sem_;
		owns_acquire_flag_ = orig.owns_acquire_flag_;

		orig.p_sem_             = SEM_FAILED;
		orig.owns_acquire_flag_ = false;

		return *this;
	}

	~semaphore_post_guard()
	{
		call_sem_post();
	}

	bool owns_acquire( void ) const
	{
		return owns_acquire_flag_;
	}

	void acquire( void )
	{
		call_sem_wait();
	}

	bool try_acquire( void )
	{
		return call_sem_trywait();
	}

	void release( void )
	{
		call_sem_post();
	}

	void swap( semaphore_post_guard& u )
	{
		sem_t* p_sem_backup             = p_sem_;
		bool   owns_acquire_flag_backup = owns_acquire_flag_;

		p_sem_             = u.p_sem_;
		owns_acquire_flag_ = u.owns_acquire_flag_;

		u.p_sem_             = p_sem_backup;
		u.owns_acquire_flag_ = owns_acquire_flag_backup;
	}

private:
	// コピーは許可しない
	semaphore_post_guard( const semaphore_post_guard& )           = delete;
	semaphore_post_guard operator=( const semaphore_post_guard& ) = delete;

	void call_sem_wait( void )
	{
		if ( p_sem_ == SEM_FAILED ) {
			return;
		}

		int ret = sem_wait( p_sem_ );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			char buff[1024];
			snprintf( buff, 1024, " by sem_wait(%p)", p_sem_ );
			throw procshared_mem_error( cur_errno, buff );
		}

		owns_acquire_flag_ = true;
	}
	bool call_sem_trywait( void )
	{
		if ( p_sem_ == SEM_FAILED ) {
			return false;
		}

		int ret = sem_trywait( p_sem_ );
		if ( ret == 0 ) {
			owns_acquire_flag_ = true;
		} else {
			auto cur_errno = errno;
			if ( cur_errno != EAGAIN ) {
				char buff[1024];
				snprintf( buff, 1024, " by sem_trywait(%p)\n", p_sem_ );
				throw procshared_mem_error( cur_errno, buff );
			}

			printf( "sem_trywait(%p), but semaphore value is already 0(Zero)\n", p_sem_ );
			owns_acquire_flag_ = false;
		}
	}
	void call_sem_post( void )
	{
		if ( p_sem_ == SEM_FAILED ) {
			return;
		}
		if ( not owns_acquire_flag_ ) {
			return;
		}

		int ret = sem_post( p_sem_ );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			char buff[1024];
			snprintf( buff, 1024, " by sem_post(%p)", p_sem_ );
			std::string errlog = make_strerror( cur_errno ) + buff;
			fprintf( stderr, "%s\n", errlog.c_str() );
			// throw procshared_mem_error( cur_errno, buff );
		}
		owns_acquire_flag_ = false;
	}

	sem_t* p_sem_;
	bool   owns_acquire_flag_;
};

static_assert( std::is_integral<ino_t>::value, "ino_t is not integral..." );
static_assert( std::is_integral<off_t>::value, "off_t is not integral..." );

struct procshared_mem::procshared_mem_mem_header {
	std::atomic<ino_t> inode_val_;
	std::atomic<off_t> length_val_;
	unsigned char      shm_buff_[0];

	procshared_mem_mem_header( ino_t inode_v, off_t len_v )
	  : inode_val_( inode_v )   // constructor of atomic is not atomic operation
	  , length_val_( len_v )    // constructor of atomic is not atomic operation
	{
		inode_val_.store( inode_v );
		length_val_.store( len_v );
	}
};

procshared_mem::procshared_mem( const char* p_shm_name, off_t length, mode_t mode, const procshared_mem_defer_t& procshared_mem_defer_val )
  : pathname_( ( p_shm_name == nullptr ) ? "p_shm_name_is_nullptr" : p_shm_name )
  , id_fname_( get_id_filename( p_shm_name ) )
  , mode_( mode )
  , length_( calc_total_len( length ) )
  , is_owner_( false )
  , p_sem_( SEM_FAILED )
  , id_fd_( -1 )
  , shm_fd_( -1 )
  , p_mem_( nullptr )
  , initfunctor_( []( void*, off_t ) {} )
{
	check_path_name( p_shm_name );
}

procshared_mem::procshared_mem( const char* p_shm_name, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg )
  : pathname_( ( p_shm_name == nullptr ) ? "p_shm_name_is_nullptr" : p_shm_name )
  , id_fname_( get_id_filename( p_shm_name ) )
  , mode_( mode )
  , length_( calc_total_len( length ) )
  , is_owner_( false )
  , p_sem_( SEM_FAILED )
  , id_fd_( -1 )
  , shm_fd_( -1 )
  , p_mem_( nullptr )
  , initfunctor_( initfunctor_arg )
{
	check_path_name( p_shm_name );

	setup_as_both();
}

procshared_mem::procshared_mem( const char* p_shm_name, off_t length, mode_t mode )
  : procshared_mem( p_shm_name, length, mode, []( void*, off_t ) {} )
{
}

procshared_mem::~procshared_mem()
{
	teardown();
}

off_t procshared_mem::calc_total_len( off_t requested_length )
{
	off_t total_length = sizeof( procshared_mem_mem_header ) + requested_length;
	off_t page_len     = sysconf( _SC_PAGE_SIZE );
	off_t mx           = total_length / page_len;
	off_t rx           = total_length % page_len;

	return mx * page_len + ( ( rx == 0 ) ? 0 : page_len );
}

void procshared_mem::check_path_name( const char* p_shm_name )
{
	if ( p_shm_name == nullptr ) {
		throw procshared_mem_error( "p_shm_name is nullptr" );
	}
	if ( p_shm_name[0] != '/' ) {
		std::string error_str = "first charactor of p_shm_name is /. actual=";
		error_str += std::string( p_shm_name );
		throw procshared_mem_error( error_str );
	}
	size_t namelen = strlen( p_shm_name );
	if ( namelen >= ( NAME_MAX - 4 ) ) {
		std::string error_str = "p_shm_name is too long. it should be shorter than NAME_MAX-4. actual=";
		error_str += std::to_string( namelen );
		throw procshared_mem_error( error_str );
	}
}

std::string procshared_mem::get_id_filename( const char* p_path_name_arg )
{
	std::string ans( TMP_DIR_FOR_ID_FILE );
	return ans + std::string( p_path_name_arg );
}

ino_t procshared_mem::get_inode_of_fd( int id_f_fd )
{
	struct stat stat_buff;
	int         ret = fstat( id_f_fd, &stat_buff );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		char buff[1024];
		snprintf( buff, 1024, " by fstat(%d)", id_f_fd );
		throw procshared_mem_error( cur_errno, buff );
	}

	return stat_buff.st_ino;
}

void procshared_mem::setup_as_both( void )
{
	bool retry_loop = true;

	do {
		semaphore_post_guard spg;
		try {
			p_sem_ = sem_open( pathname_.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, mode_, 0 );
			if ( p_sem_ != SEM_FAILED ) {
				// デストラクタで、sem_postを行う。このsem_postは、ほかのプロセス向けにpostを行う。
				spg = semaphore_post_guard( p_sem_, semaphore_post_guard_adopt_acquire );

				// セマフォの作成に成功し、オーナー権を取得
				is_owner_ = true;

				setup_as_primary();
			} else {
				// セマフォの作成に失敗したので、この時点では、少なくともオーナー権を取得できていない
				is_owner_ = false;

				if ( errno != EEXIST ) {
					// 何らかの予期しない要因で、セマフォの作成に失敗したため、例外を投げる
					auto cur_errno = errno;
					char buff[1024];
					snprintf( buff, 1024, " by sem_open(%p, %x)", pathname_.c_str(), O_RDWR );
					throw procshared_mem_error( cur_errno, buff );
				}
				// すでにセマフォが存在ていたため、オーナー権を取得しないで、セマフォをオープンする。
				p_sem_ = sem_open( pathname_.c_str(), O_RDWR | O_CLOEXEC );
				if ( p_sem_ == SEM_FAILED ) {
					if ( errno == ENOENT ) {
						// ここに来た場合、コンストラクタの実行途中(最初のsem_openからここに到達するまでの間)に、
						// 共有メモリのエントリが削除された＝別プロセスでデストラクタが実行されたを意味する。
						// よって、最初からOpen手続きのやり直しが必要。
						throw procshared_mem_retry();
					}

					// 何らかの予期しない要因で、セマフォのオープンに失敗したため、例外を投げる
					auto cur_errno = errno;
					char buff[1024];
					snprintf( buff, 1024, " by sem_open(%s, %x)", pathname_.c_str(), O_RDWR );
					throw procshared_mem_error( cur_errno, buff );
				}

				// セマフォの取得をまつ。
				// デストラクタで、sem_postを行う。このsem_postは、ほかのプロセス向けにpostを行う。
				spg = semaphore_post_guard( p_sem_ );
				// セマフォの取得ができたことから、オーナー権を持つプロセスによる初期化が完了したことの確認ができた。

				setup_as_secondary();
			}

			retry_loop = false;
		} catch ( procshared_mem_retry& e ) {
			teardown_in_semaphore_lock( std::move( spg ) );
			// printf( "asyn_shm do internal retry open\n" );
		} catch ( ... ) {
			teardown_in_semaphore_lock( std::move( spg ) );
			throw;
		}
	} while ( retry_loop );
}

void procshared_mem::setup_as_primary( void )
{
	// インスタンスID代わりのinode番号を取得するために、ファイルを作成する。
	if ( unlink( id_fname_.c_str() ) != 0 ) {   // ごみファイルを消す
		type_of_errno cur_errno = errno;
		if ( cur_errno != ENOENT ) {
			auto err_str = make_strerror( cur_errno );
			printf( "Warning: fail to unlink(%s), errno(%d)=%s\n", id_fname_.c_str(), cur_errno, err_str.c_str() );
		}
	}
	id_fd_ = open( id_fname_.c_str(), O_CREAT | O_EXCL | O_CLOEXEC, mode_ );
	if ( id_fd_ < 0 ) {
		type_of_errno cur_errno = errno;
		auto          err_str   = make_strerror( cur_errno );
		printf( "Warning: fail to open(%s), errno(%d)=%s\n", id_fname_.c_str(), cur_errno, err_str.c_str() );
		char buff[1024];
		snprintf( buff, 1024, " by open(%s, O_CREAT | O_EXCL, %x)", pathname_.c_str(), mode_ );
		throw procshared_mem_error( cur_errno, buff );
	}
	ino_t inode_num_of_id_file = get_inode_of_fd( id_fd_ );

	// 共有メモリの作成
	int ret = shm_unlink( pathname_.c_str() );   // to avoid race condition
	if ( ret == 0 ) {
		printf( "Warning: race condition guard code works successfully for shm_unlink(%s)\n", pathname_.c_str() );
	}
	shm_fd_ = shm_open( pathname_.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, mode_ );
	if ( shm_fd_ < 0 ) {
		auto cur_errno = errno;
		char buff[1024];
		snprintf( buff, 1024, " by shm_open(%s, O_CREAT | O_EXCL | O_RDWR, %x)", pathname_.c_str(), mode_ );
		throw procshared_mem_error( cur_errno, buff );
	}
	ret = ftruncate( shm_fd_, length_ );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		char buff[1024];
		snprintf( buff, 1024, " by ftruncate(%d, %ld)", shm_fd_, length_ );
		throw procshared_mem_error( cur_errno, buff );
	}

	void* p_header = mmap( NULL, length_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0 );
	if ( p_header == MAP_FAILED ) {
		auto cur_errno = errno;
		char buff[1024];
		snprintf( buff, 1024, " by mmap(NULL, %ld, PROT_READ | PROT_WRITE, MAP_SHARED, %d, 0)", length_, shm_fd_ );
		throw procshared_mem_error( cur_errno, buff );
	}

	p_mem_ = new ( p_header ) procshared_mem_mem_header( inode_num_of_id_file, length_ );

	initfunctor_( p_mem_->shm_buff_, available_size() );

	initfunctor_ = []( void*, off_t ) {};   // 使い終わったので、事故を避けるために、初期化する。

	// printf( "success as owner\n" );
}

void procshared_mem::setup_as_secondary( void )
{

	// TODO:
	// 上の後に、プロセススイッチして、別のプロセス側で削除＆再作成された場合、shm_open()に成功したとしても、セマフォと以下の処理で行うshm_openの組み合わせが正しくなくなる。
	// 解決するためには、トランザクション的な処理を導入する必要があるが、今は割り切る。
	// 現時点では組み合わせ検証に、専用のファイルを作成し、ファイルのinode番号を検査することで組み合わせを検証する。
	// ファイルが存在しない等のNGが検出されたら、retryする必要があるので、procshared_mem_retryをthrowする。

	id_fd_ = open( id_fname_.c_str(), O_RDONLY | O_CLOEXEC, mode_ );
	if ( id_fd_ < 0 ) {
		type_of_errno cur_errno = errno;
		if ( cur_errno == ENOENT ) {
			throw procshared_mem_retry();
		}
		auto err_str = make_strerror( cur_errno );
		printf( "Warning: fail to open(%s), errno(%d)=%s\n", id_fname_.c_str(), cur_errno, err_str.c_str() );
		char buff[1024];
		snprintf( buff, 1024, " by open(%s, O_RDONLY | O_CLOEXEC, %x)", pathname_.c_str(), mode_ );
		throw procshared_mem_error( cur_errno, buff );
	}

	shm_fd_ = shm_open( pathname_.c_str(), O_RDWR | O_CLOEXEC, mode_ );
	if ( shm_fd_ < 0 ) {
		if ( errno == ENOENT ) {
			// ここに来た場合、コンストラクタの実行途中(sem_openからここに到達するまでの間)に、
			// 共有メモリのエントリが削除された＝別プロセスでデストラクタが実行されたを意味する。
			// よって、最初からOpen手続きのやり直しが必要。
			throw procshared_mem_retry();
		}
		auto cur_errno = errno;
		char buff[1024];
		snprintf( buff, 1024, " by shm_open(%s, O_RDWR | O_CLOEXEC, %x)", pathname_.c_str(), mode_ );
		throw procshared_mem_error( cur_errno, buff );
	}
	{
		// primary側で、ftrancateでサイズ設定が完了しているはずなのに、なぜかmmap()の段階でbusエラーになる。
		// そのため、lseekで事前にサイズ検証を行う。NGがあるようなら、リトライする。
		off_t epos      = lseek( shm_fd_, 0, SEEK_END );
		auto  cur_errno = errno;
		lseek( shm_fd_, 0, SEEK_SET );
		if ( epos <= -1 ) {
			char buff[1024];
			snprintf( buff, 1024, " by lseek(%s, 0, SEEK_END)", pathname_.c_str() );
			throw procshared_mem_error( cur_errno, buff );
		} else if ( static_cast<size_t>( epos ) < sizeof( procshared_mem_mem_header ) ) {
			// 共有メモリのサイズがうまく反映されていないので、リトライする。
			// printf( "Retry, because shared memory(%s) is too short. size by lseek()=%ld. expected size=%ld\n", pathname_.c_str(), epos, length_ );
			throw procshared_mem_retry();
		} else if ( epos < length_ ) {
			// 共有メモリのサイズが不足しているので、エラーとする
			char buff[1024];
			snprintf( buff, 1024, "shared memory(%s) is too short. size by lseek()=%ld. expected size=%ld", pathname_.c_str(), epos, length_ );
			throw procshared_mem_error( buff );
		}
	}

	void* p_header = mmap( NULL, length_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0 );
	if ( p_header == MAP_FAILED ) {
		auto cur_errno = errno;
		char buff[1024];
		snprintf( buff, 1024, " by mmap(NULL, %ld, PROT_READ | PROT_WRITE, MAP_SHARED, %d, 0)", length_, shm_fd_ );
		throw procshared_mem_error( cur_errno, buff );
	}

	p_mem_ = reinterpret_cast<procshared_mem_mem_header*>( p_header );
	if ( p_mem_->inode_val_.load( std::memory_order_acquire ) != get_inode_of_fd( id_fd_ ) ) {
		// ここに来た場合、コンストラクタの実行途中(sem_openからここに到達するまでの間)に、
		// 共有メモリのエントリが削除され、その後さらに作成された状況となる。
		// よって、最初からOpen手続きのやり直しが必要。
		throw procshared_mem_retry();
	}
	if ( p_mem_->length_val_.load() < length_ ) {
		// ここに来た場合、メモリサイズが少なすぎる。
		char buff[1024];
		snprintf( buff, 1024, "shared memory created by Primary is too small. Secondary expectation size=%ld, actual=%ld", length_, p_mem_->length_val_.load( std::memory_order_acquire ) );
		throw procshared_mem_error( buff );
	}

	// printf( "success\n" );
}

void procshared_mem::remove_resources( void )
{
	if ( shm_fd_ != -1 ) {
		// オーナー権を持ち、かつオーナーとして作成まで完了しているので、削除を行う
		int ret = shm_unlink( pathname_.c_str() );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "%s by shm_unlink(%s)\n", es.c_str(), pathname_.c_str() );
		}
	}
	if ( id_fd_ != -1 ) {
		// オーナー権を持ち、かつオーナーとして作成まで完了しているので、削除を行う
		int ret = unlink( id_fname_.c_str() );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "%s by unlink(%s)\n", es.c_str(), id_fname_.c_str() );
		}
	}
	if ( p_sem_ != SEM_FAILED ) {
		// オーナー権を持ち、かつオーナーとして作成まで完了しているので、削除を行う
		int ret = sem_unlink( pathname_.c_str() );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "%s by sem_unlink(%s)\n", es.c_str(), pathname_.c_str() );
		}
	}
}
void procshared_mem::close_resources( void )
{
	if ( p_mem_ != nullptr ) {
		int ret = munmap( p_mem_, length_ );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "%s by munmap(%p)\n", es.c_str(), p_sem_ );
		}
		p_mem_ = nullptr;
	}
	if ( shm_fd_ != -1 ) {
		int ret = close( shm_fd_ );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "%s by close(%d)\n", es.c_str(), shm_fd_ );
		}
		shm_fd_ = -1;
	}
	if ( id_fd_ != -1 ) {
		int ret = close( id_fd_ );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "%s by close(%d) of id file %s\n", es.c_str(), id_fd_, id_fname_.c_str() );
		}
		id_fd_ = -1;
	}
	if ( p_sem_ != SEM_FAILED ) {
		int ret = sem_close( p_sem_ );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "%s by sem_close(%p)\n", es.c_str(), p_sem_ );
		}
		p_sem_ = SEM_FAILED;
	}
}

void procshared_mem::teardown_in_semaphore_lock( semaphore_post_guard&& semguard_arg )
{
	{
		semaphore_post_guard in_sem_guard( std::move( semguard_arg ) );
		if ( is_owner_ ) {
			remove_resources();
			is_owner_ = false;
		}
	}

	close_resources();
}

void procshared_mem::teardown( void )
{
	if ( is_owner_ ) {
		semaphore_post_guard sem_guard( p_sem_ );
		// semaphore_post_guard sem_guard( p_sem_, semaphore_post_guard_try_to_acquire );

		remove_resources();

		is_owner_ = false;
	}

	close_resources();
}

off_t procshared_mem::available_size( void ) const
{
	return length_ - sizeof( procshared_mem_mem_header );
}

void* procshared_mem::get( void )
{
	procshared_mem_mem_header* p_header = reinterpret_cast<procshared_mem_mem_header*>( p_mem_ );
	return reinterpret_cast<void*>( p_header->shm_buff_ );
}

void procshared_mem::allocate_shm_as_both( std::function<void( void*, off_t )> initfunctor_arg )
{
	initfunctor_ = initfunctor_arg;
	setup_as_both();
}

void procshared_mem::allocate_shm_as_primary( std::function<void( void*, off_t )> initfunctor_arg )
{
	if ( p_sem_ != SEM_FAILED ) {
		printf( "already allocated as %s\n", is_owner_ ? "primary" : "secondary" );
		return;
	}

	initfunctor_    = initfunctor_arg;
	bool retry_loop = true;

	do {
		semaphore_post_guard spg;
		try {
			p_sem_ = sem_open( pathname_.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, mode_, 0 );
			if ( p_sem_ == SEM_FAILED ) {
				auto cur_errno = errno;
				char buff[1024];
				snprintf( buff, 1024, " by sem_open(%s, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, %x, 0) as primary", pathname_.c_str(), mode_ );
				throw procshared_mem_error( cur_errno, buff );
			}
			// デストラクタで、sem_postを行う。このsem_postは、ほかのプロセス向けにpostを行う。
			spg = semaphore_post_guard( p_sem_, semaphore_post_guard_adopt_acquire );

			// セマフォの作成に成功し、オーナー権を取得
			is_owner_ = true;
			setup_as_primary();

			retry_loop = false;
		} catch ( procshared_mem_retry& e ) {
			teardown_in_semaphore_lock( std::move( spg ) );
			// printf( "asyn_shm do internal retry open\n" );
		} catch ( ... ) {
			teardown_in_semaphore_lock( std::move( spg ) );
			throw;
		}
	} while ( retry_loop );
}

void procshared_mem::allocate_shm_as_secondary( void )
{
	if ( p_sem_ != SEM_FAILED ) {
		printf( "already allocated as %s\n", is_owner_ ? "primary" : "secondary" );
		return;
	}

	bool retry_loop = true;

	do {
		semaphore_post_guard spg;
		try {

			p_sem_ = sem_open( pathname_.c_str(), O_RDWR | O_CLOEXEC );
			if ( p_sem_ == SEM_FAILED ) {
				auto cur_errno = errno;
				char buff[1024];
				snprintf( buff, 1024, " by sem_open(%s, O_RDWR | O_CLOEXEC) as secondary", pathname_.c_str() );
				throw procshared_mem_error( cur_errno, buff );
			}
			// セマフォの取得をまつ。
			// デストラクタで、sem_postを行う。このsem_postは、ほかのプロセス向けにpostを行う。
			spg = semaphore_post_guard( p_sem_ );
			// セマフォの取得ができたことから、オーナー権を持つプロセスによる初期化が完了したことの確認ができた。

			// secondaryとして初期化するので、オーナー権を更新＆確定する。
			is_owner_ = false;
			setup_as_secondary();

			retry_loop = false;
		} catch ( procshared_mem_retry& e ) {
			teardown_in_semaphore_lock( std::move( spg ) );
			// printf( "asyn_shm do internal retry open\n" );
		} catch ( ... ) {
			teardown_in_semaphore_lock( std::move( spg ) );
			throw;
		}
	} while ( retry_loop );
}

ino_t procshared_mem::debug_get_id_file_inode( void ) const
{
	return reinterpret_cast<procshared_mem_mem_header*>( p_mem_ )->inode_val_.load( std::memory_order_acquire );
}

bool procshared_mem::debug_test_integrity( void ) const
{
	bool ans = false;
	try {
		ans = ( debug_get_id_file_inode() == get_inode_of_fd( id_fd_ ) ) &&
		      ( length_ <= reinterpret_cast<procshared_mem_mem_header*>( p_mem_ )->length_val_.load( std::memory_order_acquire ) );

	} catch ( procshared_mem_error& e ) {
		ans = false;
	}

	return ans;
}

void procshared_mem::debug_set_as_owner( void )
{
	is_owner_ = true;
}

void procshared_mem::debug_force_cleanup( const char* p_shm_name )
{
	int ret = shm_unlink( p_shm_name );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		auto es        = make_strerror( cur_errno );
		printf( "%s by shm_unlink(%s)\n", es.c_str(), p_shm_name );
	}
	ret = sem_unlink( p_shm_name );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		auto es        = make_strerror( cur_errno );
		printf( "%s by sem_unlink(%s)\n", es.c_str(), p_shm_name );
	}
}