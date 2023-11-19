/**
 * @file offset_unique_ptr.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief Offset based unique pointer
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef OFFSET_UNIQUE_PTR_HPP
#define OFFSET_UNIQUE_PTR_HPP

#include <memory>
#include <type_traits>

#include "offset_ptr.hpp"

/**
 * @brief unique_ptr based on offset_ptr
 *
 * This is also primary template
 *
 * @tparam T element type
 * @tparam D deleter type
 */
template <class T, class D = std::default_delete<T>>
class offset_unique_ptr {
public:
	using pointer      = T*;
	using element_type = T;
	using deleter_type = D;

	constexpr offset_unique_ptr( void ) noexcept = default;
	constexpr explicit offset_unique_ptr( pointer p ) noexcept
	  : op_target_( p )
	  , deleter_()
	{
	}
	constexpr offset_unique_ptr( pointer p, const D& d1 ) noexcept
	  : op_target_( p )
	  , deleter_( d1 )
	{
	}
	constexpr offset_unique_ptr( pointer p, D&& d2 ) noexcept
	  : op_target_( p )
	  , deleter_( std::move( d2 ) )
	{
	}
	constexpr offset_unique_ptr( nullptr_t ) noexcept
	  : op_target_( nullptr )
	  , deleter_()
	{
	}

#if ( __cplusplus >= 202002L )
	template <class U = T, class E = D,
	          typename std::enable_if<std::is_nothrow_convertible<typename offset_unique_ptr<U, E>::pointer, pointer>::value &&
	                                  ( !std::is_array<U>::value ) &&
	                                  std::is_nothrow_convertible<E, D>::value>::type* = nullptr>
	offset_unique_ptr( offset_unique_ptr<U, E>&& u ) noexcept
	  : op_target_( u.release() )
	  , deleter_( u.deleter_ )
	{
	}
#else
	template <class U = T, class E = D,
	          typename std::enable_if<std::is_convertible<typename offset_unique_ptr<U, E>::pointer, pointer>::value &&
	                                  ( !std::is_array<U>::value ) &&
	                                  std::is_convertible<E, D>::value>::type* = nullptr>
	offset_unique_ptr( offset_unique_ptr<U, E>&& u )
	  : op_target_( u.release() )
	  , deleter_( u.deleter_ )
	{
	}
#endif

#if ( __cplusplus >= 202002L )
	constexpr
#endif
		~offset_unique_ptr()
	{
		reset();
	}
	constexpr offset_unique_ptr( const offset_unique_ptr& )  = delete;
	constexpr offset_unique_ptr( offset_unique_ptr&& )       = default;
	offset_unique_ptr& operator=( const offset_unique_ptr& ) = delete;
	offset_unique_ptr& operator=( offset_unique_ptr&& x )
	{
		if ( this != &x ) {
			reset();
			op_target_ = std::move( x.op_target_ );
			deleter_   = std::move( x.deleter_ );
		}
		return *this;
	}

	constexpr pointer release( void ) noexcept
	{
		pointer p  = op_target_.get();
		op_target_ = nullptr;
		return p;
	}

	constexpr void reset( pointer p = nullptr ) noexcept
	{
		pointer orig_p = op_target_.get();
		if ( orig_p != nullptr ) {
			deleter_( orig_p );
		}
		op_target_ = p;
	}

	constexpr void swap( offset_unique_ptr& x ) noexcept
	{
		op_target_.swap( x.op_target_ );
		deleter_type tmp_dl = deleter_;
		deleter_            = x.deleter_;
		x.deleter_          = tmp_dl;
	}

	constexpr pointer get() const noexcept
	{
		return op_target_.get();
	}

	constexpr deleter_type& get_deleter() noexcept
	{
		return deleter_;
	}

	constexpr const deleter_type& get_deleter() const noexcept
	{
		return deleter_;
	}

	constexpr explicit operator bool() const noexcept
	{
		return ( get() != nullptr );
	}

	constexpr typename std::add_lvalue_reference<element_type>::type operator*() const
	{
		return *get();
	}

	constexpr pointer operator->() const noexcept
	{
		return get();
	}

#if ( __cpp_impl_three_way_comparison > 201907L )
	template <class T2, class D2>
	constexpr bool operator==( const offset_unique_ptr<T2, D2>& c ) const noexcept
	{
		return ( get() == c.get() );
	}
	template <class T2, class D2>
	constexpr auto operator<=>( const offset_unique_ptr<T2, D2>& c ) const noexcept -> std::strong_ordering
	{
		return ( get() <=> c.get() );
	}
	constexpr bool operator==( nullptr_t ) const noexcept
	{
		return ( get() == nullptr );
	}
	constexpr auto operator<=>( nullptr_t ) const noexcept -> std::strong_ordering
	{
		return ( get() <=> nullptr );
	}

#else
#endif   // __cpp_impl_three_way_comparison

private:
	offset_ptr<T> op_target_;
	D             deleter_;
};

/**
 * @brief offset based unique pointer for array
 *
 * @tparam T
 * @tparam D
 */
template <class T, class D>
class offset_unique_ptr<T[], D> {
public:
	using pointer      = T*;
	using element_type = T;
	using deleter_type = D;

	constexpr offset_unique_ptr( void ) noexcept = default;
	constexpr explicit offset_unique_ptr( pointer p ) noexcept
	  : op_target_( p )
	  , deleter_()
	{
	}
	constexpr offset_unique_ptr( pointer p, const D& d1 ) noexcept
	  : op_target_( p )
	  , deleter_( d1 )
	{
	}
	constexpr offset_unique_ptr( pointer p, D&& d2 ) noexcept
	  : op_target_( p )
	  , deleter_( std::move( d2 ) )
	{
	}
	constexpr offset_unique_ptr( nullptr_t ) noexcept
	  : op_target_( nullptr )
	  , deleter_()
	{
	}

#if ( __cplusplus >= 202002L )
	template <class U = T, class E = D,
	          typename std::enable_if<std::is_nothrow_convertible<typename offset_unique_ptr<U, E>::pointer, pointer>::value &&
	                                  ( !std::is_array<U>::value ) &&
	                                  std::is_nothrow_convertible<E, D>::value>::type* = nullptr>
	offset_unique_ptr( offset_unique_ptr<U, E>&& u ) noexcept
	  : op_target_( u.release() )
	  , deleter_( u.deleter_ )
	{
	}
#else
	template <class U = T, class E = D,
	          typename std::enable_if<std::is_convertible<typename offset_unique_ptr<U, E>::pointer, pointer>::value &&
	                                  ( !std::is_array<U>::value ) &&
	                                  std::is_convertible<E, D>::value>::type* = nullptr>
	offset_unique_ptr( offset_unique_ptr<U, E>&& u )
	  : op_target_( u.release() )
	  , deleter_( u.deleter_ )
	{
	}
#endif

#if ( __cplusplus >= 202002L )
	constexpr
#endif
		~offset_unique_ptr()
	{
		reset();
	}
	constexpr offset_unique_ptr( const offset_unique_ptr& )  = delete;
	constexpr offset_unique_ptr( offset_unique_ptr&& )       = default;
	offset_unique_ptr& operator=( const offset_unique_ptr& ) = delete;
	offset_unique_ptr& operator=( offset_unique_ptr&& x )
	{
		if ( this != &x ) {
			reset();
			op_target_ = std::move( x.op_target_ );
			deleter_   = std::move( x.deleter_ );
		}
		return *this;
	}

	constexpr pointer release( void ) noexcept
	{
		pointer p  = op_target_.get();
		op_target_ = nullptr;
		return p;
	}

	constexpr void reset( pointer p = nullptr ) noexcept
	{
		pointer orig_p = op_target_.get();
		if ( orig_p != nullptr ) {
			deleter_( orig_p );
		}
		op_target_ = p;
	}

	constexpr void swap( offset_unique_ptr& x ) noexcept
	{
		op_target_.swap( x.op_target_ );
		deleter_type tmp_dl = deleter_;
		deleter_            = x.deleter_;
		x.deleter_          = tmp_dl;
	}

	constexpr pointer get() const noexcept
	{
		return op_target_.get();
	}

	constexpr deleter_type& get_deleter() noexcept
	{
		return deleter_;
	}

	constexpr const deleter_type& get_deleter() const noexcept
	{
		return deleter_;
	}

	constexpr explicit operator bool() const noexcept
	{
		return ( get() != nullptr );
	}

	constexpr typename std::add_lvalue_reference<element_type>::type operator*() const
	{
		return *get();
	}

	constexpr pointer operator->() const noexcept
	{
		return get();
	}

	constexpr element_type& operator[]( size_t idx )
	{
		return op_target_[idx];
	}

#if ( __cpp_impl_three_way_comparison > 201907L )
	template <class T2, class D2>
	constexpr bool operator==( const offset_unique_ptr<T2, D2>& c ) const noexcept
	{
		return ( get() == c.get() );
	}
	template <class T2, class D2>
	constexpr auto operator<=>( const offset_unique_ptr<T2, D2>& c ) const noexcept -> std::strong_ordering
	{
		return ( get() <=> c.get() );
	}
	constexpr bool operator==( nullptr_t ) const noexcept
	{
		return ( get() == nullptr );
	}
	constexpr auto operator<=>( nullptr_t ) const noexcept -> std::strong_ordering
	{
		return ( get() <=> nullptr );
	}

#else
#endif   // __cpp_impl_three_way_comparison

private:
	offset_ptr<T> op_target_;
	D             deleter_;
};

#if ( __cpp_impl_three_way_comparison > 201907L )
#else

template <class T1, class D1, class T2, class D2>
constexpr bool operator==( const offset_unique_ptr<T1, D1>& a, const offset_unique_ptr<T2, D2>& b )
{
	return ( a.get() == b.get() );
}

template <class T, class D>
constexpr bool operator==( const offset_unique_ptr<T, D>& x, nullptr_t ) noexcept
{
	return ( x.get() == nullptr );
}

template <class T, class D>
constexpr bool operator==( nullptr_t, const offset_unique_ptr<T, D>& x ) noexcept
{
	return ( x.get() == nullptr );
}

template <class T1, class D1, class T2, class D2>
constexpr bool operator!=( const offset_unique_ptr<T1, D1>& a, const offset_unique_ptr<T2, D2>& b )
{
	return !( a.get() == b.get() );
}

template <class T, class D>
constexpr bool operator!=( const offset_unique_ptr<T, D>& x, nullptr_t ) noexcept
{
	return !( x.get() == nullptr );
}

template <class T, class D>
constexpr bool operator!=( nullptr_t, const offset_unique_ptr<T, D>& x ) noexcept
{
	return !( x.get() == nullptr );
}

template <class T1, class D1, class T2, class D2>
bool operator<( const offset_unique_ptr<T1, D1>& a, const offset_unique_ptr<T2, D2>& b )
{
	using CT = typename std::common_type<typename offset_unique_ptr<T1, D1>::pointer, typename offset_unique_ptr<T2, D2>::pointer>::type;
	CT ap    = static_cast<CT>( a.get() );
	CT bp    = static_cast<CT>( b.get() );
	return ( ap < bp );
}

template <class T, class D>
constexpr bool operator<( const offset_unique_ptr<T, D>& x, nullptr_t )
{
	return ( std::less<typename offset_unique_ptr<T, D>::pointer>()( x.get(), nullptr ) );
}

template <class T, class D>
constexpr bool operator<( nullptr_t, const offset_unique_ptr<T, D>& x )
{
	return ( std::less<typename offset_unique_ptr<T, D>::pointer>()( nullptr, x.get() ) );
}

template <class T1, class D1, class T2, class D2>
bool operator<=( const offset_unique_ptr<T1, D1>& a, const offset_unique_ptr<T2, D2>& b )
{
	return !( b < a );
}

template <class T, class D>
constexpr bool operator<=( const offset_unique_ptr<T, D>& x, nullptr_t )
{
	return !( nullptr < x );
}

template <class T, class D>
constexpr bool operator<=( nullptr_t, const offset_unique_ptr<T, D>& x )
{
	return !( x < nullptr );
}

template <class T1, class D1, class T2, class D2>
bool operator>( const offset_unique_ptr<T1, D1>& a, const offset_unique_ptr<T2, D2>& b )
{
	return ( b < a );
}

template <class T, class D>
constexpr bool operator>( const offset_unique_ptr<T, D>& x, nullptr_t )
{
	return ( nullptr < x );
}

template <class T, class D>
constexpr bool operator>( nullptr_t, const offset_unique_ptr<T, D>& x )
{
	return ( x < nullptr );
}

template <class T1, class D1, class T2, class D2>
bool operator>=( const offset_unique_ptr<T1, D1>& a, const offset_unique_ptr<T2, D2>& b )
{
	return !( a < b );
}

template <class T, class D>
constexpr bool operator>=( const offset_unique_ptr<T, D>& x, nullptr_t )
{
	return !( x < nullptr );
}

template <class T, class D>
constexpr bool operator>=( nullptr_t, const offset_unique_ptr<T, D>& x )
{
	return !( nullptr < x );
}

#endif   // __cpp_impl_three_way_comparison

template <class T, class D>
constexpr void swap( offset_unique_ptr<T, D>& a, offset_unique_ptr<T, D>& b ) noexcept
{
	a.swap( b );
}

template <class T, class... Args>
constexpr offset_unique_ptr<T> make_offset_based_unique( Args&&... args )
{
	return offset_unique_ptr<T>( new T( args... ) );
}

#endif   // OFFSET_BASED_UNIQUE_PTR_HPP