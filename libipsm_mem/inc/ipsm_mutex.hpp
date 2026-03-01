/**
 * @file ipsm_mutex.hpp
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

#ifndef IPSM_MUTEX_HPP_
#define IPSM_MUTEX_HPP_

#include <type_traits>

#include <pthread.h>

#include "ipsm_mutex_base.hpp"

namespace ipsm {

/**
 * @brief mutex that is sharable b/w processes
 *
 */
class ipsm_mutex {
public:
	using native_handle_type = typename ipsm_mutex_base::native_handle_type;

	ipsm_mutex( void );
	~ipsm_mutex() = default;

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
	ipsm_mutex( const ipsm_mutex& )            = delete;
	ipsm_mutex& operator=( const ipsm_mutex& ) = delete;

	ipsm_mutex_base mtx_;
};

static_assert( std::is_standard_layout<ipsm_mutex>::value, "ipsm_mutex should be standard layout" );

}   // namespace ipsm

#endif   // IPSM_MUTEX_HPP_
