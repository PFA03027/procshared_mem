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

#endif   // IPSM_MUTEX_HPP_
