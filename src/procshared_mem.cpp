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
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <string.h>

#include <fcntl.h>  /* For O_* constants */
#include <limits.h> /* for NAME_MAX */
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

#include "procshared_mem.hpp"

#define TMP_DIR_FOR_ID_FILE "/tmp"

static_assert( std::is_integral<ino_t>::value, "ino_t is not integral..." );
static_assert( std::is_integral<off_t>::value, "off_t is not integral..." );

const char* p_id_file_base_dir = TMP_DIR_FOR_ID_FILE;

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

ino_t get_inode_of_fd( int id_f_fd )
{
	struct stat stat_buff;
	int         ret = fstat( id_f_fd, &stat_buff );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		char buff[1024];
		snprintf( buff, 1024, "Error: Fail to fstat(%d)", id_f_fd );
		throw procshared_mem_error( cur_errno, buff );
	}

	return stat_buff.st_ino;
}

////////////////////////////////////////////////////////////////////////////////////////////////
procshared_mem_error::procshared_mem_error( type_of_errno e_v )
  : std::runtime_error( make_strerror( e_v ) )
{
}
procshared_mem_error::procshared_mem_error( type_of_errno e_v, const std::string& additional_error_str )
  : std::runtime_error( make_strerror( e_v ) + additional_error_str )
{
}
procshared_mem_error::procshared_mem_error( const char* p_error_str )
  : std::runtime_error( p_error_str )
{
}
procshared_mem_error::procshared_mem_error( const std::string& error_str )
  : std::runtime_error( error_str )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////
class procshared_mem_retry : public std::runtime_error {
public:
	procshared_mem_retry( void )
	  : std::runtime_error( "retry procshared_mem_impl construction" )
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////
class id_file_resource_handler {
public:
	id_file_resource_handler( void )
	  : fname_()
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
	}

	~id_file_resource_handler()
	{
		if ( is_valid() ) {
			if ( close( fd_ ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				fprintf( stderr, "Warning: fail to close(%d) of id file %s, %s\n", fd_, fname_.c_str(), es.c_str() );
			}
		}

		fname_.clear();
		fd_       = -1;
		inode_id_ = 0;
	}

	id_file_resource_handler( id_file_resource_handler&& src )
	  : fname_( std::move( src.fname_ ) )
	  , fd_( src.fd_ )
	  , inode_id_( src.inode_id_ )
	{
		src.fd_   = -1;
		inode_id_ = 0;
	}

	id_file_resource_handler& operator=( id_file_resource_handler&& src )
	{
		if ( this == &src ) return *this;

		id_file_resource_handler( std::move( src ) ).swap( *this );

		return *this;
	}

	id_file_resource_handler( const std::string& fname_str, mode_t mode_arg )
	  : fname_( fname_str )
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
		if ( fname_.empty() ) {
			// same as default constructor
			return;
		}

		try_create_or_open( mode_arg );
	}

	id_file_resource_handler( std::string&& fname_str, mode_t mode_arg )
	  : fname_( std::move( fname_str ) )
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
		if ( fname_.empty() ) {
			// same as default constructor
			return;
		}

		try_create_or_open( mode_arg );
	}

	explicit id_file_resource_handler( const std::string& fname_str )
	  : fname_( fname_str )
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
		if ( fname_.empty() ) {
			// same as default constructor
			return;
		}

		try_open();
	}

	explicit id_file_resource_handler( std::string&& fname_str )
	  : fname_( std::move( fname_str ) )
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
		if ( fname_.empty() ) {
			// same as default constructor
			return;
		}

		try_open();
	}

	void do_unlink( void ) noexcept
	{
		if ( !fname_.empty() ) {
			if ( unlink( fname_.c_str() ) != 0 ) {
				auto cur_errno = errno;
				if ( cur_errno != ENOENT ) {
					fprintf( stderr, "Warning: try to unlink(%s) for id file, but it has not existed already.\n", fname_.c_str() );
				} else {
					auto es = make_strerror( cur_errno );
					fprintf( stderr, "Warning: fail to unlink(%s), %s\n", fname_.c_str(), es.c_str() );
				}
			}
		} else {
			fprintf( stderr, "Warning: id file is requested to unlink(), but fname_ is empty\n" );
		}
	}

	ino_t get_inode_number( void ) const noexcept
	{
		return inode_id_;
	}

	bool is_valid( void ) const noexcept
	{
		return ( fd_ >= 0 );
	}

	const std::string& name( void ) const noexcept
	{
		return fname_;
	}

	void swap( id_file_resource_handler& b )
	{
		fname_.swap( b.fname_ );
		std::swap( fd_, b.fd_ );
		std::swap( inode_id_, b.inode_id_ );
	}

	void release_resource( void )
	{
		id_file_resource_handler().swap( *this );
	}

private:
	id_file_resource_handler( const id_file_resource_handler& )            = delete;
	id_file_resource_handler& operator=( const id_file_resource_handler& ) = delete;

	void try_create_or_open( mode_t mode_arg )
	{
		fd_ = open( fname_.c_str(), O_CREAT | O_CLOEXEC, mode_arg );
		if ( fd_ < 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "Debug: fail to open(%s, O_CREAT | O_CLOEXEC, %x): %s\n", fname_.c_str(), mode_arg, es.c_str() );
			return;
		}
		try {
			inode_id_ = get_inode_of_fd( fd_ );
		} catch ( ... ) {
			// ファイルの作り主なので、ファイルをunlinkする。
			if ( unlink( fname_.c_str() ) != 0 ) {
				type_of_errno cur_errno = errno;
				if ( cur_errno != ENOENT ) {
					auto err_str = make_strerror( cur_errno );
					fprintf( stderr, "Warning: fail to unlink(%s): %s\n", fname_.c_str(), err_str.c_str() );
				}
			}
			if ( close( fd_ ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				fprintf( stderr, "Warning: fail to close(%d) of id file %s, %s\n", fd_, fname_.c_str(), es.c_str() );
			}
			fd_ = -1;

			throw;   // inode番号が取れないのは致命傷なので、握りつぶさずに例外を再送する。
		}
	}

	void try_open( void )
	{
		fd_ = open( fname_.c_str(), O_CLOEXEC );
		if ( fd_ < 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			printf( "Debug: fail to open(%s, O_CLOEXEC): %s\n", fname_.c_str(), es.c_str() );
			return;
		}
		try {
			inode_id_ = get_inode_of_fd( fd_ );
		} catch ( ... ) {
			if ( close( fd_ ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				fprintf( stderr, "Warning: fail to close(%d) of id file %s, %s\n", fd_, fname_.c_str(), es.c_str() );
			}
			fd_ = -1;

			throw;   // inode番号が取れないのは致命傷なので、握りつぶさずに例外を再送する。
		}
	}

	std::string fname_;      //!< 監視対象のIDファイルへのファイル名
	int         fd_;         //!< 監視対象のIDファイルへのfd
	ino_t       inode_id_;   //!< 監視対象のIDファイルのinode番号
};

////////////////////////////////////////////////////////////////////////////////////////////////
class semaphore_resource_handler {
public:
	using native_handle_type = sem_t*;

	semaphore_resource_handler( void ) noexcept
	  : sem_name_()
	  , p_sem_( SEM_FAILED )
	{
	}

	~semaphore_resource_handler()
	{
		if ( is_valid() ) {
			if ( sem_close( p_sem_ ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				fprintf( stderr, "Error: Fail to sem_close(%p), %s\n", p_sem_, es.c_str() );
			}
		}

		sem_name_.clear();
		p_sem_ = SEM_FAILED;
	}

	semaphore_resource_handler( semaphore_resource_handler&& src )
	  : sem_name_( std::move( src.sem_name_ ) )
	  , p_sem_( src.p_sem_ )
	{
		src.p_sem_ = SEM_FAILED;
	}

	semaphore_resource_handler& operator=( semaphore_resource_handler&& src )
	{
		if ( this == &src ) return *this;

		semaphore_resource_handler( std::move( src ) ).swap( *this );

		return *this;
	}

	// this constructor try to create and open with mode_arg
	semaphore_resource_handler( const std::string& sem_name, mode_t mode_arg )
	  : sem_name_( sem_name )
	  , p_sem_( SEM_FAILED )
	{
		try_create( mode_arg );
	}

	// this constructor try to create and open with mode_arg
	semaphore_resource_handler( std::string&& sem_name, mode_t mode_arg )
	  : sem_name_( std::move( sem_name ) )
	  , p_sem_( SEM_FAILED )
	{
		try_create( mode_arg );
	}

	// this constructor try to open
	explicit semaphore_resource_handler( const std::string& sem_name )
	  : sem_name_( sem_name )
	  , p_sem_( SEM_FAILED )
	{
		try_open();
	}

	// this constructor try to open
	explicit semaphore_resource_handler( std::string&& sem_name )
	  : sem_name_( std::move( sem_name ) )
	  , p_sem_( SEM_FAILED )
	{
		try_open();
	}

	constexpr native_handle_type native_handle( void ) const
	{
		return p_sem_;
	}

	void do_unlink( void ) noexcept
	{
		if ( !sem_name_.empty() ) {
			if ( sem_unlink( sem_name_.c_str() ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				fprintf( stderr, "Error: Fail to sem_unlink(%s), %s\n", sem_name_.c_str(), es.c_str() );
			}
		} else {
			fprintf( stderr, "Warning: semaphore is requested to sem_unlink(), but sem_name_ is empty\n" );
		}
	}

	bool is_valid( void ) const noexcept
	{
		return ( p_sem_ != SEM_FAILED );
	}

	const std::string& name( void ) const noexcept
	{
		return sem_name_;
	}

	void swap( semaphore_resource_handler& b )
	{
		sem_name_.swap( b.sem_name_ );
		std::swap( p_sem_, b.p_sem_ );
	}

	void release_resource( void )
	{
		semaphore_resource_handler().swap( *this );
	}

private:
	semaphore_resource_handler( const semaphore_resource_handler& )            = delete;
	semaphore_resource_handler& operator=( const semaphore_resource_handler& ) = delete;

	void try_create( mode_t mode_arg )
	{
		p_sem_ = sem_open( sem_name_.c_str(), O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, mode_arg, 0 );
		if ( p_sem_ == SEM_FAILED ) {
			if ( errno != EEXIST ) {
				// 何らかの予期しない要因で、セマフォの作成に失敗したため、例外を投げる
				auto cur_errno = errno;
				char buff[1024];
				snprintf( buff, 1024, "Error: Fail sem_open(%s, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, %x, 0): ", sem_name_.c_str(), mode_arg );
				throw procshared_mem_error( cur_errno, buff );
			}
		}
	}

	void try_open( void )
	{
		p_sem_ = sem_open( sem_name_.c_str(), O_RDWR | O_CLOEXEC );
		if ( p_sem_ == SEM_FAILED ) {
			if ( errno != ENOENT ) {
				// 何らかの予期しない要因で、セマフォのオープンに失敗したため、例外を投げる
				auto cur_errno = errno;
				char buff[1024];
				snprintf( buff, 1024, "Error: Fail sem_open(%s, O_RDWR | O_CLOEXEC)", sem_name_.c_str() );
				throw procshared_mem_error( cur_errno, buff );
			}
		}
	}

	std::string        sem_name_;
	native_handle_type p_sem_;
};

////////////////////////////////////////////////////////////////////////////////////////////////
class shm_resource_handler {
public:
	struct try_create_tag {};
	struct try_open_tag {};

	shm_resource_handler( void )
	  : shm_name_()
	  , shm_fd_( -1 )
	  , length_( 0 )
	  , p_mem_( nullptr )
	{
	}

	~shm_resource_handler()
	{
		if ( is_valid() ) {
			if ( p_mem_ != nullptr ) {
				int ret = munmap( p_mem_, length_ );
				if ( ret != 0 ) {
					auto cur_errno = errno;
					auto es        = make_strerror( cur_errno );
					fprintf( stderr, "%s by munmap(%p)\n", es.c_str(), p_mem_ );
				}
			}
			if ( shm_fd_ != -1 ) {
				int ret = close( shm_fd_ );
				if ( ret != 0 ) {
					auto cur_errno = errno;
					auto es        = make_strerror( cur_errno );
					fprintf( stderr, "%s by close(%d)\n", es.c_str(), shm_fd_ );
				}
			}
		}

		shm_name_.clear();
		shm_fd_ = -1;
		length_ = 0;
		p_mem_  = nullptr;
	}

	shm_resource_handler( shm_resource_handler&& src )
	  : shm_name_( std::move( src.shm_name_ ) )
	  , shm_fd_( src.shm_fd_ )
	  , length_( src.length_ )
	  , p_mem_( src.p_mem_ )
	{
		src.shm_fd_ = -1;
		src.length_ = 0;
		src.p_mem_  = nullptr;
	}

	shm_resource_handler& operator=( shm_resource_handler&& src )
	{
		if ( this == &src ) return *this;
		shm_resource_handler( std::move( src ) ).swap( *this );
		return *this;
	}

	shm_resource_handler( const try_create_tag, const std::string& shm_name_arg, size_t length_arg, mode_t mode_arg )
	  : shm_name_( shm_name_arg )
	  , shm_fd_( -1 )
	  , length_( 0 )
	  , p_mem_( nullptr )
	{
		try_create( length_arg, mode_arg );
	}

	shm_resource_handler( const try_open_tag, const std::string& shm_name_arg, size_t length_arg, mode_t mode_arg )
	  : shm_name_( shm_name_arg )
	  , shm_fd_( -1 )
	  , length_( 0 )
	  , p_mem_( nullptr )
	{
		try_open( length_arg, mode_arg );
	}

	void do_unlink( void ) noexcept
	{
		if ( !shm_name_.empty() ) {
			if ( shm_unlink( shm_name_.c_str() ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				fprintf( stderr, "Error: Fail shm_unlink(%s), %s", shm_name_.c_str(), es.c_str() );
			}
		}
	}

	bool is_valid( void ) const noexcept
	{
		return ( shm_fd_ >= 0 ) && ( p_mem_ != nullptr );
	}

	const std::string& name( void ) const noexcept
	{
		return shm_name_;
	}

	void* get_shm_pointer( void ) const noexcept
	{
		return p_mem_;
	}

	size_t allocated_size( void ) const noexcept
	{
		return length_;
	}

	void swap( shm_resource_handler& b )
	{
		shm_name_.swap( b.shm_name_ );
		std::swap( shm_fd_, b.shm_fd_ );
		std::swap( length_, b.length_ );
		std::swap( p_mem_, b.p_mem_ );
	}

	void release_resource( void )
	{
		shm_resource_handler().swap( *this );
	}

private:
	shm_resource_handler( const shm_resource_handler& )            = delete;
	shm_resource_handler& operator=( const shm_resource_handler& ) = delete;

	void try_common_create_or_open( int oflags_arg, size_t length_arg, mode_t mode_arg )
	{
		// 共有メモリの作成
		shm_fd_ = shm_open( shm_name_.c_str(), oflags_arg, mode_arg );
		if ( shm_fd_ < 0 ) {
			auto cur_errno = errno;
			char buff[1024];
			auto es2 = make_strerror( cur_errno );
			snprintf( buff, 1024, "Fail shm_open(%s, %x, %x), %s", shm_name_.c_str(), oflags_arg, mode_arg, es2.c_str() );
			return;
		}
		if ( ftruncate( shm_fd_, length_arg ) != 0 ) {
			auto cur_errno = errno;
			char buff[1024];
			auto es = make_strerror( cur_errno );
			snprintf( buff, 1024, "Fail ftruncate(%d, %ld), %s", shm_fd_, length_arg, es.c_str() );
			std::string err_log = buff;

			if ( shm_unlink( shm_name_.c_str() ) != 0 ) {
				auto cur_errno = errno;
				char buff[1024];
				auto es2 = make_strerror( cur_errno );
				snprintf( buff, 1024, ", then fail shm_unlink(%s), %s", shm_name_.c_str(), es2.c_str() );
				err_log += buff;
			}
			if ( close( shm_fd_ ) != 0 ) {
				auto cur_errno2 = errno;
				char buff[1024];
				auto es2 = make_strerror( cur_errno2 );
				snprintf( buff, 1024, ", then fail close(%d), %s\n", shm_fd_, es2.c_str() );
				err_log += buff;
			}

			shm_fd_ = -1;
			throw procshared_mem_error( err_log.c_str() );
		}
		length_ = length_arg;

		void* p_mem_ = mmap( NULL, length_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0 );
		if ( p_mem_ == MAP_FAILED ) {
			auto cur_errno = errno;
			char buff[1024];
			auto es2 = make_strerror( cur_errno );
			snprintf( buff, 1024, "Fail mmap(NULL, %ld, PROT_READ | PROT_WRITE, MAP_SHARED, %d, 0), %s", length_, shm_fd_, es2.c_str() );
			std::string err_log = buff;

			if ( shm_unlink( shm_name_.c_str() ) != 0 ) {
				auto cur_errno = errno;
				char buff[1024];
				auto es2 = make_strerror( cur_errno );
				snprintf( buff, 1024, ", then fail shm_unlink(%s), %s", shm_name_.c_str(), es2.c_str() );
				err_log += buff;
			}
			if ( close( shm_fd_ ) != 0 ) {
				auto cur_errno2 = errno;
				char buff[1024];
				auto es2 = make_strerror( cur_errno2 );
				snprintf( buff, 1024, ", then fail close(%d), %s\n", shm_fd_, es2.c_str() );
				err_log += buff;
			}

			shm_fd_ = -1;
			length_ = 0;
			p_mem_  = nullptr;
			throw procshared_mem_error( err_log.c_str() );
		}

		return;
	}

	void try_create( size_t length_arg, mode_t mode_arg )
	{
		if ( shm_unlink( shm_name_.c_str() ) == 0 ) {
			fprintf( stderr, "Warning: race condition guard code works successfully for shm_unlink(%s)\n", shm_name_.c_str() );
		}

		try_common_create_or_open( O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, length_arg, mode_arg );
	}

	void try_open( size_t length_arg, mode_t mode_arg )
	{
		try_common_create_or_open( O_RDWR | O_CLOEXEC, length_arg, mode_arg );
	}

	std::string shm_name_;   //!< 共有メモリの名称
	int         shm_fd_;     //!< 共有メモリのfd
	size_t      length_;     //!< 共有メモリのサイズ
	void*       p_mem_;      //!< 共有メモリへのポインタ
};

////////////////////////////////////////////////////////////////////////////////////////////////
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
	constexpr semaphore_post_guard( void ) noexcept
	  : p_sem_( SEM_FAILED )
	  , owns_acquire_flag_( false )
	{
	}

	~semaphore_post_guard()
	{
		if ( p_sem_ == SEM_FAILED ) {
			return;
		}

		call_sem_post();
	}

	constexpr semaphore_post_guard( semaphore_post_guard&& orig ) noexcept
	  : p_sem_( orig.p_sem_ )
	  , owns_acquire_flag_( orig.owns_acquire_flag_ )
	{
		orig.p_sem_             = SEM_FAILED;
		orig.owns_acquire_flag_ = false;
	}

	constexpr semaphore_post_guard& operator=( semaphore_post_guard&& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		semaphore_post_guard( std::move( orig ) ).swap( *this );

		return *this;
	}

	/**
	 * @brief sem_wait()することなく、owns_acquire() == true の状態で初期化するコンストラクタ
	 *
	 */
	constexpr semaphore_post_guard( semaphore_resource_handler& sem_arg, const semaphore_post_guard_adopt_acquire_t& ) noexcept
	  : p_sem_( sem_arg.native_handle() )
	  , owns_acquire_flag_( true )
	{
	}

	/**
	 * @brief sem_wait()することなく、owns_acquire() == false の状態で初期化するコンストラクタ
	 *
	 */
	constexpr semaphore_post_guard( semaphore_resource_handler& sem_arg, const semaphore_post_guard_defer_acquire_t& ) noexcept
	  : p_sem_( sem_arg.native_handle() )
	  , owns_acquire_flag_( false )
	{
	}

	/**
	 * @brief sem_trywait()を行って初期化するコンストラクタ
	 *
	 * sem_trywait()が成功した場合、owns_acquire() == true となる。そうでない場合は、owns_acquire() == false となる。
	 *
	 */
	semaphore_post_guard( semaphore_resource_handler& sem_arg, const semaphore_post_guard_try_to_acquire_t& )
	  : p_sem_( sem_arg.native_handle() )
	  , owns_acquire_flag_( false )
	{
		call_sem_trywait();
	}

	/**
	 * @brief sem_wait()を行い、owns_acquire() == true の状態で初期化するコンストラクタ
	 *
	 */
	explicit semaphore_post_guard( const semaphore_resource_handler& sem_arg )
	  : p_sem_( sem_arg.native_handle() )
	  , owns_acquire_flag_( false )
	{
		call_sem_wait();
	}

	bool owns_acquire( void ) const noexcept
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
		if ( p_sem_ == SEM_FAILED ) {
			fprintf( stderr, "unexpected calling call_sem_post() by this=%p\n", this );
			return;
		}

		call_sem_post();
	}

	constexpr void swap( semaphore_post_guard& u ) noexcept
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
			char buff[1024];
			snprintf( buff, 1024, "unexpected calling call_sem_wait() by this=%p", this );
			throw procshared_mem_error( buff );
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
			char buff[1024];
			snprintf( buff, 1024, "unexpected calling call_sem_wait() by this=%p", this );
			throw procshared_mem_error( buff );
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

			fprintf( stderr, "sem_trywait(%p), but semaphore value is already 0(Zero)\n", p_sem_ );
			owns_acquire_flag_ = false;
		}
	}
	void call_sem_post( void ) noexcept
	{
		if ( not owns_acquire_flag_ ) {
			return;
		}

		int ret = sem_post( p_sem_ );
		if ( ret != 0 ) {
			auto cur_errno = errno;
			try {
				std::string errlog = make_strerror( cur_errno );
				fprintf( stderr, "Fail sem_post(%p): %s\n", p_sem_, errlog.c_str() );
			} catch ( ... ) {
				fprintf( stderr, "Fail sem_post(%p): errno=%d\n", p_sem_, cur_errno );
			}
		}
		owns_acquire_flag_ = false;
	}

	sem_t* p_sem_;
	bool   owns_acquire_flag_;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Async initialized shared memory
 *
 * @note
 * If you use this class for testing in one process, please intent an instance as primary explicitly.
 * sem_open return a same object of semaphore, even if sem_open is called by a different instance.
 * this will lead unexpected creation/destruction rece condition.
 */
class procshared_mem::impl {
public:
	struct construct_as_primary_tag {};
	struct construct_as_secondary_tag {};

	/**
	 * @brief Construct a new procshared mem object that is empty
	 *
	 */
	impl( void );

	~impl();

	/**
	 * @brief Construct and allocate a new cooperative startup shared memory object
	 *
	 * This constructor allocates a shared memory during constructor.
	 * If an instance got as a primary role, it calls a functor initfunctor_arg() after finish setup of a shared memory
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 * @param initfunctor_arg a functor to initialize a shared memory area that length is same to an argument 'length'
	 *
	 * @exception if failed creation by any reason, throw std::bad_alloc(in case of new operator throws) or std::run_time_error
	 *
	 * @note p_shm_name string AAA must follow POSIX semaphore name specifications. please refer sem_open or sem_overview
	 */
	impl( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg );

	/**
	 * @brief allocate a new cooperative startup shared memory object as primary
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 * @param initfunctor_arg a functor to initialize a shared memory area that length is same to an argument 'length'
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception procshared_mem_error
	 */
	impl( const construct_as_primary_tag, const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg );

	/**
	 * @brief allocate a new cooperative startup shared memory object as secondary
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception procshared_mem_error
	 */
	impl( const construct_as_secondary_tag, const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode );

	void* get( void );
	off_t available_size( void ) const;

	ino_t       debug_get_id_file_inode( void ) const;
	bool        debug_test_integrity( void ) const;
	std::string debug_dump_string( void ) const;

private:
	struct procshared_mem_mem_header;

	void check_path_name( const char* p_shm_name );
	void setup_as_both( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg, std::function<void( void*, off_t )> initfunctor_arg );
	void setup_as_primary( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg, std::function<void( void*, off_t )> initfunctor_arg );
	void setup_as_secondary( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg );
	bool try_setup_as_both( int role_type, const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg, std::function<void( void*, off_t )> initfunctor_arg );

	static std::string get_id_filename( const char* p_path_name_arg, const char* p_id_dirname_arg );
	static off_t       calc_total_neccesary_len( off_t requested_length );
	static off_t       calc_available_size( off_t allocated_shm_length );

	std::string                sem_name_;
	id_file_resource_handler   id_res_;
	shm_resource_handler       shm_res_;
	procshared_mem_mem_header* p_mem_;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct procshared_mem::impl::procshared_mem_mem_header {
	std::atomic<off_t> length_val_;
	std::atomic<int>   reference_count_;
	std::atomic<ino_t> inode_val_;
	unsigned char      shm_buff_[0];

	procshared_mem_mem_header( ino_t inode_v, off_t len_v )
	  : length_val_( len_v )    // constructor of atomic is not atomic operation
	  , reference_count_( 0 )   // constructor of atomic is not atomic operation
	  , inode_val_( inode_v )   // constructor of atomic is not atomic operation
	{
		length_val_.store( len_v );
		reference_count_.store( 0 );
		inode_val_.store( inode_v );
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
off_t procshared_mem::impl::calc_total_neccesary_len( off_t requested_length )
{
	off_t total_length = sizeof( procshared_mem_mem_header ) + requested_length;
	off_t page_len     = sysconf( _SC_PAGE_SIZE );
	off_t mx           = total_length / page_len;
	off_t rx           = total_length % page_len;

	return mx * page_len + ( ( rx == 0 ) ? 0 : page_len );
}

void procshared_mem::impl::check_path_name( const char* p_shm_name )
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

std::string procshared_mem::impl::get_id_filename( const char* p_shm_name_arg, const char* p_id_dirname_arg )
{
	if ( p_shm_name_arg == nullptr ) {
		throw procshared_mem_error( "p_shm_name_arg is nullptr" );
	}
	std::string ans( ( p_id_dirname_arg != nullptr ) ? p_id_dirname_arg : TMP_DIR_FOR_ID_FILE );
	return ans + std::string( p_shm_name_arg );
}

off_t procshared_mem::impl::calc_available_size( off_t allocated_shm_length )
{
	return allocated_shm_length - sizeof( procshared_mem_mem_header );
}

procshared_mem::impl::impl( void )
  : sem_name_()
  , id_res_()
  , shm_res_()
  , p_mem_( nullptr )
{
}

procshared_mem::impl::~impl()
{
	try {
		semaphore_resource_handler cur_sem( sem_name_ );   // セマフォを再オープン & セマフォの取得をまつ。

		semaphore_post_guard spg( cur_sem, semaphore_post_guard_adopt_acquire_t() );
		// デストラクタで、sem_postを行う。このsem_postは、ほかのプロセス向けにpostを行う。

		int final_ref_c = p_mem_->reference_count_.fetch_sub( 1 ) - 1;
		if ( final_ref_c == 0 ) {
			id_res_.do_unlink();
			shm_res_.do_unlink();
			cur_sem.do_unlink();
		}

	} catch ( ... ) {
		// 握りつぶす
		fprintf( stderr, "procshared_mem::impl destructor catch exception\n" );
	}
}

procshared_mem::impl::impl( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg )
  : sem_name_()
  , id_res_()
  , shm_res_()
  , p_mem_( nullptr )
{
	check_path_name( p_shm_name );
	size_t nessesary_size = calc_total_neccesary_len( length );
	setup_as_both( p_shm_name, p_id_dirname, mode, nessesary_size, initfunctor_arg );
	sem_name_ = p_shm_name;
}

procshared_mem::impl::impl( const construct_as_primary_tag, const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg )
  : sem_name_()
  , id_res_()
  , shm_res_()
  , p_mem_( nullptr )
{
	check_path_name( p_shm_name );
	size_t nessesary_size = calc_total_neccesary_len( length );
	setup_as_primary( p_shm_name, p_id_dirname, mode, nessesary_size, initfunctor_arg );
	sem_name_ = p_shm_name;
}

procshared_mem::impl::impl( const construct_as_secondary_tag, const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode )
  : sem_name_()
  , id_res_()
  , shm_res_()
  , p_mem_( nullptr )
{
	check_path_name( p_shm_name );
	size_t nessesary_size = calc_total_neccesary_len( length );
	setup_as_secondary( p_shm_name, p_id_dirname, mode, nessesary_size );
	sem_name_ = p_shm_name;
}

void procshared_mem::impl::setup_as_both( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg, std::function<void( void*, off_t )> initfunctor_arg )
{
	while ( !try_setup_as_both( 0, p_shm_name_arg, p_id_dirname_arg, mode_arg, length_arg, initfunctor_arg ) ) {
	}
}

void procshared_mem::impl::setup_as_primary( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg, std::function<void( void*, off_t )> initfunctor_arg )
{
	try_setup_as_both( 1, p_shm_name_arg, p_id_dirname_arg, mode_arg, length_arg, initfunctor_arg );
}

void procshared_mem::impl::setup_as_secondary( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg )
{
	while ( !try_setup_as_both( 2, p_shm_name_arg, p_id_dirname_arg, mode_arg, length_arg, std::function<void( void*, off_t )>() ) ) {
	}
}

bool procshared_mem::impl::try_setup_as_both( int role_type, const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg, std::function<void( void*, off_t )> initfunctor_arg )
{
	std::string              id_fname = get_id_filename( p_shm_name_arg, p_id_dirname_arg );
	id_file_resource_handler cur_id_res( id_fname, mode_arg );
	if ( !cur_id_res.is_valid() ) {
		throw procshared_mem_error( "fail top open id file" );
	}

	bool                       is_primary = true;
	semaphore_resource_handler cur_sem_res;
	switch ( role_type ) {
		case 0: /* both role */ {
			cur_sem_res = semaphore_resource_handler( p_shm_name_arg, mode_arg );
			if ( !cur_sem_res.is_valid() ) {
				// 作成オープンできなかったので、secondary扱いとして、オープンを再トライする。
				is_primary  = false;
				cur_sem_res = semaphore_resource_handler( p_shm_name_arg );
				if ( !cur_sem_res.is_valid() ) {
					return false;
				}
			}
		} break;

		case 1: /* primary */ {
			cur_sem_res = semaphore_resource_handler( p_shm_name_arg, mode_arg );
			if ( !cur_sem_res.is_valid() ) {
				return false;
			}
		} break;

		case 2: /* secondary */ {
			cur_sem_res = semaphore_resource_handler( p_shm_name_arg );
			if ( !cur_sem_res.is_valid() ) {
				return false;
			}
			is_primary = false;
		} break;

		default: {
			throw procshared_mem_error( "role type error" );
		} break;
	}
	semaphore_post_guard( cur_sem_res, semaphore_post_guard_adopt_acquire_t() );

	id_file_resource_handler tmp_id_file = id_file_resource_handler( id_fname );
	if ( !tmp_id_file.is_valid() ) {
		return false;
	}
	if ( cur_id_res.get_inode_number() != tmp_id_file.get_inode_number() ) {
		return false;
	}

	shm_resource_handler       cur_shm_res;
	procshared_mem_mem_header* p_cur_mem;
	if ( is_primary ) {
		// primary
		cur_shm_res = shm_resource_handler( shm_resource_handler::try_create_tag(), p_shm_name_arg, length_arg, mode_arg );
		if ( !cur_shm_res.is_valid() ) {
			return false;
		}

		p_cur_mem = new ( cur_shm_res.get_shm_pointer() ) procshared_mem_mem_header( cur_id_res.get_inode_number(), length_arg );

		initfunctor_arg( p_cur_mem->shm_buff_, calc_available_size( cur_shm_res.allocated_size() ) );
	} else {
		// secondary
		cur_shm_res = shm_resource_handler( shm_resource_handler::try_open_tag(), p_shm_name_arg, length_arg, mode_arg );
		if ( !cur_shm_res.is_valid() ) {
			return false;
		}

		p_cur_mem = reinterpret_cast<procshared_mem_mem_header*>( cur_shm_res.get_shm_pointer() );
		if ( cur_id_res.get_inode_number() != p_cur_mem->inode_val_.load() ) {
			return false;
		}
		if ( p_cur_mem->inode_val_.load() < 1 ) {
			return false;
		}
	}

	// 参照カウント+1
	p_cur_mem->inode_val_.fetch_add( 1 );

	// メンバ変数へ設定。
	id_res_  = std::move( cur_id_res );
	shm_res_ = std::move( cur_shm_res );
	p_mem_   = p_cur_mem;

	return true;   // セマフォポスト。そのあと、セマフォクローズ
}

void* procshared_mem::impl::get( void )
{
	procshared_mem_mem_header* p_header = reinterpret_cast<procshared_mem_mem_header*>( p_mem_ );
	return reinterpret_cast<void*>( p_header->shm_buff_ );
}

off_t procshared_mem::impl::available_size( void ) const
{
	if ( !shm_res_.is_valid() ) return 0;

	return calc_available_size( shm_res_.allocated_size() );
}

ino_t procshared_mem::impl::debug_get_id_file_inode( void ) const
{
	return reinterpret_cast<procshared_mem_mem_header*>( p_mem_ )->inode_val_.load();
}

bool procshared_mem::impl::debug_test_integrity( void ) const
{
	bool ans = false;
	try {
		ans = ( debug_get_id_file_inode() == id_res_.get_inode_number() );

	} catch ( procshared_mem_error& e ) {
		ans = false;
	}

	return ans;
}

std::string procshared_mem::impl::debug_dump_string( void ) const
{
	char buff[2048];

	snprintf( buff, 2048,
	          "p_mem_      = %p\n",
	          p_mem_ );

	std::string ans( buff );
	return ans;
}

//////////////////////////////////////////////////////////////////////////////////
procshared_mem::procshared_mem( void )
  : p_impl_( nullptr )
{
}

procshared_mem::~procshared_mem()
{
	delete p_impl_;
}

procshared_mem::procshared_mem( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg )
  : p_impl_( nullptr )
{
	p_impl_ = new impl( p_shm_name, p_id_dirname, length, mode, initfunctor_arg );
}

void procshared_mem::allocate_shm_as_both( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg )
{
	delete p_impl_;
	p_impl_ = new impl( p_shm_name, p_id_dirname, length, mode, initfunctor_arg );
}

void procshared_mem::allocate_shm_as_primary( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg )
{
	delete p_impl_;
	p_impl_ = new impl( procshared_mem::impl::construct_as_primary_tag(), p_shm_name, p_id_dirname, length, mode, initfunctor_arg );
}

void procshared_mem::allocate_shm_as_secondary( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode )
{
	delete p_impl_;
	p_impl_ = new impl( procshared_mem::impl::construct_as_secondary_tag(), p_shm_name, p_id_dirname, length, mode );
}

off_t procshared_mem::available_size( void ) const
{
	if ( p_impl_ == nullptr ) return 0;

	return p_impl_->available_size();
}

void* procshared_mem::get( void )
{
	if ( p_impl_ == nullptr ) return nullptr;

	return p_impl_->get();
}

ino_t procshared_mem::debug_get_id_file_inode( void ) const
{
	if ( p_impl_ == nullptr ) return 0;

	return p_impl_->debug_get_id_file_inode();
}
bool procshared_mem::debug_test_integrity( void ) const
{
	if ( p_impl_ == nullptr ) return false;

	return p_impl_->debug_test_integrity();
}
std::string procshared_mem::debug_dump_string( void ) const
{
	if ( p_impl_ == nullptr ) return std::string();

	return p_impl_->debug_dump_string();
}
void procshared_mem::debug_force_cleanup( const char* p_shm_name )
{
	int ret = shm_unlink( p_shm_name );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		auto es        = make_strerror( cur_errno );
		fprintf( stderr, "%s by shm_unlink(%s)\n", es.c_str(), p_shm_name );
	}
	ret = sem_unlink( p_shm_name );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		auto es        = make_strerror( cur_errno );
		fprintf( stderr, "%s by sem_unlink(%s)\n", es.c_str(), p_shm_name );
	}
}
