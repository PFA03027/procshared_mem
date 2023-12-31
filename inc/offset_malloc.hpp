/**
 * @file offset_malloc.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief malloc that is shareable b/w processes
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef OFFSET_MALLOC_HPP_
#define OFFSET_MALLOC_HPP_

#include <cstddef>
#include <memory>

class offset_malloc {
public:
	offset_malloc( void );
	~offset_malloc();
	offset_malloc( const offset_malloc& src )            = default;   // bind to memory allocator that has already setup
	offset_malloc( offset_malloc&& src )                 = default;   // bind to memory allocator that has already setup
	offset_malloc& operator=( const offset_malloc& src ) = default;   // bind to memory allocator that has already setup
	offset_malloc& operator=( offset_malloc&& src )      = default;   // bind to memory allocator that has already setup

	offset_malloc( void* p_mem, size_t mem_bytes );   // bind and setup memory allocator implementation.

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void*
		 allocate( size_t req_bytes, size_t alignment = alignof( std::max_align_t ) );
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

private:
	class offset_mem_malloc_impl;

	std::shared_ptr<offset_mem_malloc_impl> sp_impl_;
};

#endif   // OFFSET_MALLOC_HPP_