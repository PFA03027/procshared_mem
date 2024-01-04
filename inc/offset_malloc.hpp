/**
 * @file offset_malloc.hpp
 * @author PFA03027@nifty.com
 * @brief malloc that is shareable b/w processes
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef OFFSET_MALLOC_HPP_
#define OFFSET_MALLOC_HPP_

#include <cstddef>

class offset_malloc {
public:
	class offset_malloc_impl;

	constexpr offset_malloc( void ) noexcept
	  : p_impl_( nullptr )
	{
	}
	~offset_malloc();
	offset_malloc( const offset_malloc& src );              // bind to memory allocator that has already setup
	offset_malloc( offset_malloc&& src );                   // bind to memory allocator that has already setup
	offset_malloc& operator=( const offset_malloc& src );   // bind to memory allocator that has already setup
	offset_malloc& operator=( offset_malloc&& src );        // bind to memory allocator that has already setup

	explicit offset_malloc( void* p_mem, size_t mem_bytes );   // bind and setup memory allocator implementation. caution: this instance does not become not p_mem area owner.
	explicit offset_malloc( void* p_mem );                     // bind to memory allocator that has already setup. caution: this instance does not become not p_mem area owner.

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void*
		 allocate( size_t req_bytes, size_t alignment = alignof( std::max_align_t ) );
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

	void swap( offset_malloc& src );

	int  get_bind_count( void ) const;
	bool is_belong_to( void* p_mem ) const;

private:
	offset_malloc_impl* p_impl_;

	friend constexpr bool operator==( const offset_malloc& a, const offset_malloc& b ) noexcept;
	friend constexpr bool operator!=( const offset_malloc& a, const offset_malloc& b ) noexcept;
};

constexpr bool operator==( const offset_malloc& a, const offset_malloc& b ) noexcept
{
	return ( a.p_impl_ == b.p_impl_ );
}
constexpr bool operator!=( const offset_malloc& a, const offset_malloc& b ) noexcept
{
	return ( a.p_impl_ != b.p_impl_ );
}

#endif   // OFFSET_MALLOC_HPP_