/**
 * @file offset_allocator.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2024-01-04
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef OFFSET_MEMORY_UTIL_HPP_
#define OFFSET_MEMORY_UTIL_HPP_

#include <memory>
#include <type_traits>

namespace ipsm {

template <typename T, typename Allocator, typename... Args,
          typename std::enable_if<
			  ( !std::uses_allocator<T, Allocator>::value ) &&
			  ( !std::is_constructible<T, Args...>::value )>::type* = nullptr>
inline T* make_obj_construct_using_allocator( const Allocator& alloc_arg, Args&&... args )
{
	using target_allocator_type        = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	using target_allocator_traits_type = std::allocator_traits<target_allocator_type>;

	target_allocator_type alloc( alloc_arg );

	// Tは集成体として構築できることを想定した関数
	auto p = target_allocator_traits_type::allocate( alloc, 1 );
	target_allocator_traits_type::construct( alloc, p, T { std::forward<Args>( args )... } );

	return p;
}
template <typename T, typename Allocator, typename... Args,
          typename std::enable_if<
			  ( !std::uses_allocator<T, Allocator>::value ) &&
			  std::is_constructible<T, Args...>::value>::type* = nullptr>
inline T* make_obj_construct_using_allocator( const Allocator& alloc_arg, Args&&... args )
{
	using target_allocator_type        = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	using target_allocator_traits_type = std::allocator_traits<target_allocator_type>;

	target_allocator_type alloc( alloc_arg );

	// Tはコンストラクタ呼び出しT(args...)として構築できることを想定した関数
	auto p = target_allocator_traits_type::allocate( alloc, 1 );
	target_allocator_traits_type::construct( alloc, p, std::forward<Args>( args )... );

	return p;
}
template <typename T, typename Allocator, typename... Args,
          typename std::enable_if<
			  std::uses_allocator<T, Allocator>::value &&
			  std::is_constructible<T, std::allocator_arg_t, Allocator, Args...>::value>::type* = nullptr>
inline T* make_obj_construct_using_allocator( const Allocator& alloc_arg, Args&&... args )
{
	using target_allocator_type        = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	using target_allocator_traits_type = std::allocator_traits<target_allocator_type>;

	target_allocator_type alloc( alloc_arg );

	// Tをアロケーター付きのコンストラクタ呼び出しT(args...)として構築できることを想定した関数
	auto p = target_allocator_traits_type::allocate( alloc, 1 );
	target_allocator_traits_type::construct( alloc, p, std::allocator_arg_t(), alloc, std::forward<Args>( args )... );

	return p;
}
template <typename T, typename Allocator, typename... Args,
          typename std::enable_if<
			  std::uses_allocator<T, Allocator>::value &&
			  std::is_constructible<T, Args..., Allocator>::value>::type* = nullptr>
inline T* make_obj_construct_using_allocator( const Allocator& alloc_arg, Args&&... args )
{
	using target_allocator_type        = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	using target_allocator_traits_type = std::allocator_traits<target_allocator_type>;

	target_allocator_type alloc( alloc_arg );

	// Tをアロケーター付きのコンストラクタ呼び出しT(args...)として構築できることを想定した関数
	auto p = target_allocator_traits_type::allocate( alloc, 1 );
	target_allocator_traits_type::construct( alloc, p, std::forward<Args>( args )..., alloc );

	return p;
}

template <typename T, typename Allocator>
inline void destruct_obj_usee_allocator( const Allocator& alloc_arg, T* p )
{
	if ( p == nullptr ) return;

	using target_allocator_type        = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
	using target_allocator_traits_type = std::allocator_traits<target_allocator_type>;

	target_allocator_type alloc( alloc_arg );

	target_allocator_traits_type::destroy( alloc, p );
	target_allocator_traits_type::deallocate( alloc, p, 1 );
}

}   // namespace ipsm

#endif   // OFFSET_MEMORY_UTIL_HPP_
