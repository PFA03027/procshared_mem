/**
 * @file procshared_allocator.hpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-10-21
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef PROCSHARED_ALLOCATOR_HPP
#define PROCSHARED_ALLOCATOR_HPP

#include <exception>
#include <memory>
#include <type_traits>

#include "offset_malloc.hpp"

/**
 * @brief allocator that is inter-process sharable memory
 *
 * @tparam T type to allocate
 */
template <typename T>
class procshared_allocator {
public:
	using value_type                             = T;
	using propagate_on_container_move_assignment = std::true_type;
	using size_type                              = size_t;
	using difference_type                        = ptrdiff_t;
	using is_always_equal                        = std::false_type;

	constexpr procshared_allocator() noexcept
	  : p_malloc_( nullptr )
	  , sp_malloc_( nullptr )
	{
	}

	constexpr explicit procshared_allocator( offset_malloc* p ) noexcept
	  : p_malloc_( p )
	  , sp_malloc_( nullptr )
	{
	}

	constexpr explicit procshared_allocator( const std::shared_ptr<offset_malloc>& sp ) noexcept
	  : p_malloc_( sp.get() )
	  , sp_malloc_( sp )
	{
	}

	constexpr procshared_allocator( const procshared_allocator& ) noexcept = default;

	template <class U>
	constexpr procshared_allocator( const procshared_allocator<U>& orig ) noexcept
	  : p_malloc_( orig.p_malloc_ )
	  , sp_malloc_( orig.sp_malloc_ )
	{
	}

	constexpr procshared_allocator& operator=( const procshared_allocator& ) = default;

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	constexpr value_type*
	allocate( size_type n )
	{
		if ( p_malloc_ == nullptr ) {
			throw std::bad_cast();
		}

		return p_malloc_->allocate( sizeof( value_type[n] ) );
	}

	constexpr void deallocate( value_type* p, size_type n )
	{
		if ( p_malloc_ == nullptr ) {
			// TODO: メモリーリークになるが、とりあえず何もしない。例外を投げるとか(bad_free?)、エラーログを出力するとかできるけど。
			return;
		}
		p_malloc_->deallocate( p );
	}

private:
	offset_malloc*                 p_malloc_;
	std::shared_ptr<offset_malloc> sp_malloc_;
};

template <class T, class U>
constexpr bool operator==( const procshared_allocator<T>& a, const procshared_allocator<U>& b ) noexcept
{
	return ( a.p_malloc_ == b.p_malloc_ );
}
template <class T, class U>
constexpr bool operator!=( const procshared_allocator<T>& a, const procshared_allocator<U>& b ) noexcept
{
	return ( a.p_malloc_ != b.p_malloc_ );
}

#endif   // PROCSHARED_ALLOCATOR_HPP