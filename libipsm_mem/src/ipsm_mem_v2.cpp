/**
 * @file ipsm_mem_v2.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-08
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <chrono>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>

#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ipsm_logger.hpp"
#include "misc_utility.hpp"

#include "ipsm_mem.hpp"
#include "ipsm_mem_v2_internal.hpp"

namespace ipsm_v2 {

static size_t roundup_to_page_size( size_t length )
{
	size_t page_size = static_cast<size_t>( sysconf( _SC_PAGESIZE ) );
	return ( ( length + page_size - 1 ) / page_size ) * page_size;
}

// ==============================================================================
lock_file_guard::~lock_file_guard()
{
	if ( fd_ < 0 ) {
		return;
	}
	try {
		release_lock();
	} catch ( const std::exception& e ) {
		psm_logoutput( ipsm::psm_log_lv::kErr, "exception in lock_file_guard destructor: %s", e.what() );
	} catch ( ... ) {
		psm_logoutput( ipsm::psm_log_lv::kErr, "unknown exception in lock_file_guard destructor" );
	}
	close( fd_ );
}

lock_file_guard::lock_file_guard( const std::string& fname, mode_t mode )
  : fd_( -1 )
  , is_locked_( false )
{
	if ( fname.empty() ) {
		// same as default constructor
		return;
	}

	// open or create a lock file
	fd_ = open( fname.c_str(), O_RDWR | O_CREAT | O_NOFOLLOW, mode );
	if ( fd_ < 0 ) {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to open lock file: %s, error: %s", fname.c_str(), ipsm::make_strerror( cur_errno ).c_str() );
		throw ipsm::ipsm_mem_error( cur_errno, "failed to open lock file: " + fname );
	}
}

bool lock_file_guard::try_exclusive_lock( void )
{
	if ( fd_ < 0 ) {
		return false;
	}

	int ret = flock( fd_, LOCK_EX | LOCK_NB );
	if ( ret == 0 ) {
		is_locked_ = true;
		return true;
	} else {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to acquire exclusive lock, error: %s", ipsm::make_strerror( cur_errno ).c_str() );
		if ( cur_errno != EWOULDBLOCK ) {
			throw ipsm::ipsm_mem_error( cur_errno, "failed to acquire exclusive lock" );
		}
		return false;
	}
}

bool lock_file_guard::try_shared_lock( void )
{
	if ( fd_ < 0 ) {
		return false;
	}

	int ret = flock( fd_, LOCK_SH | LOCK_NB );
	if ( ret == 0 ) {
		is_locked_ = true;
		return true;
	} else {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to acquire shared lock, error: %s", ipsm::make_strerror( cur_errno ).c_str() );
		if ( cur_errno != EWOULDBLOCK ) {
			throw ipsm::ipsm_mem_error( cur_errno, "failed to acquire shared lock" );
		}
		return false;
	}
}

void lock_file_guard::release_lock( void )
{
	if ( fd_ < 0 || !is_locked_ ) {
		return;
	}

	int ret = flock( fd_, LOCK_UN );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to release lock, error: %s", ipsm::make_strerror( cur_errno ).c_str() );
#ifdef NDEBUG
#else
		throw ipsm::ipsm_mem_error( cur_errno, "failed to release lock" );
#endif
	}
	is_locked_ = false;
}

// ==============================================================================
shm_guard::~shm_guard()
{
	if ( p_addr_ != nullptr ) {
		munmap( p_addr_, length_ );
		p_addr_ = nullptr;
	}
	if ( fd_ >= 0 ) {
		close( fd_ );
		fd_ = -1;
	}
}

shm_guard::shm_guard( void )
  : fd_( -1 )
  , p_addr_( nullptr )
  , length_( 0 )
{
}

shm_guard::shm_guard( shm_guard&& src )
  : fd_( src.fd_ )
  , p_addr_( src.p_addr_ )
  , length_( src.length_ )
{
	src.fd_     = -1;
	src.p_addr_ = nullptr;
	src.length_ = 0;
}

shm_guard& shm_guard::operator=( shm_guard&& src )
{
	if ( this == &src ) {
		return *this;
	}

	shm_guard( std::move( src ) ).swap( *this );

	return *this;
}

void shm_guard::swap( shm_guard& src )
{
	std::swap( fd_, src.fd_ );
	std::swap( p_addr_, src.p_addr_ );
	std::swap( length_, src.length_ );
}

bool shm_guard::open( const std::string& shm_name, size_t length, mode_t mode )
{
	if ( shm_name.empty() ) {
		throw std::invalid_argument( "shared memory name is empty" );
	}
	if ( length == 0 ) {
		throw std::invalid_argument( "shared memory length is zero" );
	}
	if ( length > static_cast<size_t>( std::numeric_limits<off_t>::max() ) ) {
		throw std::invalid_argument( "shared memory length is too large" );
	}

	// 共有メモリオブジェクトを開くために、shm_openを呼び出す
	int fd_ret = shm_open( shm_name.c_str(), O_RDWR, mode );
	if ( fd_ret < 0 ) {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to open shared memory object: %s, error: %s", shm_name.c_str(), ipsm::make_strerror( cur_errno ).c_str() );
		return false;
	}

	// sysconf(_SC_PAGESIZE)で、ページサイズを取得して、lengthをページサイズの倍数に切り上げる
	size_t aligned_length = roundup_to_page_size( length );

	// 共有メモリをマッピングするために、mmapを呼び出す
	void* p_addr_ret = mmap( nullptr, aligned_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ret, 0 );
	if ( p_addr_ret == MAP_FAILED ) {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to map shared memory object: %s, error: %s", shm_name.c_str(), ipsm::make_strerror( cur_errno ).c_str() );
		close( fd_ret );
		return false;
	}

	fd_     = fd_ret;
	p_addr_ = p_addr_ret;
	length_ = aligned_length;

	return true;
}

void shm_guard::create( const std::string& shm_name, size_t length, mode_t mode )
{
	if ( shm_name.empty() ) {
		throw std::invalid_argument( "shared memory name is empty" );
	}
	if ( shm_name[0] != '/' ) {
		throw std::invalid_argument( "shared memory name should start with '/'" );
	}
	if ( length == 0 ) {
		throw std::invalid_argument( "shared memory length is zero" );
	}
	if ( length > static_cast<size_t>( std::numeric_limits<off_t>::max() ) ) {
		throw std::invalid_argument( "shared memory length is too large" );
	}

	// 共有メモリオブジェクトを作成するために、shm_openを呼び出す
	// O_CREAT | O_TRUNCを指定して、共有メモリオブジェクトを作成する。すでに同名の共有メモリオブジェクトが存在する場合は、サイズをゼロに切り詰めて作り直しができるようにする。
	int fd_ret = shm_open( shm_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, mode );
	if ( fd_ret < 0 ) {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to create shared memory object: %s, error: %s", shm_name.c_str(), ipsm::make_strerror( cur_errno ).c_str() );
		throw ipsm::ipsm_mem_error( cur_errno, "failed to create shared memory object: " + shm_name );
	}

	// sysconf(_SC_PAGESIZE)で、ページサイズを取得して、lengthをページサイズの倍数に切り上げる
	size_t aligned_length = roundup_to_page_size( length );

	// 共有メモリのサイズを設定するために、ftruncateを呼び出す
	int ret = ftruncate( fd_ret, static_cast<off_t>( aligned_length ) );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to set size of shared memory object: %s, error: %s", shm_name.c_str(), ipsm::make_strerror( cur_errno ).c_str() );
		close( fd_ret );
		throw ipsm::ipsm_mem_error( cur_errno, "failed to set size of shared memory object: " + shm_name );
	}

	// 共有メモリをマッピングするために、mmapを呼び出す
	void* p_addr_ret = mmap( nullptr, aligned_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ret, 0 );
	if ( p_addr_ret == MAP_FAILED ) {
		auto cur_errno = errno;
		psm_logoutput( ipsm::psm_log_lv::kErr, "failed to map shared memory object: %s, error: %s", shm_name.c_str(), ipsm::make_strerror( cur_errno ).c_str() );
		close( fd_ret );
		throw ipsm::ipsm_mem_error( cur_errno, "failed to map shared memory object: " + shm_name );
	}

	fd_     = fd_ret;
	p_addr_ = p_addr_ret;
	length_ = aligned_length;
}

// ==============================================================================
ipsm_mem::impl::~impl()
{
	try {
		shared_lock_guard_.release_lock();

		lock_file_guard exclusive_lock_guard( lifetime_ctrl_fname_, mode_ );
		if ( exclusive_lock_guard.try_exclusive_lock() ) {
			// 排他ロックが確保できた場合、このプロセスが最後のプロセスであることを示す。
			// よって、共有メモリオブジェクトを削除する。
			psm_logoutput( ipsm::psm_log_lv::kInfo, "This process is last process for shared memory: %s, unlinking that shared memory.", shm_name_.c_str() );
			shm_unlink( shm_name_.c_str() );
		}
	} catch ( const std::exception& e ) {
		psm_logoutput( ipsm::psm_log_lv::kErr, "exception in ipsm_mem::impl destructor: %s", e.what() );
	} catch ( ... ) {
		psm_logoutput( ipsm::psm_log_lv::kErr, "unknown exception in ipsm_mem::impl destructor" );
	}
}

ipsm_mem::impl::impl(
	const char* p_shm_name,
	const char* p_lifetime_ctrl_fname,
	size_t      length,
	mode_t      mode,
	int         timeout_msec,
	int         retry_interval_msec )
  : shm_name_( p_shm_name )
  , lifetime_ctrl_fname_( p_lifetime_ctrl_fname )
  , req_length_( length )
  , mode_( mode )
  , shared_lock_guard_( lifetime_ctrl_fname_, mode )
  , shm_guard_()
  , shm_length_( 0 )
  , available_length_( 0 )
{
	const size_t nessesary_size     = req_length_ + sizeof( ipsm_mem::status );
	const auto   timeout_time_point = std::chrono::steady_clock::now() + std::chrono::milliseconds( timeout_msec );

	while ( true ) {
		shm_guard shm_create_guard;

		// 排他ロックの取得を試みる。排他ロックを取得出来た場合、このプロセスが共有メモリの初期化を行うプロセスとなる。
		{
			lock_file_guard exclusive_lock_guard( lifetime_ctrl_fname_, mode_ );
			if ( exclusive_lock_guard.try_exclusive_lock() ) {
				// 共有メモリの初期化を行うプロセスの場合、共有メモリを作成してマッピングする
				shm_create_guard.create( shm_name_, nessesary_size, mode_ );

				ipsm_mem::status* p_status = reinterpret_cast<ipsm_mem::status*>( shm_create_guard.get() );
				*p_status                  = ipsm_mem::status::initializing;

				// TODO: 共有メモリの初期化処理を実装する。初期化処理は、init_functor_argで指定された関数オブジェクトを呼び出す形で実装する。
				// 共有メモリの初期化処理が完了したら、共有メモリの状態をreadyに変更する。
				*p_status = ipsm_mem::status::ready;
			}
		}
		{
			// 共有ロックの取得を試みる。共有ロックを取得出来た場合、共有メモリの初期化を行うプロセスが初期化処理を完了していることを示す。
			while ( !shared_lock_guard_.try_shared_lock() ) {
				std::this_thread::sleep_for( std::chrono::milliseconds( retry_interval_msec ) );
				if ( std::chrono::steady_clock::now() > timeout_time_point ) {
					throw ipsm::ipsm_mem_error( ETIMEDOUT, "timeout while waiting for shared memory initialization: " + shm_name_ );
				}
			}

			// 共有ロックの取得に成功した場合、共有メモリのオープンと状態の確認を行う
			// 共有メモリのオープンに失敗した場合、共有メモリの初期化を行うプロセスが初期化処理中にプロセスが終了したことを示す。
			bool ret = shm_guard_.open( shm_name_, nessesary_size, mode_ );
			if ( !ret ) {
				shared_lock_guard_.release_lock();
				continue;   // 共有メモリのオープンに失敗した場合、共有ロックを解放してから、再度、排他ロック確保からやり直す。
			}

			// 共有メモリの状態を確認する。状態がreadyでない場合、共有メモリの初期化を行うプロセスが初期化処理中にプロセスが終了したことを示す。
			// 共有メモリと、共有ロックを解放して、再度、排他ロック確保からやり直す。
			ipsm_mem::status* p_status = reinterpret_cast<ipsm_mem::status*>( shm_guard_.get() );
			if ( *p_status != ipsm_mem::status::ready ) {
				shm_guard_ = shm_guard();   // 共有メモリのマッピングを解除する
				shared_lock_guard_.release_lock();
				continue;   // 共有メモリの状態がreadyでない場合、共有ロックを解放してから、再度最初からやり直す。
			}

			// 共有メモリのオープンと状態の確認に成功した場合、ループを抜ける
			break;
		}
	}

	shm_length_       = shm_guard_.mmap_length();
	available_length_ = shm_length_ - sizeof( ipsm_mem::status );
}

void* ipsm_mem::impl::get( void ) const
{
	std::uintptr_t addr = reinterpret_cast<std::uintptr_t>( shm_guard_.get() );
	if ( addr == 0 ) {
		return nullptr;
	}
	addr += sizeof( ipsm_mem::status );
	return reinterpret_cast<void*>( addr );
}

size_t ipsm_mem::impl::available_size( void ) const
{
	return available_length_;
}

ipsm_mem::status ipsm_mem::impl::get_status( void ) const
{
	if ( shm_guard_.get() == nullptr ) {
		psm_logoutput( ipsm::psm_log_lv::kWarn, "shared memory is not allocated" );
		return ipsm_mem::status::initializing;
	}

	ipsm_mem::status* p_status = reinterpret_cast<ipsm_mem::status*>( shm_guard_.get() );
	return *p_status;
}

// ==============================================================================
ipsm_mem::~ipsm_mem()
{
	delete p_impl_;
	p_impl_ = nullptr;
}
ipsm_mem::ipsm_mem( void )
  : p_impl_( nullptr )
{
}
ipsm_mem::ipsm_mem( ipsm_mem&& src )
  : p_impl_( src.p_impl_ )
{
	src.p_impl_ = nullptr;
}
ipsm_mem& ipsm_mem::operator=( ipsm_mem&& src )
{
	if ( this == &src ) {
		return *this;
	}

	ipsm_mem( std::move( src ) ).swap( *this );

	return *this;
}

void ipsm_mem::swap( ipsm_mem& src )
{
	std::swap( p_impl_, src.p_impl_ );
}

/**
 * @brief allocate a new cooperative startup shared memory object
 *
 * @pre this instance is default constructed instance
 *
 * @exception ipsm_mem_error
 */
void ipsm_mem::setup(
	const char*                           p_shm_name,              //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
	const char*                           p_lifetime_ctrl_fname,   //!< [in] lifetime control file name.
	size_t                                length,                  //!< [in] shared memory size
	mode_t                                mode,                    //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	std::function<void*( void*, size_t )> init_functor_arg,        //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length. return value is set to opt_info.
	int                                   timeout_msec,            //!< [in] timeout in milliseconds for waiting for shared memory initialization.
	int                                   retry_interval_msec      //!< [in] retry interval in milliseconds for waiting for shared memory initialization.
)
{
	if ( p_impl_ != nullptr ) {
		psm_logoutput( ipsm::psm_log_lv::kWarn, "shared memory is already allocated" );
		return;
	}

	p_impl_ = new impl( p_shm_name, p_lifetime_ctrl_fname, length, mode, timeout_msec, retry_interval_msec );
}

void* ipsm_mem::get( void ) const
{
	if ( p_impl_ == nullptr ) {
		return nullptr;
	}

	return p_impl_->get();
}

size_t ipsm_mem::available_size( void ) const
{
	if ( p_impl_ == nullptr ) {
		return 0;
	}

	return p_impl_->available_size();
}

ipsm_mem::status ipsm_mem::get_status( void ) const
{
	if ( p_impl_ == nullptr ) {
		return ipsm_mem::status::initializing;
	}

	return p_impl_->get_status();
}

}   // namespace ipsm_v2
