/**
 * @file procshared_mutex.hpp
 * @author PFA03027@nifty.com
 * @brief mutex that is sharable b/w processes
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 * @note
 * This class requires pthread library
 */

#ifndef PROCSHARED_MUTEX_HPP_
#define PROCSHARED_MUTEX_HPP_

#include <type_traits>

#include <pthread.h>

#include "procshared_mutex_base.hpp"

/**
 * @brief mutex that is sharable b/w processes
 *
 */
class procshared_mutex {
public:
	using native_handle_type = typename procshared_mutex_base::native_handle_type;

	procshared_mutex( void );
	~procshared_mutex() = default;

	void lock( void )
	{
		mtx_.lock();
	}
	bool try_lock( void )
	{
		return mtx_.try_lock();
	}
	void unlock( void )
	{
		mtx_.unlock();
	}
	native_handle_type native_handle( void )
	{
		return mtx_.native_handle();
	}

private:
	procshared_mutex( const procshared_mutex& )            = delete;
	procshared_mutex& operator=( const procshared_mutex& ) = delete;

	procshared_mutex_base mtx_;
};

static_assert( std::is_standard_layout<procshared_mutex>::value, "procshared_mutex should be standard layout" );

#endif   // PROCSHARED_MUTEX_HPP_
