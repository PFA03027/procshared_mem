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
#include <stdexcept>

#include "offset_ptr.hpp"

namespace ipsm {

/**
 * @brief memory allocator from internal heap memory
 *
 * heap memory is managed by offset pointer. therefore this class is constructed on the shared memory and sharable with other processes.
 */
class offset_malloc {
public:
	class offset_malloc_impl;

	~offset_malloc();
	constexpr offset_malloc( void ) noexcept
	  : p_impl_( nullptr )
	{
	}
	offset_malloc( const offset_malloc& src );                  // bind to memory allocator that has already setup
	offset_malloc( offset_malloc&& src ) noexcept;              // bind to memory allocator that has already setup
	offset_malloc& operator=( const offset_malloc& src );       // bind to memory allocator that has already setup
	offset_malloc& operator=( offset_malloc&& src ) noexcept;   // bind to memory allocator that has already setup

	void swap( offset_malloc& src );

	explicit offset_malloc( void* p_mem, size_t mem_bytes );   // bind and setup memory allocator implementation. caution: this instance does not become not p_mem area owner.
	explicit offset_malloc( void* p_mem );                     // bind to memory that has already setup. caution: this instance does not become not p_mem area owner.

	/**
	 * @brief Allocate memory from internal heap memory
	 *
	 * @param n the number of bytes to allocate
	 * @param alignment the alignment of allocated memory. default value is alignof(std::max_align_t). if alignment is 0, it is treated as 1.
	 *
	 * @return void* pointer to allocated memory as the address of a process. if allocation failed, return nullptr.
	 *
	 * @note
	 * This interface is analogy to std::malloc();
	 */
#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void* allocate( size_t req_bytes, size_t alignment = alignof( std::max_align_t ) );

	/**
	 * @brief Deallocate memory from shared memory
	 *
	 * @param p pointer to the memory to deallocate
	 * @param alignment the alignment of the memory to deallocate. default value is alignof(std::max_align_t). if alignment is 0, it is treated as 1.
	 *
	 * @note
	 * This interface is analogy to std::free();
	 */
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

	/**
	 * @brief allocate and construct T instance.
	 *
	 * @tparam T allocating type
	 * @tparam Args template parameter pack of argument types to construct T
	 * @param args paramter pack of arguments to construct T
	 * @return T* pointer to a constructed instance
	 *
	 * @note
	 * This interface is analogy to non-array type new expression
	 */
	template <typename T, typename... Args>
	T* new_instance( Args&&... args )
	{
		void* p = reinterpret_cast<T*>( allocate( sizeof( T ), alignof( T ) ) );
		if ( p == nullptr ) {
			throw std::bad_alloc();
		}

		T* p_ans = nullptr;
		try {
			p_ans = new ( p ) T( std::forward<Args>( args )... );
		} catch ( ... ) {
			deallocate( p, alignof( T ) );
			throw;
		}
		return p_ans;
	}

	/**
	 * @brief destroy and deallocate T instance.
	 *
	 * @tparam T allocating type
	 * @param p pointer to an allocated instance that constructed by new_instance()
	 * @return T* pointer to a constructed instance
	 *
	 * @note
	 * This interface is analogy to non-array type delete expression
	 */
	template <typename T>
	void delete_instance( T* p )
	{
		p->~T();
		if ( !is_belong_to( p ) ) {
			// will be happened the memory leak
			return;
		}
		deallocate( p, alignof( T ) );
	}

	/**
	 * @brief allocate and construct T array
	 *
	 * @tparam T element type of array
	 * @param n number of elements
	 * @return T* pointer to the constructed array
	 *
	 * @note
	 * This interface is analogy to array type new expression
	 */
	template <typename T>
	T* new_array( size_t n )
	{
		constexpr size_t need_header_size = ( alignof( T ) > sizeof( size_t ) ) ? alignof( T ) : sizeof( size_t );
		size_t           need_bytes       = need_header_size + sizeof( T ) * n;
		void*            p                = allocate( need_bytes, need_header_size );
		if ( p == nullptr ) {
			throw std::bad_alloc();
		}
		size_t* p_size = reinterpret_cast<size_t*>( p );
		*p_size        = n;

		std::uintptr_t top_addr_of_T_array = reinterpret_cast<std::uintptr_t>( p ) + need_header_size;
		T*             p_top_of_T_array    = reinterpret_cast<T*>( top_addr_of_T_array );

		size_t i = 0;
		try {
			for ( i = 0; i < n; i++ ) {
				new ( &p_top_of_T_array[i] ) T();
			}
		} catch ( ... ) {
			for ( size_t j = i; j > 0; j-- ) {
				p_top_of_T_array[j - 1].~T();
			}
			throw;
		}
		return p_top_of_T_array;
	}

	/**
	 * @brief destroy and deallocate T array
	 *
	 * @tparam T element type of array
	 * @param p pointer to the array
	 *
	 * @note
	 * This interface is analogy to array type delete expression
	 */
	template <typename T>
	void delete_array( T* p )
	{
		constexpr size_t need_header_size      = ( alignof( T ) > sizeof( size_t ) ) ? alignof( T ) : sizeof( size_t );
		std::uintptr_t   top_addr_of_allocated = reinterpret_cast<std::uintptr_t>( p ) - need_header_size;
		size_t*          p_size                = reinterpret_cast<size_t*>( top_addr_of_allocated );
		for ( size_t i = 0; i < ( *p_size ); i++ ) {
			p[i].~T();
		}
		deallocate( p_size, need_header_size );
	}

	int  get_bind_count( void ) const;
	bool is_belong_to( void* p_mem ) const noexcept;

private:
	offset_ptr<offset_malloc_impl> p_impl_;

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

}   // namespace ipsm

#endif   // OFFSET_MALLOC_HPP_