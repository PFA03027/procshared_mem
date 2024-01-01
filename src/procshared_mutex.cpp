/**
 * @file procshared_mutex_base.cpp
 * @author PFA03027@nifty.com
 * @brief utility class of procshared_mem
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <system_error>

#include <pthread.h>

#include "procshared_logger.hpp"
#include "procshared_mutex.hpp"
#include "procshared_mutex_base.hpp"
#include "procshared_recursive_mutex.hpp"

procshared_mutex_base::procshared_mutex_base( int kind )
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init( &attr );
	int ret = pthread_mutexattr_settype( &attr, kind );
	if ( ret != 0 ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to set pthread kind by pthread_mutexattr_settype()" );
	}

	ret = pthread_mutexattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
	if ( ( ret != ENOSYS ) && ( ret != 0 ) ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to set PTHREAD_PROCESS_SHARED" );
	}

	ret = pthread_mutexattr_setrobust( &attr, PTHREAD_MUTEX_ROBUST );
	if ( ret != 0 ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to set PTHREAD_MUTEX_ROBUST" );
	}

	pthread_mutex_init( &fastmutex_, &attr );
	pthread_mutexattr_destroy( &attr );
}

procshared_mutex_base::~procshared_mutex_base()
{
	int ret = pthread_mutex_destroy( &fastmutex_ );
	if ( ret == EBUSY ) {
		pthread_mutex_unlock( &fastmutex_ );    // errorは無視する
		pthread_mutex_destroy( &fastmutex_ );   // errorは無視する
	}
}

void procshared_mutex_base::lock( void )
{
	int ret = pthread_mutex_lock( &fastmutex_ );
	if ( ret == 0 ) {
		// OK
	} else if ( ret == EOWNERDEAD ) {
		// try recover
		ret = pthread_mutex_consistent( &fastmutex_ );
		if ( ret == 0 ) {
			// OK, recovered
		} else if ( ret == EINVAL ) {
			// not recovered, but not matter.
			// fastmutex_ has already destroyed, or fastmutex_ is not inconsistent.
			psm_logoutput( psm_log_lv::kWarn, "Warning: Fail to call pthread_mutex_consistent(). Has mutex alread destroyed ?" );
		} else {
			// fail to recover. this means mutex may corrupted.
			std::error_code ec( ret, std::system_category() );
			throw std::system_error( ec, "Fail to call pthread_mutex_consistent() in pthread_mutex_lock()" );
		}
	} else {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, "Fail to call pthread_mutex_lock()" );
	}
}
bool procshared_mutex_base::try_lock( void )
{
	bool ans = false;
	int  ret = pthread_mutex_trylock( &fastmutex_ );
	if ( ret == 0 ) {
		ans = true;   // success to get lock
	} else if ( ( ret == EBUSY ) || ( ret == EDEADLK ) ) {
		ans = false;   // fail to get lock
	} else if ( ret == EOWNERDEAD ) {
		// try recover
		ret = pthread_mutex_consistent( &fastmutex_ );
		if ( ret == 0 ) {
			// OK, recovered
			ans = true;   // success to get lock
		} else if ( ret == EINVAL ) {
			// not recovered, but not matter. but, fail to get lock anyway.
			// fastmutex_ has already destroyed, or fastmutex_ is not inconsistent.
			ans = false;   // fail to get lock
			psm_logoutput( psm_log_lv::kWarn, "Warning: Fail to call pthread_mutex_consistent(). Has mutex alread destroyed ?" );
		} else {
			// fail to recover. this means mutex may corrupted.
			std::error_code ec( ret, std::system_category() );
			throw std::system_error( ec, "Fail to call pthread_mutex_consistent() in pthread_mutex_trylock()" );
		}
	} else {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, "Fail to call pthread_mutex_trylock()" );
	}
	return ans;
}
void procshared_mutex_base::unlock( void )
{
	int ret = pthread_mutex_unlock( &fastmutex_ );
	if ( ret == 0 ) {
		// OK
	} else if ( ret == EPERM ) {
		// caller thread is not lock owner thread
		psm_logoutput( psm_log_lv::kWarn, "Warning: caller thread is not mutex lock owner. caller side may have critical logic error" );
	} else {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, "Fail to call pthread_mutex_unlock()" );
	}
}

procshared_mutex::procshared_mutex( void )
#ifdef ENABLE_PTHREAD_MUTEX_ERRORTYPE
  : mtx_( PTHREAD_MUTEX_ERRORCHECK_NP )
#else
  : mtx_( PTHREAD_MUTEX_FAST_NP )
#endif
{
}

procshared_recursive_mutex::procshared_recursive_mutex( void )
  : mtx_( PTHREAD_MUTEX_RECURSIVE_NP )
{
}
