/**
 * @file procshared_mem_internal.hpp
 * @author PFA03027@nifty.com
 * @brief async open/close shared memory object
 * @version 0.1
 * @date 2023-12-17
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef SEMAPHORE_MUTEX_HPP_
#define SEMAPHORE_MUTEX_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
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

#include "procshared_logger.hpp"
#include "procshared_mem.hpp"

ino_t get_inode_of_fd( int id_f_fd );

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
				psm_logoutput( psm_log_lv::kErr, "Error: Fail to sem_close(%p), %s", p_sem_, es.c_str() );
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
		if ( sem_name_.empty() ) {
			psm_logoutput( psm_log_lv::kWarn, "Warning: semaphore is requested to sem_unlink(), but sem_name_ is empty" );
			return;
		}
		// psm_logoutput( psm_log_lv::kDebug, "Debug: semaphore deletion is requested, %s", sem_name_.c_str() );

		if ( sem_unlink( sem_name_.c_str() ) != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			psm_logoutput( psm_log_lv::kErr, "Error: Fail to sem_unlink(%s), %s", sem_name_.c_str(), es.c_str() );
		} else {
			// printf( "DEBUG: semaphore is sem_unlink(%s)\n", sem_name_.c_str() );
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
		call_sem_trywait();
		return owns_acquire_flag_;
	}

	void post( void )
	{
		if ( p_sem_ == SEM_FAILED ) {
			psm_logoutput( psm_log_lv::kErr, "Error: unexpected calling call_sem_post() by this=%p", this );
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
	void call_sem_trywait( void )
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

			psm_logoutput( psm_log_lv::kErr, "Error: sem_trywait(%p), but semaphore value is already 0(Zero)", p_sem_ );
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
				psm_logoutput( psm_log_lv::kErr, "Error: Fail sem_post(%p): %s", p_sem_, errlog.c_str() );
			} catch ( ... ) {
				psm_logoutput( psm_log_lv::kErr, "Error: Fail sem_post(%p): errno=%d", p_sem_, cur_errno );
			}
		}
		owns_acquire_flag_ = false;
	}

	sem_t* p_sem_;
	bool   owns_acquire_flag_;
};

#endif   // SEMAPHORE_MUTEX_HPP_
