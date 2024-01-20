/**
 * @file procshared_condition_variable_base.cpp
 * @author PFA03027@nifty.com
 * @brief condition variable that is sharable b/w processes
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 * @note
 * This class requires pthread library
 */

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <pthread.h>

#include "procshared_condition_variable.hpp"
#include "procshared_logger.hpp"
#include "procshared_time_util.hpp"

procshared_condition_variable_base::procshared_condition_variable_base( clockid_t ct )
{
	pthread_condattr_t cond_attr;
	pthread_condattr_init( &cond_attr );
	int ret = pthread_condattr_setpshared( &cond_attr, PTHREAD_PROCESS_SHARED );
	if ( ret != 0 ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to set pthread kind by pthread_condattr_setpshared()" );
	}
	ret = pthread_condattr_setclock( &cond_attr, ct );
	if ( ret != 0 ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to set pthread kind by pthread_condattr_setclock()" );
	}
	pthread_cond_init( &cond_, &cond_attr );
	pthread_condattr_destroy( &cond_attr );
}

procshared_condition_variable_base::~procshared_condition_variable_base()
{
	int ret = pthread_cond_destroy( &cond_ );
	if ( ret == EBUSY ) {
		psm_logoutput( psm_log_lv::kErr, "Error: fail pthread_cond_destroy(), caused by EBUSY(other thread/s are still waiting.)" );
	}
}

void procshared_condition_variable_base::notify_one() noexcept
{
	pthread_cond_signal( &cond_ );
}
void procshared_condition_variable_base::notify_all() noexcept
{
	pthread_cond_broadcast( &cond_ );
}

void procshared_condition_variable_base::wait( std::unique_lock<procshared_mutex>& lock )
{
	pthread_cond_wait( &cond_, lock.mutex()->native_handle() );
}

std::cv_status procshared_condition_variable_base::wait_until(
	std::unique_lock<procshared_mutex>& lock,
	const struct timespec&              abs_time )
{
	bool           loopflag = true;
	std::cv_status ans      = std::cv_status::no_timeout;

	do {
		int ret = pthread_cond_timedwait( &cond_, lock.mutex()->native_handle(), &( abs_time ) );
		switch ( ret ) {
			case 0: {
				ans      = std::cv_status::no_timeout;
				loopflag = false;
			} break;
			case ETIMEDOUT: {
				ans      = std::cv_status::timeout;
				loopflag = false;
			} break;
			case EINTR: {
				// need pthread_cond_timedwait() again
			} break;
			default: {
				// loopflag = false;
				std::error_code ec( ret, std::system_category() );
				throw std::system_error( ec, " pthread_cond_timedwait() return unknown error code" );
			} break;
		}
	} while ( loopflag );

	return ans;
}
