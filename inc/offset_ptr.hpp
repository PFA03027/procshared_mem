/**
 * @file offset_ptr.hpp
 * @author PFA03027@nifty.com
 * @brief offset base pointer
 * @version 0.1
 * @date 2023-08-19
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 * 作ってみたはよいが、じつはすでに boost::interprocess::offset_ptr というのがあった。thisポインタをうまく使うというコンセプトもまったく同じ。
 * まぁ、だれでも思いつきそうだしなぁ。誰かが作っていそうだとは思ってた。( 一一)
 *
 * 下記が、それ。う～ん、車輪の再発明かぁ。まぁ、しょうがない。
 * https://www.boost.org/doc/libs/1_36_0/doc/html/interprocess/offset_ptr.html
 *
 */

#ifndef OFFSET_PTR_HPP_
#define OFFSET_PTR_HPP_

#include <atomic>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

#if ( __cpp_impl_three_way_comparison > 201907L )
#include <compare>
#endif

namespace ipsm {

/**
 * @brief Offset base pointer
 *
 * @warning
 * nullptrを表現するために、offset値ゼロをnullptr扱いとする。これにより、アドレス空間の異なるプロセス間でnullptr相当の意味をもつポインタ情報の交換が可能になる。
 * ただし、このことによる制約として、自身を指すことができない、また自身とその前後で連続しないアドレスを指すため、真にcontiguous_iteratorとは言えなくなる。
 *
 * @note
 * This is primary template class. type "void" is specialized
 *
 * @tparam T
 */
template <typename T>
class offset_ptr {
public:
	using value_type        = T;
	using pointer           = value_type*;
	using reference         = value_type&;      // std::add_lvalue_reference<value_type>::type;
	using difference_type   = std::ptrdiff_t;   // using difference_type = decltype( std::declval<pointer>() - std::declval<pointer>() );
	using iterator_category = std::random_access_iterator_tag;

	constexpr offset_ptr( void ) noexcept
	  : offset_( calc_offset_as_nullptr( this ) )
	{
	}

	constexpr offset_ptr( pointer p ) noexcept
	  : offset_( calc_offset( this, p ) )
	{
	}

	constexpr offset_ptr( const offset_ptr& orig ) noexcept
	  : offset_( calc_offset( this, orig.calc_address() ) )
	{
	}

	constexpr offset_ptr( offset_ptr&& orig ) noexcept
	  : offset_( calc_offset( this, orig.calc_address() ) )
	{
		orig = nullptr;
	}

	offset_ptr& operator=( const offset_ptr& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		offset_ = calc_offset( this, orig.calc_address() );
		return *this;
	}

	offset_ptr& operator=( offset_ptr&& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		offset_ = calc_offset( this, orig.calc_address() );
		orig    = nullptr;
		return *this;
	}

	template <typename U = T,
	          typename std::enable_if<
				  !std::is_same<U, T>::value &&
				  std::is_convertible<typename offset_ptr<U>::pointer, pointer>::value>::type* = nullptr>
	constexpr offset_ptr( const offset_ptr<U>& orig ) noexcept
	  : offset_( calc_offset( this, orig.calc_address() ) )
	{
	}

	template <typename U = T,
	          typename std::enable_if<
				  !std::is_same<U, T>::value &&
				  std::is_convertible<typename offset_ptr<U>::pointer, pointer>::value>::type* = nullptr>
	constexpr offset_ptr( offset_ptr<U>&& orig ) noexcept
	  : offset_( calc_offset( this, orig.calc_address() ) )
	{
		orig = nullptr;
	}

	template <typename U = T,
	          typename std::enable_if<
				  !std::is_same<U, T>::value &&
				  std::is_convertible<typename offset_ptr<U>::pointer, pointer>::value>::type* = nullptr>
	offset_ptr& operator=( const offset_ptr<U>& orig ) noexcept
	{
		offset_ = calc_offset( this, orig.calc_address() );
		return *this;
	}

	template <typename U = T,
	          typename std::enable_if<
				  !std::is_same<U, T>::value &&
				  std::is_convertible<typename offset_ptr<U>::pointer, pointer>::value>::type* = nullptr>
	offset_ptr& operator=( offset_ptr<U>&& orig ) noexcept
	{
		offset_ = calc_offset( this, orig.calc_address() );
		orig    = nullptr;
		return *this;
	}

	constexpr offset_ptr( const std::nullptr_t& ) noexcept
	  : offset_( calc_offset_as_nullptr( this ) )
	{
	}

	offset_ptr& operator=( const std::nullptr_t& ) noexcept
	{
		offset_ = calc_offset_as_nullptr( this );
		return *this;
	}

	constexpr pointer get() const noexcept
	{
		return reinterpret_cast<pointer>( calc_address() );
	}

	constexpr void swap( offset_ptr& x ) noexcept
	{
		pointer p_tmp_addr_my = calc_address();
		pointer p_tmp_addr_x  = x.calc_address();
		x.offset_             = calc_offset( &x, p_tmp_addr_my );
		offset_               = calc_offset( this, p_tmp_addr_x );
	}

	constexpr uintptr_t get_offset() const noexcept
	{
		return offset_;
	}

	pointer operator->( void ) const noexcept
	{
		return get();
	}

	reference operator*( void ) const noexcept
	{
		return *( get() );
	}

	constexpr explicit operator bool() const noexcept
	{
		return ( get() != nullptr );
	}

	constexpr operator T*() const noexcept
	{
		return get();
	}

	constexpr reference operator[]( difference_type i ) const noexcept
	{
		return get()[i];
	}

	constexpr offset_ptr& operator++( void ) noexcept   // 前置インクリメント演算子
	{
		pointer p_t = get();
		++p_t;
		offset_ = calc_offset( this, p_t );
		return *this;
	}

	constexpr offset_ptr operator++( int ) noexcept   // 後置インクリメント演算子
	{
		offset_ptr ans( *this );
		++( *this );
		return ans;
	}

	constexpr offset_ptr& operator--( void ) noexcept   // 前置デクリメント演算子
	{
		pointer p_t = get();
		--p_t;
		offset_ = calc_offset( this, p_t );
		return *this;
	}

	constexpr offset_ptr operator--( int ) noexcept   // 後置デクリメント演算子
	{
		offset_ptr ans( *this );
		--( *this );
		return ans;
	}

	constexpr offset_ptr& operator+=( difference_type d ) noexcept
	{
		pointer p_t = get();
		p_t += d;
		offset_ = calc_offset( this, p_t );
		return *this;
	}

	constexpr offset_ptr& operator-=( difference_type d ) noexcept
	{
		pointer p_t = get();
		p_t -= d;
		offset_ = calc_offset( this, p_t );
		return *this;
	}

	template <typename U>
	friend offset_ptr<U> operator+( const offset_ptr<U>& a, typename offset_ptr<U>::difference_type d ) noexcept;
	template <typename U>
	friend offset_ptr<U> operator+( typename offset_ptr<U>::difference_type d, const offset_ptr<U>& a ) noexcept;
	template <typename U>
	friend offset_ptr<U> operator-( const offset_ptr<U>& a, typename offset_ptr<U>::difference_type d ) noexcept;
	template <typename U>
	friend typename offset_ptr<U>::difference_type operator-( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;

#if ( __cpp_impl_three_way_comparison > 201907L )
	constexpr bool operator==( const offset_ptr& c ) const noexcept
	{
		return ( calc_address() == c.calc_address() );
	}
	constexpr auto operator<=>( const offset_ptr& c ) const noexcept -> std::strong_ordering
	{
		return ( calc_address() <=> c.calc_address() );
	}
	constexpr bool operator==( const std::nullptr_t& c ) const noexcept
	{
		return ( calc_address() == nullptr );
	}
	constexpr auto operator<=>( const std::nullptr_t& c ) const noexcept -> std::strong_ordering
	{
		return ( calc_address() <=> nullptr );
	}

#else
	template <typename U>
	friend constexpr bool operator==( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator!=( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<=( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>=( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;

	template <typename U>
	friend constexpr bool operator==( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator!=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;

	template <typename U>
	friend constexpr bool operator==( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator!=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;

#endif

private:
	inline constexpr pointer calc_address( void ) const noexcept
	{
		if ( offset_ == 0 ) {
			return nullptr;
		} else {
			return reinterpret_cast<pointer>( reinterpret_cast<uintptr_t>( this ) + offset_ );
		}
	}
	static inline constexpr uintptr_t calc_offset( const offset_ptr* base_p, T* p ) noexcept
	{
		if ( p == nullptr ) {
			return 0;
		} else {
			return reinterpret_cast<uintptr_t>( p ) - reinterpret_cast<uintptr_t>( base_p );
		}
	}
	static inline constexpr uintptr_t calc_offset_as_nullptr( const offset_ptr* base_p ) noexcept
	{
		return 0;
		// return ( -reinterpret_cast<uintptr_t>( base_p ) );
	}

	uintptr_t offset_;

	template <typename U>
	friend class offset_ptr;
};

/////////////////////////////// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/**
 * @brief Offset base pointer of void
 *
 * @warning
 * nullptrを表現するために、offset値ゼロをnullptr扱いとする。これにより、アドレス空間の異なるプロセス間でnullptr相当の意味をもつポインタ情報の交換が可能になる。
 * ただし、このことによる制約として、自身を指すことができない、また自身とその前後で連続しないアドレスを指すため、真にcontiguous_iteratorとは言えなくなる。
 *
 */
template <>
class offset_ptr<void> {
public:
	using value_type      = void;
	using pointer         = void*;
	using difference_type = std::ptrdiff_t;   // using difference_type = decltype( std::declval<pointer>() - std::declval<pointer>() );
	// using iterator_category = std::random_access_iterator_tag;

	constexpr offset_ptr( void ) noexcept
	  : offset_( calc_offset_as_nullptr( this ) )
	{
	}

	constexpr offset_ptr( pointer p ) noexcept
	  : offset_( calc_offset( this, p ) )
	{
	}

	constexpr offset_ptr( const offset_ptr& orig ) noexcept
	  : offset_( calc_offset( this, orig.calc_address() ) )
	{
	}

	constexpr offset_ptr( offset_ptr&& orig ) noexcept
	  : offset_( calc_offset( this, orig.calc_address() ) )
	{
		orig = nullptr;
	}

	offset_ptr& operator=( const offset_ptr& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		offset_ = calc_offset( this, orig.calc_address() );
		return *this;
	}

	offset_ptr& operator=( offset_ptr&& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		offset_ = calc_offset( this, orig.calc_address() );
		orig    = nullptr;
		return *this;
	}

	template <typename U = void,
	          typename std::enable_if<
				  !std::is_same<U, void>::value &&
				  std::is_convertible<typename offset_ptr<U>::pointer, pointer>::value>::type* = nullptr>
	constexpr offset_ptr( const offset_ptr<U>& orig ) noexcept
	  : offset_( calc_offset( this, orig.calc_address() ) )
	{
	}

	template <typename U = void,
	          typename std::enable_if<
				  !std::is_same<U, void>::value &&
				  std::is_convertible<typename offset_ptr<U>::pointer, pointer>::value>::type* = nullptr>
	constexpr offset_ptr( offset_ptr<U>&& orig ) noexcept
	  : offset_( calc_offset( this, orig.calc_address() ) )
	{
		orig = nullptr;
	}

	template <typename U = void,
	          typename std::enable_if<
				  !std::is_same<U, void>::value &&
				  std::is_convertible<typename offset_ptr<U>::pointer, pointer>::value>::type* = nullptr>
	offset_ptr& operator=( const offset_ptr<U>& orig ) noexcept
	{
		offset_ = calc_offset( this, orig.calc_address() );
		return *this;
	}

	template <typename U = void,
	          typename std::enable_if<
				  !std::is_same<U, void>::value &&
				  std::is_convertible<typename offset_ptr<U>::pointer, pointer>::value>::type* = nullptr>
	offset_ptr& operator=( offset_ptr<U>&& orig ) noexcept
	{
		offset_ = calc_offset( this, orig.calc_address() );
		orig    = nullptr;
		return *this;
	}

	constexpr offset_ptr( const std::nullptr_t& ) noexcept
	  : offset_( calc_offset_as_nullptr( this ) )
	{
	}

	offset_ptr& operator=( const std::nullptr_t& ) noexcept
	{
		offset_ = calc_offset_as_nullptr( this );
		return *this;
	}

	constexpr pointer get() const noexcept
	{
		return static_cast<pointer>( calc_address() );
	}

	constexpr void swap( offset_ptr& x ) noexcept
	{
		pointer p_tmp_addr_my = calc_address();
		pointer p_tmp_addr_x  = x.calc_address();
		x.offset_             = calc_offset( &x, p_tmp_addr_my );
		offset_               = calc_offset( this, p_tmp_addr_x );
	}

	constexpr uintptr_t get_offset() const noexcept
	{
		return offset_;
	}

	constexpr explicit operator bool() const noexcept
	{
		return ( get() != nullptr );
	}

	operator void*() const noexcept
	{
		return reinterpret_cast<void*>( calc_address() );
	}

#if ( __cpp_impl_three_way_comparison > 201907L )
	constexpr bool operator==( const offset_ptr& c ) const noexcept
	{
		return ( calc_address() == c.calc_address() );
	}
	constexpr auto operator<=>( const offset_ptr& c ) const noexcept -> std::strong_ordering
	{
		return ( calc_address() <=> c.calc_address() );
	}
	constexpr bool operator==( const std::nullptr_t& c ) const noexcept
	{
		return ( calc_address() == nullptr );
	}
	constexpr auto operator<=>( const std::nullptr_t& c ) const noexcept -> std::strong_ordering
	{
		return ( calc_address() <=> nullptr );
	}

#else
	template <typename U>
	friend constexpr bool operator==( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator!=( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<=( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>=( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept;

	template <typename U>
	friend constexpr bool operator==( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator!=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept;

	template <typename U>
	friend constexpr bool operator==( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator!=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator<=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;
	template <typename U>
	friend constexpr bool operator>=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept;

#endif

private:
	inline constexpr pointer calc_address( void ) const noexcept
	{
		if ( offset_ == 0 ) {
			return nullptr;
		} else {
			return reinterpret_cast<pointer>( reinterpret_cast<uintptr_t>( this ) + offset_ );
		}
	}
	static inline constexpr uintptr_t calc_offset( const offset_ptr* base_p, void* p ) noexcept
	{
		if ( p == nullptr ) {
			return 0;
		} else {
			return reinterpret_cast<uintptr_t>( p ) - reinterpret_cast<uintptr_t>( base_p );
		}
	}
	static inline constexpr uintptr_t calc_offset_as_nullptr( const offset_ptr* base_p ) noexcept
	{
		return 0;
		// return ( -reinterpret_cast<uintptr_t>( base_p ) );
	}

	uintptr_t offset_;

	template <typename U>
	friend class offset_ptr;
};

#if ( __cpp_impl_three_way_comparison > 201907L )
#else
template <typename T>
constexpr bool operator==( const offset_ptr<T>& a, const offset_ptr<T>& b ) noexcept
{
	return ( a.calc_address() == b.calc_address() );
}

template <typename T>
constexpr bool operator!=( const offset_ptr<T>& a, const offset_ptr<T>& b ) noexcept
{
	return !( a == b );
}

template <typename T>
constexpr bool operator<( const offset_ptr<T>& a, const offset_ptr<T>& b ) noexcept
{
	return ( a.calc_address() < b.calc_address() );
}

template <typename T>
constexpr bool operator<=( const offset_ptr<T>& a, const offset_ptr<T>& b ) noexcept
{
	return ( a < b ) || ( a == b );
}

template <typename T>
constexpr bool operator>( const offset_ptr<T>& a, const offset_ptr<T>& b ) noexcept
{
	return ( b < a );
}

template <typename T>
constexpr bool operator>=( const offset_ptr<T>& a, const offset_ptr<T>& b ) noexcept
{
	return ( b < a ) || ( a == b );
}

////////////////////////

template <typename U>
constexpr bool operator==( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept
{
	return ( a == offset_ptr<U>( b ) );
}
template <typename U>
constexpr bool operator!=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept
{
	return ( a != offset_ptr<U>( b ) );
}
template <typename U>
constexpr bool operator<( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept
{
	return ( a < offset_ptr<U>( b ) );
}
template <typename U>
constexpr bool operator<=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept
{
	return ( a <= offset_ptr<U>( b ) );
}
template <typename U>
constexpr bool operator>( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept
{
	return ( a > offset_ptr<U>( b ) );
}
template <typename U>
constexpr bool operator>=( const offset_ptr<U>& a, const std::nullptr_t& b ) noexcept
{
	return ( a >= offset_ptr<U>( b ) );
}

template <typename U>
constexpr bool operator==( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept
{
	return ( offset_ptr<U>( a ) == b );
}
template <typename U>
constexpr bool operator!=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept
{
	return ( offset_ptr<U>( a ) != b );
}
template <typename U>
constexpr bool operator<( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept
{
	return ( offset_ptr<U>( a ) < b );
}
template <typename U>
constexpr bool operator<=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept
{
	return ( offset_ptr<U>( a ) <= b );
}
template <typename U>
constexpr bool operator>( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept
{
	return ( offset_ptr<U>( a ) > b );
}
template <typename U>
constexpr bool operator>=( const std::nullptr_t& a, const offset_ptr<U>& b ) noexcept
{
	return ( offset_ptr<U>( a ) >= b );
}

////////////////////////

#endif

template <typename T>
constexpr void swap( offset_ptr<T>& a, offset_ptr<T>& b ) noexcept
{
	a.swap( b );
}

template <typename U>
offset_ptr<U> operator+( const offset_ptr<U>& a, typename offset_ptr<U>::difference_type d ) noexcept
{
	offset_ptr<U> ans( a );
	ans += d;
	return ans;
}

template <typename U>
offset_ptr<U> operator+( typename offset_ptr<U>::difference_type d, const offset_ptr<U>& a ) noexcept
{
	offset_ptr<U> ans( a );
	ans += d;
	return ans;
}

template <typename U>
offset_ptr<U> operator-( const offset_ptr<U>& a, typename offset_ptr<U>::difference_type d ) noexcept
{
	offset_ptr<U> ans( a );
	ans -= d;
	return ans;
}

template <typename U>
typename offset_ptr<U>::difference_type operator-( const offset_ptr<U>& a, const offset_ptr<U>& b ) noexcept
{
	return a.get() - b.get();
}

// 本当は、std::atomic<T>の特殊化として定義するべきだと思われるが、さすがにそれは今ではないので、独立して定義する。
template <typename T>
class atomic_offset_ptr {
public:
	using value_type      = offset_ptr<T>;
	using difference_type = typename value_type::difference_type;

	constexpr atomic_offset_ptr( void ) noexcept   // (1) C++20
	  : at_offset_( calc_offset_as_nullptr( this ) )
	{
	}

	constexpr atomic_offset_ptr( const T* p_desired ) noexcept
	  : at_offset_( calc_offset( this, p_desired ) )
	{
	}

	constexpr atomic_offset_ptr( const value_type& desired ) noexcept
	  : at_offset_( calc_offset( this, desired.get() ) )
	{
	}

	atomic_offset_ptr( const atomic_offset_ptr& ) = delete;   // (3) C++11
	atomic_offset_ptr( atomic_offset_ptr&& )      = delete;   // (4) C++11

	~atomic_offset_ptr() = default;

	atomic_offset_ptr& operator=( const atomic_offset_ptr& )          = delete;   // (1) C++11
	atomic_offset_ptr& operator=( const atomic_offset_ptr& ) volatile = delete;   // (2) C++11

	value_type operator=( const value_type& desired ) volatile noexcept   // (3) C++11
	{
		store( desired );
		return desired;
	}
	value_type operator=( const value_type& desired ) noexcept   // (4) C++11
	{
		store( desired );
		return desired;
	}

	bool is_lock_free() const volatile noexcept
	{
		return at_offset_.is_lock_free();
	}
	bool is_lock_free() const noexcept
	{
		return at_offset_.is_lock_free();
	}

	void store( const value_type& desired, std::memory_order order = std::memory_order_seq_cst ) volatile noexcept   // (1) C++11
	{
		at_offset_.store( calc_offset( this, desired.get() ), order );
	}
	void store( const value_type& desired, std::memory_order order = std::memory_order_seq_cst ) noexcept   // (2) C++11
	{
		at_offset_.store( calc_offset( this, desired.get() ), order );
	}

	value_type load( std::memory_order order = std::memory_order_seq_cst ) const volatile noexcept   // (1) C++11
	{
		element_pointer p_cur_addr = calc_address( at_offset_.load( order ) );
		return value_type( p_cur_addr );
	}
	value_type load( std::memory_order order = std::memory_order_seq_cst ) const noexcept   // (2) C++11
	{
		element_pointer p_cur_addr = calc_address( at_offset_.load( order ) );
		return value_type( p_cur_addr );
	}

	operator value_type() const volatile noexcept   // (1) C++11
	{
		return load();
	}
	operator value_type() const noexcept   // (2) C++11
	{
		return load();
	}

	value_type exchange( const value_type& desired, std::memory_order order = std::memory_order_seq_cst ) volatile noexcept   // (1) C++11
	{
		uintptr_t desired_offset = calc_offset( this, desired.get() );
		uintptr_t old_offset     = at_offset_.exchange( desired_offset, order );
		return value_type( calc_address( old_offset ) );
	}
	value_type exchange( const value_type& desired, std::memory_order order = std::memory_order_seq_cst ) noexcept   // (2) C++11
	{
		uintptr_t desired_offset = calc_offset( this, desired.get() );
		uintptr_t old_offset     = at_offset_.exchange( desired_offset, order );
		return value_type( calc_address( old_offset ) );
	}

	bool compare_exchange_weak( value_type&       expected,
	                            const value_type& desired,
	                            std::memory_order success,
	                            std::memory_order failure ) volatile noexcept   // (1) C++11
	{
		uintptr_t expected_offset = calc_offset( this, expected.get() );
		uintptr_t desired_offset  = calc_offset( this, desired.get() );
		bool      ans             = compare_exchange_weak( expected_offset, desired_offset, success, failure );
		expected                  = calc_address( expected_offset );
		return ans;
	}
	bool compare_exchange_weak( value_type&       expected,
	                            const value_type& desired,
	                            std::memory_order success,
	                            std::memory_order failure ) noexcept   // (2) C++11
	{
		uintptr_t expected_offset = calc_offset( this, expected.get() );
		uintptr_t desired_offset  = calc_offset( this, desired.get() );
		bool      ans             = compare_exchange_weak( expected_offset, desired_offset, success, failure );
		expected                  = calc_address( expected_offset );
		return ans;
	}

	bool compare_exchange_weak( value_type&       expected,
	                            const value_type& desired,
	                            std::memory_order order = std::memory_order_seq_cst ) volatile noexcept   // (3) C++11
	{
		uintptr_t expected_offset = calc_offset( this, expected.get() );
		uintptr_t desired_offset  = calc_offset( this, desired.get() );
		bool      ans             = compare_exchange_weak( expected_offset, desired_offset, order );
		expected                  = calc_address( expected_offset );
		return ans;
	}
	bool compare_exchange_weak( value_type&       expected,
	                            const value_type& desired,
	                            std::memory_order order = std::memory_order_seq_cst ) noexcept   // (4) C++11
	{
		uintptr_t expected_offset = calc_offset( this, expected.get() );
		uintptr_t desired_offset  = calc_offset( this, desired.get() );
		bool      ans             = compare_exchange_weak( expected_offset, desired_offset, order );
		expected                  = calc_address( expected_offset );
		return ans;
	}

	bool compare_exchange_strong( value_type&       expected,
	                              const value_type& desired,
	                              std::memory_order success,
	                              std::memory_order failure ) volatile noexcept   // (1) C++11
	{
		uintptr_t expected_offset = calc_offset( this, expected.get() );
		uintptr_t desired_offset  = calc_offset( this, desired.get() );
		bool      ans             = compare_exchange_strong( expected_offset, desired_offset, success, failure );
		expected                  = calc_address( expected_offset );
		return ans;
	}
	bool compare_exchange_strong( value_type&       expected,
	                              const value_type& desired,
	                              std::memory_order success,
	                              std::memory_order failure ) noexcept   // (2) C++11
	{
		uintptr_t expected_offset = calc_offset( this, expected.get() );
		uintptr_t desired_offset  = calc_offset( this, desired.get() );
		bool      ans             = compare_exchange_strong( expected_offset, desired_offset, success, failure );
		expected                  = calc_address( expected_offset );
		return ans;
	}

	bool compare_exchange_strong( value_type&       expected,
	                              const value_type& desired,
	                              std::memory_order order = std::memory_order_seq_cst ) volatile noexcept   // (3) C++11
	{
		uintptr_t expected_offset = calc_offset( this, expected.get() );
		uintptr_t desired_offset  = calc_offset( this, desired.get() );
		bool      ans             = compare_exchange_strong( expected_offset, desired_offset, order );
		expected                  = calc_address( expected_offset );
		return ans;
	}
	bool compare_exchange_strong( value_type&       expected,
	                              const value_type& desired,
	                              std::memory_order order = std::memory_order_seq_cst ) noexcept   // (4) C++11
	{
		uintptr_t expected_offset = calc_offset( this, expected.get() );
		uintptr_t desired_offset  = calc_offset( this, desired.get() );
		bool      ans             = compare_exchange_strong( expected_offset, desired_offset, order );
		expected                  = calc_address( expected_offset );
		return ans;
	}

	value_type fetch_add( difference_type operand, std::memory_order order = std::memory_order_seq_cst ) volatile noexcept   // (1) C++11
	{
		uintptr_t diff       = calc_addr_diff( operand );
		uintptr_t old_offset = at_offset_.fetch_add( diff, order );
		return value_type( calc_address( old_offset ) );
	}
	value_type fetch_add( difference_type operand, std::memory_order order = std::memory_order_seq_cst ) noexcept   // (2) C++11
	{
		uintptr_t diff       = calc_addr_diff( operand );
		uintptr_t old_offset = at_offset_.fetch_add( diff, order );
		return value_type( calc_address( old_offset ) );
	}

	value_type fetch_sub( difference_type operand, std::memory_order order = std::memory_order_seq_cst ) volatile noexcept   // (1) C++11
	{
		uintptr_t diff       = calc_addr_diff( operand );
		uintptr_t old_offset = at_offset_.fetch_sub( diff, order );
		return value_type( calc_address( old_offset ) );
	}
	value_type fetch_sub( difference_type operand, std::memory_order order = std::memory_order_seq_cst ) noexcept   // (2) C++11
	{
		uintptr_t diff       = calc_addr_diff( operand );
		uintptr_t old_offset = at_offset_.fetch_sub( diff, order );
		return value_type( calc_address( old_offset ) );
	}

	value_type operator++( void ) volatile noexcept   // (1) C++20
	{
		value_type ans = fetch_add( 1 );
		++ans;
		return ans;
	}

	value_type operator++( void ) noexcept   // (2) C++20
	{
		value_type ans = fetch_add( 1 );
		++ans;
		return ans;
	}

	value_type operator++( int ) volatile noexcept   // (3) C++20
	{
		return fetch_add( 1 );
	}

	value_type operator++( int ) noexcept   // (4) C++20
	{
		return fetch_add( 1 );
	}

	value_type operator--() volatile noexcept   // (1) C++20
	{
		value_type ans = fetch_sub( 1 );
		--ans;
		return ans;
	}

	value_type operator--() noexcept   // (2) C++20
	{
		value_type ans = fetch_sub( 1 );
		--ans;
		return ans;
	}

	value_type operator--( int ) volatile noexcept   // (3) C++20
	{
		return fetch_sub( 1 );
	}

	value_type operator--( int ) noexcept   // (4) C++20
	{
		return fetch_sub( 1 );
	}

	value_type operator+=( difference_type operand ) volatile noexcept   // (1) C++11
	{
		value_type ans = fetch_add( operand );
		ans += operand;
		return ans;
	}
	value_type operator+=( difference_type operand ) noexcept   // (2) C++11
	{
		value_type ans = fetch_add( operand );
		ans += operand;
		return ans;
	}

	value_type operator-=( difference_type operand ) volatile noexcept   // (1) C++11
	{
		value_type ans = fetch_sub( operand );
		ans -= operand;
		return ans;
	}
	value_type operator-=( difference_type operand ) noexcept   // (2) C++11
	{
		value_type ans = fetch_sub( operand );
		ans -= operand;
		return ans;
	}

#ifdef __cpp_lib_atomic_is_always_lock_free   // #if __cpp_lib_atomic_is_always_lock_free >= 201603
	static constexpr bool is_always_lock_free = std::atomic<uintptr_t>::is_always_lock_free;
#endif

private:
	using element_pointer = T*;
	inline constexpr element_pointer calc_address( uintptr_t offset ) const noexcept
	{
		return reinterpret_cast<element_pointer>( reinterpret_cast<uintptr_t>( this ) + offset );
	}
	static inline constexpr uintptr_t calc_offset( const atomic_offset_ptr* base_p, element_pointer p ) noexcept
	{
		if ( p == nullptr ) {
			return 0;
		} else {
			return reinterpret_cast<uintptr_t>( p ) - reinterpret_cast<uintptr_t>( base_p );
		}
	}
	static inline constexpr uintptr_t calc_offset_as_nullptr( const atomic_offset_ptr* base_p ) noexcept
	{
		return 0;
		// return ( -reinterpret_cast<uintptr_t>( base_p ) );
	}
	static inline constexpr uintptr_t calc_addr_diff( difference_type n ) noexcept
	{
		element_pointer p = nullptr;
		p += n;
		return reinterpret_cast<uintptr_t>( p );
	}

	std::atomic<uintptr_t> at_offset_;
};

}   // namespace ipsm

#endif   // OFFSET_PTR_HPP_
