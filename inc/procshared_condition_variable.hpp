/**
 * @file procshared_condition_variable_base.hpp
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

#ifndef PROCSHARED_CONDITION_VARIABLE_HPP_
#define PROCSHARED_CONDITION_VARIABLE_HPP_

#include <condition_variable>
#include <mutex>
#include <type_traits>

#include <pthread.h>

#include "procshared_mutex.hpp"
#include "procshared_time_util.hpp"

namespace ipsm {

/**
 * @brief base class of condition variable that is sharable b/w processes as common part
 *
 * @warning this class support std::chrono::steady_clock only.
 *
 */
class procshared_condition_variable_base {
public:
	using native_handle_type = pthread_cond_t*;

	procshared_condition_variable_base( clockid_t ct );
	~procshared_condition_variable_base();

	void notify_one() noexcept;
	void notify_all() noexcept;

	void wait( std::unique_lock<procshared_mutex>& lock );

	template <class Predicate>
	void wait( std::unique_lock<procshared_mutex>& lock, Predicate pred )
	{
		while ( !pred() ) {
			wait( lock );
		}
	}

	native_handle_type native_handle()
	{
		return &cond_;
	}

protected:
	std::cv_status wait_until( std::unique_lock<procshared_mutex>& lock, const struct timespec& abs_time );

private:
	procshared_condition_variable_base( const procshared_condition_variable_base& )            = delete;
	procshared_condition_variable_base& operator=( const procshared_condition_variable_base& ) = delete;

	pthread_cond_t cond_;
};

template <clockid_t CT>
class procshared_condition_variable : public procshared_condition_variable_base {
public:
	using native_handle_type = typename procshared_condition_variable_base::native_handle_type;

	procshared_condition_variable( void )
	  : procshared_condition_variable_base( CT )
	{
	}

	// void notify_one() noexcept; is defined in parent class
	// void notify_all() noexcept; is defined in parent class

	// void wait( std::unique_lock<procshared_mutex>& lock ); is defined in parent class

	// template <class Predicate>
	// void wait( std::unique_lock<procshared_mutex>& lock, Predicate pred ) is defined in parent class

	std::cv_status wait_until( std::unique_lock<procshared_mutex>& lock, const time_util::timespec_ct<CT>& abs_time )
	{
		return procshared_condition_variable_base::wait_until( lock, abs_time.get() );
	}

	template <class Predicate>
	bool wait_until( std::unique_lock<procshared_mutex>& lock,
	                 const time_util::timespec_ct<CT>&   abs_time,
	                 Predicate                           pred )
	{
		while ( !pred() ) {
			if ( wait_until( lock, abs_time ) == std::cv_status::timeout ) {
				return pred();
			}
		}
		return true;
	}

	template <class Rep, class Period, clockid_t CTX = CT, typename std::enable_if<time_util::timespec_ct<CT>::is_steady>::type* = nullptr>
	std::cv_status wait_for( std::unique_lock<procshared_mutex>&       lock,
	                         const std::chrono::duration<Rep, Period>& rel_time )
	{
		return wait_until( lock, time_util::timespec_ct<CT>::now() + rel_time );
	}

	template <class Rep, class Period, class Predicate, clockid_t CTX = CT, typename std::enable_if<time_util::timespec_ct<CT>::is_steady>::type* = nullptr>
	bool wait_for( std::unique_lock<procshared_mutex>&       lock,
	               const std::chrono::duration<Rep, Period>& rel_time,
	               Predicate                                 pred )
	{
		return wait_until( lock, time_util::timespec_ct<CT>::now() + rel_time, pred );
	}

	// native_handle_type native_handle() is defined in parent class

private:
	procshared_condition_variable( const procshared_condition_variable& )            = delete;
	procshared_condition_variable& operator=( const procshared_condition_variable& ) = delete;
};

using procshared_condition_variable_monotonic = procshared_condition_variable<CLOCK_MONOTONIC>;
using procshared_condition_variable_realtime  = procshared_condition_variable<CLOCK_REALTIME>;

static_assert( std::is_standard_layout<procshared_condition_variable_monotonic>::value, "procshared_condition_variable_base needs standard layout" );

}   // namespace ipsm

#endif
