/**
 * @file lockfile_mutex.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2024-01-21
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <chrono>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "lockfile_mutex.hpp"
#include "misc_utility.hpp"
#include "procshared_logger.hpp"

constexpr int invalid_fd = -1;
constexpr int maxdelay   = 64;

constexpr bool is_valid_fd( int tfd )
{
	return tfd >= 0;
}

void lockfile_mutex::debug_force_cleanup( const char* p_lockfilename )
{
	if ( p_lockfilename == nullptr ) {
		printf( "try to do lockfile_mutex::debug_force_cleanup(), but argument is nullptr\n" );
		return;
	}

	int ret = unlink( p_lockfilename );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		auto es        = make_strerror( cur_errno );
		printf( "%s by unlink(%s)\n", es.c_str(), p_lockfilename );
	} else {
		printf( "success to unlink id file: %s\n", p_lockfilename );
	}
}

lockfile_mutex::lockfile_mutex( const char* p_lockfilename, mode_t mode_arg )
  : lockfilename_( ( p_lockfilename == nullptr ) ? "" : p_lockfilename )
  , file_access_permission_( mode_arg )
  , lockfilefd_( invalid_fd )
{
	if ( p_lockfilename == nullptr ) {
		throw std::runtime_error( "pointer to lock file name string is nullptr" );
	}
	if ( lockfilename_.empty() ) {
		throw std::runtime_error( "lock file name string is empty" );
	}
}

lockfile_mutex::~lockfile_mutex()
{
	discard_lockfile();
}

lockfile_mutex::lockfile_mutex( lockfile_mutex&& src )
  : lockfilename_( std::move( src.lockfilename_ ) )
  , file_access_permission_( src.file_access_permission_ )
  , lockfilefd_( src.lockfilefd_ )
{
	src.lockfilefd_             = invalid_fd;
	src.file_access_permission_ = 0;
	src.lockfilename_.clear();
}
lockfile_mutex& lockfile_mutex::operator=( lockfile_mutex&& src )
{
	if ( this != &src ) {
		lockfile_mutex( std::move( src ) ).swap( *this );
	}
	return *this;
}

void lockfile_mutex::lock( void )
{
	int cur_sleep_time = 1;
	while ( !try_create_lockfile() ) {
		std::this_thread::sleep_for( std::chrono::milliseconds( cur_sleep_time ) );
		cur_sleep_time *= 2;
		if ( cur_sleep_time > maxdelay ) {
			thread_local pid_t                     seed_depens_on_thread = gettid();
			static std::mutex                      mtx_for_random;
			std::lock_guard<std::mutex>            lk( mtx_for_random );
			static std::mt19937                    engine( static_cast<std::mt19937::result_type>( seed_depens_on_thread ) );
			static std::uniform_int_distribution<> dist( 1, maxdelay );
			cur_sleep_time = dist( engine );
			psm_logoutput( psm_log_lv::kWarn, "Warning: reach to max delay time for lockfile(%s) open", lockfilename_.c_str() );
		}
	}
}
bool lockfile_mutex::try_lock( void )
{
	return try_create_lockfile();
}
void lockfile_mutex::unlock( void )
{
	discard_lockfile();
}

void lockfile_mutex::swap( lockfile_mutex& src )
{
	std::swap( lockfilefd_, src.lockfilefd_ );
	lockfilename_.swap( src.lockfilename_ );
}

bool lockfile_mutex::try_create_lockfile( void )
{
	bool ans = false;
	if ( is_valid_fd( lockfilefd_ ) ) {
		psm_logoutput( psm_log_lv::kWarn, "Warning: lockfile(%s) is already acquired. this means dual lock is happened", lockfilename_.c_str() );
		return false;
	}

	int tmpfd = open( lockfilename_.c_str(), O_RDWR | O_CLOEXEC | O_CREAT | O_EXCL, file_access_permission_ );
	if ( tmpfd >= 0 ) {
		lockfilefd_ = tmpfd;
		ans         = true;
	} else {
		ans            = false;
		auto cur_errno = errno;
		if ( cur_errno != EEXIST ) {
			std::string es = make_strerror( cur_errno );
			psm_logoutput( psm_log_lv::kErr, "Error: fail open lockfile(%s): error(%d), %s", lockfilename_.c_str(), cur_errno, es.c_str() );
		}
	}

	return ans;
}

void lockfile_mutex::discard_lockfile( void )
{
	if ( !is_valid_fd( lockfilefd_ ) ) {
		return;
	}

	int ret = unlink( lockfilename_.c_str() );
	if ( ret < 0 ) {
		auto cur_errno = errno;
		psm_logoutput( psm_log_lv::kErr, "error(%d) when unlink lockfile(%s)", cur_errno, lockfilename_.c_str() );
	}

	ret         = close( lockfilefd_ );
	lockfilefd_ = invalid_fd;
	if ( ret < 0 ) {
		auto cur_errno = errno;
		switch ( cur_errno ) {
			case EBADF: {
				psm_logoutput( psm_log_lv::kErr, "EBADF error when closing lockfile" );
			} break;
			case EINTR: {
				psm_logoutput( psm_log_lv::kErr, "EINTR error when closing lockfile" );
			} break;
			case EIO: {
				psm_logoutput( psm_log_lv::kErr, "EIO error when closing lockfile" );
			} break;
			default: {
				std::string es = make_strerror( cur_errno );
				psm_logoutput( psm_log_lv::kErr, "Error: fail close lockfile(%s), unknown error(%d), %s", lockfilename_.c_str(), cur_errno, es.c_str() );
			} break;
		}
	}
}
