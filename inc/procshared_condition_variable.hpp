/**
 * @file procshared_condition_variable.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief condition variable that is sharable b/w processes
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
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

/**
 * @brief condition variable that is sharable b/w processes
 *
 * @warning this class support std::chrono::steady_clock only.
 *
 */
class procshared_condition_variable {
public:
	using native_handle_type = pthread_cond_t*;

	procshared_condition_variable( void );
	~procshared_condition_variable();

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

	std::cv_status wait_until( std::unique_lock<procshared_mutex>&          lock,
	                           const std::chrono::steady_clock::time_point& abs_time );

	template <class Predicate>
	bool wait_until( std::unique_lock<procshared_mutex>&          lock,
	                 const std::chrono::steady_clock::time_point& abs_time,
	                 Predicate                                    pred )
	{
		while ( !pred() ) {
			if ( wait_until( lock, abs_time ) == std::cv_status::timeout ) {
				return pred();
			}
		}
		return true;
	}

	template <class Rep, class Period>
	std::cv_status wait_for( std::unique_lock<procshared_mutex>&       lock,
	                         const std::chrono::duration<Rep, Period>& rel_time )
	{
		return wait_until( lock, std::chrono::steady_clock::now() + rel_time );
	}

	template <class Rep, class Period, class Predicate>
	bool wait_for( std::unique_lock<procshared_mutex>&       lock,
	               const std::chrono::duration<Rep, Period>& rel_time,
	               Predicate                                 pred )
	{
		return wait_until( lock, std::chrono::steady_clock::now() + rel_time, pred );
	}

	native_handle_type native_handle()
	{
		return &cond_steady_;
	}

private:
	procshared_condition_variable( const procshared_condition_variable& )            = delete;
	procshared_condition_variable& operator=( const procshared_condition_variable& ) = delete;

	pthread_cond_t cond_steady_;
	// pthread_cond_t cond_system_;
};

static_assert( std::is_standard_layout<procshared_condition_variable>::value, "procshared_condition_variable needs standard layout" );

#endif
