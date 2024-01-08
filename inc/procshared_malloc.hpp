/**
 * @file procshared_malloc.hpp
 * @author PFA03027@nifty.com
 * @brief memory allocator from procshared memory
 * @version 0.1
 * @date 2023-12-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef PROCSHARED_MALLOC_HPP_
#define PROCSHARED_MALLOC_HPP_

#include <cstddef>

#include "offset_allocator.hpp"
#include "offset_list.hpp"
#include "offset_malloc.hpp"
#include "procshared_condition_variable.hpp"
#include "procshared_mem.hpp"
#include "procshared_mutex.hpp"

struct msg_channel;

class procshared_malloc {
public:
	procshared_malloc( void );
	~procshared_malloc();
	procshared_malloc( procshared_malloc&& src ) = default;
	procshared_malloc& operator=( procshared_malloc&& src );

	/**
	 * @brief Construct and allocate a new cooperative startup shared memory object
	 *
	 * This constructor allocates a shared memory during constructor.
	 * If an instance got as a primary role, it calls a functor initfunctor_arg() after finish setup of a shared memory
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 * @param initfunctor_arg a functor to initialize a shared memory area that length is same to an argument 'length'
	 *
	 * @exception if failed creation by any reason, throw std::bad_alloc(in case of new operator throws) or std::run_time_error
	 *
	 * @note p_shm_name string AAA must follow POSIX semaphore name specifications. please refer sem_open or sem_overview
	 */
	procshared_malloc( const char* p_shm_name, const char* p_id_dirname, size_t length, mode_t mode );

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void*
	allocate( size_t n, size_t alignment = alignof( std::max_align_t ) );

	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

	int get_bind_count( void ) const;

	template <typename T>
	offset_allocator<T> get_allocator( void );

	void                send( unsigned int ch, offset_ptr<void> sending_value );   // 仮実装
	offset_ptr<void>    receive( unsigned int ch );                                // 仮実装
	static unsigned int channel_size( void );

private:
	procshared_malloc( const procshared_malloc& src )            = delete;
	procshared_malloc& operator=( const procshared_malloc& src ) = delete;

	void swap( procshared_malloc& src );

	procshared_mem shm_obj_;    //!< shared memory object. this member variable declaration order required like procshared_mem, then offset_malloc
	offset_malloc  shm_heap_;   //!< offset base memory allocator on shared memory. this member variable declaration order required like procshared_mem, then offset_malloc
	msg_channel*   p_msgch_;
};

////////////////////////////////////////////////////////////////////
// Implement
template <typename T>
offset_allocator<T> procshared_malloc::get_allocator( void )
{
	return offset_allocator<T>( shm_heap_ );   // NOLINT(clang-diagnostic-error)
}

#endif   // PROCSHARED_MALLOC_HPP_
