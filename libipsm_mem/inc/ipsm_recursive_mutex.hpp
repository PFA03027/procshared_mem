/**
 * @file ipsm_recursive_mutex.hpp
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

#ifndef IPSM_RECURSIVE_MUTEX_HPP_
#define IPSM_RECURSIVE_MUTEX_HPP_

#include <type_traits>

#include <pthread.h>

#include "ipsm_mutex_base.hpp"

namespace ipsm {

/**
 * @brief mutex that is sharable b/w processes
 *
 */
class ipsm_recursive_mutex {
public:
	using native_handle_type = typename ipsm_mutex_base::native_handle_type;

	ipsm_recursive_mutex( void );
	~ipsm_recursive_mutex() = default;

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
	ipsm_recursive_mutex( const ipsm_recursive_mutex& )            = delete;
	ipsm_recursive_mutex& operator=( const ipsm_recursive_mutex& ) = delete;

	ipsm_mutex_base mtx_;
};

static_assert( std::is_standard_layout<ipsm_recursive_mutex>::value, "ipsm_recursive_mutex needs standard layout" );

}   // namespace ipsm

#endif   // IPSM_RECURSIVE_MUTEX_HPP_
