/**
 * @file procshared_mutex_base.cpp
 * @author PFA03027@nifty.com
 * @brief utility class of procshared_mem
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023, PFA03027@nifty.com
 *
 */

#include <stdexcept>
#include <system_error>

#include <pthread.h>

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
	if ( ret != 0 ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to call pthread_mutex_lock()" );
	}
}
bool procshared_mutex_base::try_lock( void )
{
	bool ans = false;
	int  ret = pthread_mutex_trylock( &fastmutex_ );
	if ( ret == 0 ) {
		ans = true;   // success to get lock
	} else if ( ret == EBUSY ) {
		ans = false;   // fail to get lock
	} else {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to call pthread_mutex_trylock()" );
	}
	return ans;
}
void procshared_mutex_base::unlock( void )
{
	int ret = pthread_mutex_unlock( &fastmutex_ );
	if ( ret != 0 ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to call pthread_mutex_unlock()" );
	}
}

procshared_mutex::procshared_mutex( void )
  : mtx_( PTHREAD_MUTEX_FAST_NP )
{
}

procshared_recursive_mutex::procshared_recursive_mutex( void )
  : mtx_( PTHREAD_MUTEX_RECURSIVE_NP )
{
}
