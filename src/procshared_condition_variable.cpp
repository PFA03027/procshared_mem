/**
 * @file procshared_condition_variable.cpp
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

procshared_condition_variable::procshared_condition_variable( void )
{
	pthread_condattr_t cond_attr;
	pthread_condattr_init( &cond_attr );
	int ret = pthread_condattr_setpshared( &cond_attr, PTHREAD_PROCESS_SHARED );
	if ( ret != 0 ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to set pthread kind by pthread_condattr_setpshared()" );
	}
	ret = pthread_condattr_setclock( &cond_attr, CLOCK_MONOTONIC );
	if ( ret != 0 ) {
		std::error_code ec( ret, std::system_category() );
		throw std::system_error( ec, " fail to set pthread kind by pthread_condattr_setclock()" );
	}
	pthread_cond_init( &cond_steady_, &cond_attr );
	pthread_condattr_destroy( &cond_attr );
}

procshared_condition_variable::~procshared_condition_variable()
{
	int ret = pthread_cond_destroy( &cond_steady_ );
	if ( ret == EBUSY ) {
		psm_logoutput( psm_log_lv::kErr, "Error: fail pthread_cond_destroy(), caused by EBUSY(other thread/s are still waiting.)" );
	}
}

void procshared_condition_variable::notify_one() noexcept
{
	pthread_cond_signal( &cond_steady_ );
}
void procshared_condition_variable::notify_all() noexcept
{
	pthread_cond_broadcast( &cond_steady_ );
}

void procshared_condition_variable::wait( std::unique_lock<procshared_mutex>& lock )
{
	pthread_cond_wait( &cond_steady_, lock.mutex()->native_handle() );
}

std::cv_status procshared_condition_variable::wait_until(
	std::unique_lock<procshared_mutex>&  lock,
	const time_util::timespec_monotonic& abs_time )
{
	bool           loopflag = true;
	std::cv_status ans      = std::cv_status::no_timeout;

	do {
		int ret = pthread_cond_timedwait( &cond_steady_, lock.mutex()->native_handle(), &( abs_time.get() ) );
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

std::cv_status procshared_condition_variable::wait_until(
	std::unique_lock<procshared_mutex>&          lock,
	const std::chrono::steady_clock::time_point& abs_time )
{
	auto timediff = abs_time - std::chrono::steady_clock::now();
	// clock_gettime()よりも先にsteady_clock::now()を呼び出さないと、原理的に正しく動作しない。
	// 具体的には戻り値が、少なくともtpよりも早い時間となる可能性がある。その場合、Timeout時刻として仕様を満たさない。

	time_util::timespec_monotonic target_timespec = time_util::timespec_monotonic::now() + timediff;

	return wait_until( lock, target_timespec );
}
