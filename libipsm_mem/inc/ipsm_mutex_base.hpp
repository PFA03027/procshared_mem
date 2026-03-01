/**
 * @file ipsm_mutex_base.hpp
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

#ifndef IPSM_MUTEX_BASE_HPP_
#define IPSM_MUTEX_BASE_HPP_

#include <type_traits>

#include <pthread.h>

namespace ipsm {

/**
 * @brief mutex that is sharable b/w processes
 *
 */
class ipsm_mutex_base {
public:
	using native_handle_type = pthread_mutex_t*;

	ipsm_mutex_base( int kind );
	~ipsm_mutex_base();

	void lock( void );
	bool try_lock( void );
	void unlock( void );

	native_handle_type native_handle( void )
	{
		return &fastmutex_;
	}

private:
	ipsm_mutex_base( const ipsm_mutex_base& )            = delete;
	ipsm_mutex_base& operator=( const ipsm_mutex_base& ) = delete;

	pthread_mutex_t fastmutex_;
};

static_assert( std::is_standard_layout<ipsm_mutex_base>::value, "ipsm_mutex_base needs standard layout" );

}   // namespace ipsm

#endif   // IPSM_MUTEX_BASE_HPP_
