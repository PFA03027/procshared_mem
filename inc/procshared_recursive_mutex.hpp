/**
 * @file procshared_recursive_mutex.hpp
 * @author PFA03027@nifty.com
 * @brief recursive mutex that is sharable b/w processes
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 * @note
 * This class requires pthread library
 */

#ifndef PROCSHARED_RECURSIVE_MUTEX_HPP_
#define PROCSHARED_RECURSIVE_MUTEX_HPP_

#include <type_traits>

#include <pthread.h>

#include "procshared_mutex_base.hpp"

namespace ipsm {

/**
 * @brief mutex that is sharable b/w processes
 *
 */
class procshared_recursive_mutex {
public:
	using native_handle_type = typename procshared_mutex_base::native_handle_type;

	procshared_recursive_mutex( void );
	~procshared_recursive_mutex() = default;

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
	procshared_recursive_mutex( const procshared_recursive_mutex& )            = delete;
	procshared_recursive_mutex& operator=( const procshared_recursive_mutex& ) = delete;

	procshared_mutex_base mtx_;
};

static_assert( std::is_standard_layout<procshared_recursive_mutex>::value, "procshared_recursive_mutex needs standard layout" );

}   // namespace ipsm

#endif   // PROCSHARED_RECURSIVE_MUTEX_HPP_
