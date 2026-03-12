/**
 * @file ipsm_mem.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-08
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <atomic>
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
#include "ipsm_mem_internal.hpp"

namespace ipsm {

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
		psm_logoutput( ipsm::psm_log_lv::kDebug, "failed to acquire exclusive lock, error: %s", ipsm::make_strerror( cur_errno ).c_str() );
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
		psm_logoutput( ipsm::psm_log_lv::kDebug, "failed to acquire shared lock, error: %s", ipsm::make_strerror( cur_errno ).c_str() );
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
		psm_logoutput( ipsm::psm_log_lv::kWarn, "failed to open shared memory object: %s, error: %s", shm_name.c_str(), ipsm::make_strerror( cur_errno ).c_str() );
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
struct ipsm_mem_header {
	std::atomic<ipsm_mem::status> status_;
	std::atomic<std::uintptr_t>   sharing_value_;   // 共有メモリの初期化後、共有ロックでオープンしたプロセスと共有する値。共有ロックでオープンしたプロセスは、この値を参照して、共有メモリの使用開始処理に反映する。

	ipsm_mem_header()
	  : status_( ipsm_mem::status::initializing )
	  , sharing_value_( 0 )
	{
	}

	void set_ready_with_sharing_value( std::uintptr_t value )
	{
		sharing_value_.store( value, std::memory_order_release );
		status_.store( ipsm_mem::status::ready, std::memory_order_release );
	}

	ipsm_mem::status get_status( void ) const
	{
		return status_.load( std::memory_order_acquire );
	}
};

// ==============================================================================
ipsm_mem::impl::~impl()
{
	try {
		shared_lock_guard_.release_lock();

		lock_file_guard exclusive_lock_guard( lifetime_ctrl_fname_, mode_ );
		if ( exclusive_lock_guard.try_exclusive_lock() ) {
			// 排他ロックが確保できた場合、このプロセスが最後のプロセスであることを示す。
			// よって、共有メモリオブジェクトを削除する。
			// psm_logoutput( ipsm::psm_log_lv::kInfo, "This process is last process for shared memory: %s, unlinking that shared memory.", shm_name_.c_str() );
			shm_unlink( shm_name_.c_str() );
		}
	} catch ( const std::exception& e ) {
		psm_logoutput( ipsm::psm_log_lv::kErr, "exception in ipsm_mem::impl destructor: %s", e.what() );
	} catch ( ... ) {
		psm_logoutput( ipsm::psm_log_lv::kErr, "unknown exception in ipsm_mem::impl destructor" );
	}
}

ipsm_mem::impl::impl(
	const char*                                    p_shm_name,
	const char*                                    p_lifetime_ctrl_fname,
	size_t                                         length,
	mode_t                                         mode,
	std::function<std::uintptr_t( void*, size_t )> creater_init_functor_arg,
	int                                            timeout_msec,
	int                                            retry_interval_msec )
  : shm_name_( p_shm_name )
  , lifetime_ctrl_fname_( p_lifetime_ctrl_fname )
  , req_length_( length )
  , mode_( mode )
  , shared_lock_guard_( lifetime_ctrl_fname_, mode )
  , shm_guard_()
  , shm_length_( 0 )
  , available_length_( 0 )
{
	const size_t nessesary_size     = req_length_ + sizeof( ipsm_mem_header );
	const auto   timeout_time_point = std::chrono::steady_clock::now() + std::chrono::milliseconds( timeout_msec );

	while ( true ) {
		shm_guard shm_create_guard;

		// 排他ロックの取得を試みる。排他ロックを取得出来た場合、このプロセスが共有メモリの初期化を行うプロセスとなる。
		{
			lock_file_guard exclusive_lock_guard( lifetime_ctrl_fname_, mode_ );
			if ( exclusive_lock_guard.try_exclusive_lock() ) {
				// 共有メモリの初期化を行うプロセスの場合、共有メモリを作成してマッピングする
				shm_create_guard.create( shm_name_, nessesary_size, mode_ );

				// 共有メモリのヘッダ領域の初期化
				ipsm_mem_header* p_header = new ( shm_create_guard.get() ) ipsm_mem_header();

				// ヘッダ領域の後ろに配置される領域の初期化を、init_functor_argで指定された関数オブジェクトを呼び出す形で実装する。
				std::uintptr_t addr               = reinterpret_cast<std::uintptr_t>( p_header ) + sizeof( ipsm_mem_header );
				size_t         cur_available_size = shm_create_guard.mmap_length() - sizeof( ipsm_mem_header );
				std::uintptr_t hint_value         = creater_init_functor_arg( reinterpret_cast<void*>( addr ), cur_available_size );

				// 共有メモリの初期化処理が完了したら、ヘッダ領域とその後ろの領域で消費されたサイズを指定して、readyに変更する。
				p_header->set_ready_with_sharing_value( hint_value );
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
				psm_logoutput( ipsm::psm_log_lv::kInfo, "Because fail to open shared memory, retry setup of %s", shm_name_.c_str() );
				continue;   // 共有メモリのオープンに失敗した場合、共有ロックを解放してから、再度、排他ロック確保からやり直す。
			}

			// 共有メモリの状態を確認する。状態がreadyでない場合、共有メモリの初期化を行うプロセスが初期化処理中にプロセスが終了したことを示す。
			// 共有メモリと、共有ロックを解放して、再度、排他ロック確保からやり直す。
			ipsm_mem_header* p_header = reinterpret_cast<ipsm_mem_header*>( shm_guard_.get() );
			if ( p_header->get_status() != ipsm_mem::status::ready ) {
				shm_guard_ = shm_guard();   // 共有メモリのマッピングを解除する
				shared_lock_guard_.release_lock();
				psm_logoutput( ipsm::psm_log_lv::kInfo, "Because of unexpected shared memory status(0x%llx), retry setup of %s", static_cast<std::uintptr_t>( p_header->get_status() ), shm_name_.c_str() );
				continue;   // 共有メモリの状態がreadyでない場合、共有ロックを解放してから、再度最初からやり直す。
			}

			// 共有メモリのオープンと状態の確認に成功した場合、ループを抜ける
			break;
		}
	}

	shm_length_       = shm_guard_.mmap_length();
	available_length_ = shm_length_ - sizeof( ipsm_mem_header );
}

void* ipsm_mem::impl::get( void ) const
{
	ipsm_mem_header* p_header = reinterpret_cast<ipsm_mem_header*>( shm_guard_.get() );
	if ( p_header == nullptr ) {
		return nullptr;
	}
	std::uintptr_t addr = reinterpret_cast<std::uintptr_t>( p_header );
	addr += sizeof( ipsm_mem_header );
	return reinterpret_cast<void*>( addr );
}

size_t ipsm_mem::impl::available_size( void ) const
{
	return available_length_;
}

ipsm_mem::status ipsm_mem::impl::get_status( void ) const
{
	ipsm_mem_header* p_header = reinterpret_cast<ipsm_mem_header*>( shm_guard_.get() );
	if ( p_header == nullptr ) {
		psm_logoutput( ipsm::psm_log_lv::kWarn, "shared memory is not allocated" );
		return ipsm_mem::status::initializing;
	}

	return p_header->status_.load( std::memory_order_acquire );
}

std::uintptr_t ipsm_mem::impl::get_hint_value( void ) const
{
	ipsm_mem_header* p_header = reinterpret_cast<ipsm_mem_header*>( shm_guard_.get() );
	if ( p_header == nullptr ) {
		psm_logoutput( ipsm::psm_log_lv::kWarn, "shared memory is not allocated" );
		return 0;
	}
	return p_header->sharing_value_.load( std::memory_order_acquire );
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

ipsm_mem::ipsm_mem(
	const char*                            p_shm_name,
	const char*                            p_lifetime_ctrl_fname,
	size_t                                 length,
	mode_t                                 mode,
	std::function<size_t( void*, size_t )> init_functor_arg,
	int                                    timeout_msec,
	int                                    retry_interval_msec )
  : p_impl_( nullptr )
{
	bool ret = setup( p_shm_name, p_lifetime_ctrl_fname, length, mode, init_functor_arg, timeout_msec, retry_interval_msec );
	if ( !ret ) {
		// 共有メモリの初期化に失敗した場合、共有メモリの初期化を行うプロセスが初期化処理中にプロセスが終了したことを示す。
		psm_logoutput( ipsm::psm_log_lv::kWarn, "failed to setup shared memory: %s", p_shm_name );
		delete p_impl_;
		p_impl_ = nullptr;
		ipsm::ipsm_mem_error e( ETIMEDOUT, "timeout while waiting for shared memory initialization: " + std::string( p_shm_name ) );
		throw e;
	}
}

bool ipsm_mem::setup(
	const char*                            p_shm_name,              //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
	const char*                            p_lifetime_ctrl_fname,   //!< [in] lifetime control file name.
	size_t                                 length,                  //!< [in] shared memory size
	mode_t                                 mode,                    //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	std::function<size_t( void*, size_t )> init_functor_arg,        //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length. return value is consumed memory size.
	int                                    timeout_msec,            //!< [in] timeout in milliseconds for waiting for shared memory initialization.
	int                                    retry_interval_msec      //!< [in] retry interval in milliseconds for waiting for shared memory initialization.
)
{
	if ( p_impl_ != nullptr ) {
		psm_logoutput( ipsm::psm_log_lv::kWarn, "shared memory is already allocated" );
		return true;
	}
	if ( p_shm_name == nullptr || p_shm_name[0] == '\0' ) {
		throw std::invalid_argument( "shared memory name is null or empty" );
	}
	if ( p_shm_name[0] != '/' ) {
		throw std::invalid_argument( "shared memory name should start with '/'" );
	}
	if ( p_lifetime_ctrl_fname == nullptr || p_lifetime_ctrl_fname[0] == '\0' ) {
		throw std::invalid_argument( "lifetime control file name is null or empty" );
	}
	if ( length == 0 ) {
		throw std::invalid_argument( "shared memory length is zero" );
	}

	try {
		p_impl_ = new impl( p_shm_name, p_lifetime_ctrl_fname, length, mode, init_functor_arg, timeout_msec, retry_interval_msec );
	} catch ( const ipsm::ipsm_mem_error& e ) {
		if ( e.code() == ETIMEDOUT ) {
			psm_logoutput( ipsm::psm_log_lv::kWarn, "timeout while waiting for shared memory initialization: %s", p_shm_name );
			return false;
		} else {
			psm_logoutput( ipsm::psm_log_lv::kWarn, "ipsm_mem_error is thrown with errno=%d: %s", e.code(), p_shm_name );
			throw;   // それ以外の例外は呼び出し元に伝える
		}
	}   // 上記以外の例外も呼び出し元に伝える

	return true;
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

std::uintptr_t ipsm_mem::get_hint_value( void ) const
{
	if ( p_impl_ == nullptr ) {
		return 0;
	}

	return p_impl_->get_hint_value();
}

}   // namespace ipsm
