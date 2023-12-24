/**
 * @file offset_mem_allocator.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief malloc that is shareable b/w processes
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef OFFSET_MEM_ALLOCATOR_HPP_
#define OFFSET_MEM_ALLOCATOR_HPP_

#include <cstddef>

class offset_mem_allocator {
public:
	offset_mem_allocator( void );
	~offset_mem_allocator();
	offset_mem_allocator( offset_mem_allocator&& src );
	offset_mem_allocator& operator=( offset_mem_allocator&& src );

	offset_mem_allocator( void* p_mem, size_t mem_bytes );

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void*
		 allocate( size_t req_bytes, size_t alignment = alignof( std::max_align_t ) );
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

private:
	offset_mem_allocator( const offset_mem_allocator& src )            = delete;
	offset_mem_allocator& operator=( const offset_mem_allocator& src ) = delete;

	class offset_mem_malloc_impl;

	offset_mem_malloc_impl* p_impl_;
};

#endif   // OFFSET_MEM_ALLOCATOR_HPP_