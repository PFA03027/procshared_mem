/**
 * @file offset_list.hpp
 * @author PFA03027@nifty.com
 * @brief offset based list container
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef OFFSET_LIST_HPP_
#define OFFSET_LIST_HPP_

#include <initializer_list>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include "offset_memory_util.hpp"
#include "offset_ptr.hpp"

namespace ipsm {

template <typename T, typename Allocator>
class offset_list;

template <bool, typename T, typename Allocator>
class iterator_impl_base;

template <bool, typename T, typename Allocator>
class iterator_impl;

template <bool, typename T, typename Allocator>
class reverse_iterator_impl;

template <typename T, typename Allocator = std::allocator<T>>
class offset_list {
public:
	using value_type      = T;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using size_type       = size_t;
	using offset_pointer  = offset_ptr<T>;
	using pointer         = typename offset_pointer::pointer;
	using const_pointer   = const pointer;
	using allocator_type  = Allocator;

	using iterator               = iterator_impl<false, T, Allocator>;
	using const_iterator         = iterator_impl<true, T, Allocator>;
	using reverse_iterator       = reverse_iterator_impl<false, T, Allocator>;
	using const_reverse_iterator = reverse_iterator_impl<true, T, Allocator>;

	offset_list( void ) = default;
	~offset_list();
	offset_list( const Allocator& a );
	offset_list( size_type n, const T& value, const Allocator& a = Allocator() );
	explicit offset_list( size_type n, const Allocator& a = Allocator() );

	template <class InputIterator>
	offset_list( InputIterator first, InputIterator last, const Allocator& a = Allocator() );   // TODO: support input iterator concept check

	offset_list( const offset_list& x );
	offset_list( offset_list&& x );
	offset_list( const offset_list& x, const Allocator& a );
	offset_list( offset_list&& x, const Allocator& a );
	offset_list( std::initializer_list<T> il, const Allocator& a = Allocator() )
	  : offset_list( il.begin(), il.end(), a )
	{
	}

	offset_list& operator=( const offset_list& x );
#ifdef __cpp_lib_allocator_traits_is_always_equal   // #if __cpp_lib_allocator_traits_is_always_equal >= 201411
	offset_list& operator=( offset_list&& x ) noexcept( std::allocator_traits<Allocator>::is_always_equal::value );
#else
	offset_list& operator=( offset_list&& x );
#endif
	offset_list& operator=( std::initializer_list<T> x );

	iterator       begin( void ) noexcept;
	const_iterator begin( void ) const noexcept;
	const_iterator cbegin( void ) const noexcept;
	iterator       end( void ) noexcept;
	const_iterator end( void ) const noexcept;
	const_iterator cend( void ) const noexcept;

	reverse_iterator       rbegin( void ) noexcept;
	const_reverse_iterator rbegin( void ) const noexcept;
	const_reverse_iterator crbegin( void ) const noexcept;
	reverse_iterator       rend( void ) noexcept;
	const_reverse_iterator rend( void ) const noexcept;
	const_reverse_iterator crend( void ) const noexcept;

	bool      empty( void ) const noexcept;
	size_type size( void ) const noexcept;
	size_type max_size() const noexcept;

	reference       front( void );
	const_reference front( void ) const;

	reference       back( void );
	const_reference back( void ) const;

	void push_front( const T& x );
	void push_front( T&& x );

	void push_back( const T& x );
	void push_back( T&& x );

	template <class... Args>
	reference emplace_front( Args&&... args );
	template <class... Args>
	reference emplace_back( Args&&... args );

	iterator insert( const_iterator position, const T& x );
	iterator insert( const_iterator position, T&& x );
	template <class... Args>
	iterator emplace( const_iterator position, Args&&... args );
	void     pop_front();
	void     pop_back();

	iterator erase( const_iterator position );
	iterator erase( const_iterator position, const_iterator last );
	void     clear( void ) noexcept;
#ifdef __cpp_lib_allocator_traits_is_always_equal   // #if __cpp_lib_allocator_traits_is_always_equal >= 201411
	void swap( offset_list& x ) noexcept( std::allocator_traits<Allocator>::is_always_equal::value );
#else
	void swap( offset_list& x );
#endif

	allocator_type get_allocator() const noexcept
	{
		return alloc_;
	}

private:
	struct node;
	using offset_pointer_to_node       = offset_ptr<node>;
	using offset_pointer_to_const_node = offset_ptr<const node>;

	struct node {
		offset_pointer_to_node op_pre_;
		offset_pointer_to_node op_nxt_;
		value_type             data_;

		template <typename... Args>
		node( Args&&... args )
		  : op_pre_( nullptr )
		  , op_nxt_( nullptr )
		  , data_ { std::forward<Args>( args )... }
		{
		}
		template <typename... Args>
		node( Args&&... args, const Allocator& a )
		  : op_pre_( nullptr )
		  , op_nxt_( nullptr )
		  , data_( std::forward<Args>( args )..., a )
		{
		}
		template <typename... Args>
		node( std::allocator_arg_t, const Allocator& a, Args&&... args )
		  : op_pre_( nullptr )
		  , op_nxt_( nullptr )
		  , data_( std::forward<Args>( args )..., a )
		{
		}
	};

	using node_allocator_type        = typename std::allocator_traits<allocator_type>::template rebind_alloc<node>;
	using node_allocator_traits_type = std::allocator_traits<node_allocator_type>;

	template <typename... Args,
	          typename std::enable_if<
				  ( !std::uses_allocator<T, Allocator>::value ) &&
				  ( !std::is_constructible<T, Args...>::value )>::type* = nullptr>
	node* uses_allocator_contruct_node( Args&&... args )
	{
		// Tは集成体として構築できることを想定した関数
		auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
		node_allocator_traits_type::construct( alloc_, p_node, std::forward<Args>( args )... );

		return p_node;
	}
	template <typename... Args,
	          typename std::enable_if<
				  ( !std::uses_allocator<T, Allocator>::value ) &&
				  std::is_constructible<T, Args...>::value>::type* = nullptr>
	node* uses_allocator_contruct_node( Args&&... args )
	{
		// Tはコンストラクタ呼び出しT(args...)として構築できることを想定した関数
		auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
		node_allocator_traits_type::construct( alloc_, p_node, std::forward<Args>( args )... );

		return p_node;
	}
	template <typename... Args,
	          typename std::enable_if<
				  std::uses_allocator<T, Allocator>::value &&
				  std::is_constructible<T, std::allocator_arg_t, Allocator, Args...>::value>::type* = nullptr>
	node* uses_allocator_contruct_node( Args&&... args )
	{
		auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
		node_allocator_traits_type::construct( alloc_, p_node, std::allocator_arg_t(), alloc_, std::forward<Args>( args )... );

		return p_node;
	}
	template <typename... Args,
	          typename std::enable_if<
				  std::uses_allocator<T, Allocator>::value &&
				  std::is_constructible<T, Args..., Allocator>::value>::type* = nullptr>
	node* uses_allocator_contruct_node( Args&&... args )
	{
		auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
		node_allocator_traits_type::construct( alloc_, p_node, std::forward<Args>( args )..., alloc_ );

		return p_node;
	}

	void usee_allocator_destruct_node( node* p_node )
	{
		if ( p_node == nullptr ) return;

		node_allocator_traits_type::destroy( alloc_, p_node );
		node_allocator_traits_type::deallocate( alloc_, p_node, 1 );
	}

	void insert_node_to_front( node* p_node )
	{
		if ( op_head_ == nullptr ) {
			op_head_ = p_node;
			op_tail_ = p_node;
		} else {
			op_head_->op_pre_ = p_node;
			p_node->op_nxt_   = op_head_;
			op_head_          = p_node;
		}
	}
	void insert_node_to_back( node* p_node )
	{
		if ( op_tail_ == nullptr ) {
			op_head_ = p_node;
			op_tail_ = p_node;
		} else {
			op_tail_->op_nxt_ = p_node;
			p_node->op_pre_   = op_tail_;
			op_tail_          = p_node;
		}
	}
	void insert_node_before_position( const_iterator position, node* p_node )
	{
		// TODO: pre-check that position belongs to this container instance, as debug support function
		const node* p_const_cur = position.op_cur_node_;
		node*       p_cur       = const_cast<node*>( p_const_cur );
		if ( p_cur == nullptr ) {   // end()と等価
			node* p_tail = op_tail_;
			if ( p_tail == nullptr ) {
				op_tail_ = p_node;
				op_head_ = p_node;
			} else {
				p_tail->op_nxt_ = p_node;
				p_node->op_pre_ = op_tail_;
				op_tail_        = p_node;
			}
		} else {
			node* p_pre = p_cur->op_pre_;
			if ( p_pre == nullptr ) {
				p_cur->op_pre_  = p_node;
				p_node->op_nxt_ = p_cur;
				op_head_        = p_node;
			} else {
				p_pre->op_nxt_  = p_node;
				p_cur->op_pre_  = p_node;
				p_node->op_pre_ = p_pre;
				p_node->op_nxt_ = p_cur;
			}
		}
	}

	node_allocator_type    alloc_;
	offset_pointer_to_node op_head_;
	offset_pointer_to_node op_tail_;

	template <bool CU, typename U, typename UAllocator>
	friend class iterator_impl_base;
	template <bool CU, typename U, typename UAllocator>
	friend class iterator_impl;
	template <bool CU, typename U, typename UAllocator>
	friend class reverse_iterator_impl;
};

template <bool CT, typename T, typename Allocator>   // primary template
class iterator_impl_base {};
template <typename T, typename Allocator>   // const type
class iterator_impl_base<true, T, Allocator> {
public:
	using iterator_value_pointer   = typename offset_list<T, Allocator>::const_pointer;
	using iterator_value_reference = typename offset_list<T, Allocator>::const_reference;

protected:
	using iterator_node_type  = typename offset_list<T, Allocator>::offset_pointer_to_const_node;
	using iterator_owner_type = const offset_list<T, Allocator>;
};
template <typename T, typename Allocator>   // non const type
class iterator_impl_base<false, T, Allocator> {
public:
	using iterator_value_pointer   = typename offset_list<T, Allocator>::pointer;
	using iterator_value_reference = typename offset_list<T, Allocator>::reference;

protected:
	using iterator_node_type  = typename offset_list<T, Allocator>::offset_pointer_to_node;
	using iterator_owner_type = offset_list<T, Allocator>;
};

template <bool CT, typename T, typename Allocator>   // primary template
class iterator_impl : public iterator_impl_base<CT, T, Allocator> {
	using offset_pointer_to_node_t  = typename iterator_impl_base<CT, T, Allocator>::iterator_node_type;
	using owner_type                = typename iterator_impl_base<CT, T, Allocator>::iterator_owner_type;
	using offset_pointer_to_owner_t = offset_ptr<typename iterator_impl_base<CT, T, Allocator>::iterator_owner_type>;

public:
	using reference = typename iterator_impl_base<CT, T, Allocator>::iterator_value_reference;
	using pointer   = typename iterator_impl_base<CT, T, Allocator>::iterator_value_pointer;

	iterator_impl( void ) = default;
	~iterator_impl()      = default;

	template <bool CU = CT, typename U = T, typename UAllocator = Allocator,
	          typename std::enable_if<std::is_convertible<typename iterator_impl<CU, U, UAllocator>::offset_pointer_to_node_t, offset_pointer_to_node_t>::value>::type* = nullptr>
	constexpr iterator_impl( const iterator_impl<CU, U, UAllocator>& orig ) noexcept
	  : op_owner_( orig.op_owner_ )
	  , op_cur_node_( orig.op_cur_node_ )
	{
	}

	template <bool CU = CT, typename U = T, typename UAllocator = Allocator,
	          typename std::enable_if<std::is_convertible<typename iterator_impl<CU, U, UAllocator>::offset_pointer_to_node_t, offset_pointer_to_node_t>::value>::type* = nullptr>
	constexpr iterator_impl( iterator_impl<CU, U, UAllocator>&& orig ) noexcept
	  : op_owner_( std::move( orig.op_owner_ ) )
	  , op_cur_node_( std::move( orig.op_cur_node_ ) )
	{
		// orig.op_owner_    = nullptr;
		// orig.op_cur_node_ = nullptr;
	}

	template <bool CU = CT, typename U = T, typename UAllocator = Allocator,
	          typename std::enable_if<std::is_convertible<typename iterator_impl<CU, U, UAllocator>::offset_pointer_to_node_t, offset_pointer_to_node_t>::value>::type* = nullptr>
	iterator_impl& operator=( const iterator_impl<CU, U, UAllocator>& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		op_owner_    = orig.op_owner_;
		op_cur_node_ = orig.op_cur_node_;
		return *this;
	}

	template <bool CU = CT, typename U = T, typename UAllocator = Allocator,
	          typename std::enable_if<std::is_convertible<typename iterator_impl<CU, U, UAllocator>::offset_pointer_to_node_t, offset_pointer_to_node_t>::value>::type* = nullptr>
	iterator_impl& operator=( iterator_impl<CU, U, UAllocator>&& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		op_owner_         = std::move( orig.op_owner_ );
		op_cur_node_      = std::move( orig.op_cur_node_ );
		orig.op_owner_    = nullptr;
		orig.op_cur_node_ = nullptr;
		return *this;
	}

	reference operator*( void ) const noexcept
	{
		return op_cur_node_->data_;
	}
	pointer operator->( void ) const noexcept
	{
		return &( op_cur_node_->data_ );
	}

	iterator_impl& operator++()   // pre increment
	{
		if ( op_cur_node_ != nullptr ) {
			op_cur_node_ = op_cur_node_->op_nxt_;
		}
		return *this;
	}
	iterator_impl operator++( int )   // post increment
	{
		iterator_impl ans = *this;
		if ( op_cur_node_ != nullptr ) {
			op_cur_node_ = op_cur_node_->op_nxt_;
		}
		return ans;
	}

	iterator_impl& operator--()   // pre increment
	{
		if ( op_cur_node_ != nullptr ) {
			// op_cur_node_がbegin()と等価の場合は、考慮しない。結果として、UB(undefined behaviorとなる)
			op_cur_node_ = op_cur_node_->op_pre_;
		} else {
			// nullptrの場合は、end()と等価とみなす。
			op_cur_node_ = op_owner_->op_tail_;
		}
		return *this;
	}
	iterator_impl operator--( int )   // post decrement
	{
		iterator_impl ans = *this;
		if ( op_cur_node_ != nullptr ) {
			// op_cur_node_がbegin()と等価の場合は、考慮しない。結果として、UB(undefined behaviorとなる)
			op_cur_node_ = op_cur_node_->op_pre_;
		} else {
			// nullptrの場合は、end()と等価とみなす。
			op_cur_node_ = op_owner_->op_tail_;
		}
		return ans;
	}

private:
	template <typename XT, typename XAllocator>
	friend class offset_list;
	friend class iterator_impl<!CT, T, Allocator>;

	template <typename XT, typename XAllocator, bool XCT1, bool XCT2>
	friend bool operator==( const iterator_impl<XCT1, XT, XAllocator>& a, const iterator_impl<XCT2, XT, XAllocator>& b );
	template <typename XT, typename XAllocator, bool XCT1, bool XCT2>
	friend bool operator!=( const iterator_impl<XCT1, XT, XAllocator>& a, const iterator_impl<XCT2, XT, XAllocator>& b );

	iterator_impl( owner_type& owner, const offset_pointer_to_node_t& src )
	  : op_owner_( &owner )
	  , op_cur_node_( src )
	{
	}
	iterator_impl( owner_type& owner, const std::nullptr_t src )
	  : op_owner_( &owner )
	  , op_cur_node_( src )
	{
	}

	offset_pointer_to_owner_t op_owner_;
	offset_pointer_to_node_t  op_cur_node_;
};

template <bool CT, typename T, typename Allocator>   // primary template
class reverse_iterator_impl : public iterator_impl_base<CT, T, Allocator> {
	using offset_pointer_to_node_t  = typename iterator_impl_base<CT, T, Allocator>::iterator_node_type;
	using owner_type                = typename iterator_impl_base<CT, T, Allocator>::iterator_owner_type;
	using offset_pointer_to_owner_t = offset_ptr<typename iterator_impl_base<CT, T, Allocator>::iterator_owner_type>;

public:
	using reference = typename iterator_impl_base<CT, T, Allocator>::iterator_value_reference;
	using pointer   = typename iterator_impl_base<CT, T, Allocator>::iterator_value_pointer;

	reverse_iterator_impl( void ) = default;
	~reverse_iterator_impl()      = default;

	template <bool CU = CT, typename U = T, typename UAllocator = Allocator,
	          typename std::enable_if<std::is_convertible<typename reverse_iterator_impl<CU, U, UAllocator>::offset_pointer_to_node_t, offset_pointer_to_node_t>::value>::type* = nullptr>
	constexpr reverse_iterator_impl( const reverse_iterator_impl<CU, U, UAllocator>& orig ) noexcept
	  : op_owner_( orig.op_owner_ )
	  , op_cur_node_( orig.op_cur_node_ )
	{
	}

	template <bool CU = CT, typename U = T, typename UAllocator = Allocator,
	          typename std::enable_if<std::is_convertible<typename reverse_iterator_impl<CU, U, UAllocator>::offset_pointer_to_node_t, offset_pointer_to_node_t>::value>::type* = nullptr>
	constexpr reverse_iterator_impl( reverse_iterator_impl<CU, U, UAllocator>&& orig ) noexcept
	  : op_owner_( std::move( orig.op_owner_ ) )
	  , op_cur_node_( std::move( orig.op_cur_node_ ) )
	{
		orig.op_owner_    = nullptr;
		orig.op_cur_node_ = nullptr;
	}

	template <bool CU = CT, typename U = T, typename UAllocator = Allocator,
	          typename std::enable_if<std::is_convertible<typename reverse_iterator_impl<CU, U, UAllocator>::offset_pointer_to_node_t, offset_pointer_to_node_t>::value>::type* = nullptr>
	reverse_iterator_impl& operator=( const reverse_iterator_impl<CU, U, UAllocator>& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		op_owner_    = orig.op_owner_;
		op_cur_node_ = orig.op_cur_node_;
		return *this;
	}

	template <bool CU = CT, typename U = T, typename UAllocator = Allocator,
	          typename std::enable_if<std::is_convertible<typename reverse_iterator_impl<CU, U, UAllocator>::offset_pointer_to_node_t, offset_pointer_to_node_t>::value>::type* = nullptr>
	reverse_iterator_impl& operator=( reverse_iterator_impl<CU, U, UAllocator>&& orig ) noexcept
	{
		if ( this == &orig ) return *this;

		op_owner_         = std::move( orig.op_owner_ );
		op_cur_node_      = std::move( orig.op_cur_node_ );
		orig.op_owner_    = nullptr;
		orig.op_cur_node_ = nullptr;
		return *this;
	}

	reference operator*( void ) const noexcept
	{
		return op_cur_node_->data_;
	}
	pointer operator->( void ) const noexcept
	{
		return &( op_cur_node_->data_ );
	}

	reverse_iterator_impl& operator++()   // pre increment
	{
		if ( op_cur_node_ != nullptr ) {
			op_cur_node_ = op_cur_node_->op_pre_;
		}
		return *this;
	}
	reverse_iterator_impl operator++( int )   // post increment
	{
		reverse_iterator_impl ans = *this;
		if ( op_cur_node_ != nullptr ) {
			op_cur_node_ = op_cur_node_->op_pre_;
		}
		return ans;
	}

	reverse_iterator_impl& operator--()   // pre increment
	{
		if ( op_cur_node_ != nullptr ) {
			// op_cur_node_がrbegin()と等価の場合は、考慮しない。結果として、UB(undefined behaviorとなる)
			op_cur_node_ = op_cur_node_->op_nxt_;
		} else {
			// nullptrの場合は、rend()と等価とみなす。
			op_cur_node_ = op_owner_->op_head_;
		}
		return *this;
	}
	reverse_iterator_impl operator--( int )   // post decrement
	{
		reverse_iterator_impl ans = *this;
		if ( op_cur_node_ != nullptr ) {
			// op_cur_node_がrbegin()と等価の場合は、考慮しない。結果として、UB(undefined behaviorとなる)
			op_cur_node_ = op_cur_node_->op_nxt_;
		} else {
			// nullptrの場合は、rend()と等価とみなす。
			op_cur_node_ = op_owner_->op_head_;
		}
		return ans;
	}

private:
	template <typename XT, typename XAllocator>
	friend class offset_list;
	friend class reverse_iterator_impl<!CT, T, Allocator>;

	template <typename XT, typename XAllocator, bool XCT1, bool XCT2>
	friend bool operator==( const reverse_iterator_impl<XCT1, XT, XAllocator>& a, const reverse_iterator_impl<XCT2, XT, XAllocator>& b );
	template <typename XT, typename XAllocator, bool XCT1, bool XCT2>
	friend bool operator!=( const reverse_iterator_impl<XCT1, XT, XAllocator>& a, const reverse_iterator_impl<XCT2, XT, XAllocator>& b );

	reverse_iterator_impl( owner_type& owner, const offset_pointer_to_node_t& src )
	  : op_owner_( &owner )
	  , op_cur_node_( src ) {}

	offset_pointer_to_owner_t op_owner_;
	offset_pointer_to_node_t  op_cur_node_;
};

//////////////////////////////////////////////////////////////////////////////
// implementation

template <typename T, typename Allocator>
offset_list<T, Allocator>::offset_list( const Allocator& a )
  : alloc_( a )
  , op_head_()
  , op_tail_()
{
}

template <typename T, typename Allocator>
offset_list<T, Allocator>::~offset_list()
{
	clear();
}

template <typename T, typename Allocator>
offset_list<T, Allocator>::offset_list( size_type n, const T& value, const Allocator& a )
  : offset_list( a )
{
	for ( size_type i = 0; i < n; i++ ) {
		push_back( value );
	}
}

template <typename T, typename Allocator>
offset_list<T, Allocator>::offset_list( size_type n, const Allocator& a )
  : offset_list( a )
{
	for ( size_type i = 0; i < n; i++ ) {
		emplace_back();
	}
}

template <typename T, typename Allocator>
template <class InputIterator>
offset_list<T, Allocator>::offset_list( InputIterator first, InputIterator last, const Allocator& a )   // TODO: support input iterator concept check
  : offset_list( a )
{
	for ( InputIterator it = first; it != last; ++it ) {
		push_back( *it );
	}
}

template <typename T, typename Allocator>
offset_list<T, Allocator>::offset_list( const offset_list& x )
  : offset_list( x, std::allocator_traits<Allocator>::select_on_container_copy_construction( x.alloc_ ) )
{
}

template <typename T, typename Allocator>
offset_list<T, Allocator>::offset_list( offset_list&& x )
  : offset_list( std::move( x ), x.alloc_ )
{
}

template <typename T, typename Allocator>
offset_list<T, Allocator>::offset_list( const offset_list& x, const Allocator& a )
  : offset_list( a )
{
	for ( auto& e : x ) {
		push_back( e );
	}
}

template <typename T, typename Allocator>
offset_list<T, Allocator>::offset_list( offset_list&& x, const Allocator& a )
  : offset_list( a )
{
	if ( alloc_ == x.alloc_ ) {
		op_head_ = std::move( x.op_head_ );
		op_tail_ = std::move( x.op_tail_ );
	} else {
		// 本当は削除しながらmoveした方がキャッシュヒット率が高くなるので速くなると思うが、
		// わかりにくいので、2パス方式で実装する。
		for ( auto& e : x ) {
			push_back( std::move( e ) );
		}
		x.clear();
	}
}

template <typename T, typename Allocator>
offset_list<T, Allocator>& offset_list<T, Allocator>::operator=( const offset_list& x )
{
	if ( this == &x ) return *this;

	clear();   // allocatorの置き換え前に、ノードを削除する

	if ( node_allocator_traits_type::propagate_on_container_copy_assignment::value ) {
		alloc_ = x.alloc_;
	}

	for ( auto& e : x ) {
		push_back( e );
	}

	return *this;
}

template <typename T, typename Allocator>
#ifdef __cpp_lib_allocator_traits_is_always_equal   // #if __cpp_lib_allocator_traits_is_always_equal >= 201411
offset_list<T, Allocator>& offset_list<T, Allocator>::operator=( offset_list&& x ) noexcept( std::allocator_traits<Allocator>::is_always_equal::value )
#else
offset_list<T, Allocator>& offset_list<T, Allocator>::operator=( offset_list&& x )
#endif
{
	if ( this == &x ) return *this;

	clear();   // allocatorの置き換え前に、ノードを削除する

	if ( node_allocator_traits_type::propagate_on_container_move_assignment::value ) {
		alloc_ = x.alloc_;
	}

	if ( alloc_ == x.alloc_ ) {
		op_head_ = std::move( x.op_head_ );
		op_tail_ = std::move( x.op_tail_ );
	} else {
		// 本当は削除しながらmoveした方がキャッシュヒット率が高くなるので速くなると思うが、
		// わかりにくいので、2パス方式で実装する。
		for ( auto& e : x ) {
			push_back( std::move( e ) );
		}
		x.clear();
	}

	return *this;
}

template <typename T, typename Allocator>
offset_list<T, Allocator>& offset_list<T, Allocator>::operator=( std::initializer_list<T> x )
{
	clear();
	for ( auto& e : x ) {
		push_back( std::move( e ) );
	}
	return *this;
}

template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::iterator offset_list<T, Allocator>::begin( void ) noexcept
{
	return iterator( *this, op_head_ );
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_iterator offset_list<T, Allocator>::begin( void ) const noexcept
{
	return const_iterator( *this, op_head_ );
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_iterator offset_list<T, Allocator>::cbegin( void ) const noexcept
{
	return begin();
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::iterator offset_list<T, Allocator>::end( void ) noexcept
{
	return iterator( *this, nullptr );
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_iterator offset_list<T, Allocator>::end( void ) const noexcept
{
	return const_iterator( *this, nullptr );
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_iterator offset_list<T, Allocator>::cend( void ) const noexcept
{
	return end();
}

template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::reverse_iterator offset_list<T, Allocator>::rbegin( void ) noexcept
{
	return reverse_iterator( *this, op_tail_ );
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_reverse_iterator offset_list<T, Allocator>::rbegin( void ) const noexcept
{
	return const_reverse_iterator( *this, op_tail_ );
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_reverse_iterator offset_list<T, Allocator>::crbegin( void ) const noexcept
{
	return rbegin();
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::reverse_iterator offset_list<T, Allocator>::rend( void ) noexcept
{
	return reverse_iterator( *this, nullptr );
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_reverse_iterator offset_list<T, Allocator>::rend( void ) const noexcept
{
	return const_reverse_iterator( *this, nullptr );
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_reverse_iterator offset_list<T, Allocator>::crend( void ) const noexcept
{
	return rend();
}

template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::size_type offset_list<T, Allocator>::max_size() const noexcept
{
	return std::numeric_limits<size_type>::max();
}

template <typename T, typename Allocator>
inline bool offset_list<T, Allocator>::empty( void ) const noexcept
{
	return op_head_ == nullptr;
}

template <typename T, typename Allocator>
typename offset_list<T, Allocator>::size_type offset_list<T, Allocator>::size( void ) const noexcept
{
	size_type ans = 0;
	for ( auto it = begin(); it != end(); ++it ) {
		ans++;
	}
	return ans;
}

template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::reference offset_list<T, Allocator>::front( void )
{
	return *begin();
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_reference offset_list<T, Allocator>::front( void ) const
{
	return *begin();
}

template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::reference offset_list<T, Allocator>::back( void )
{
	auto eit = end();
	--eit;
	return *eit;
}
template <typename T, typename Allocator>
inline typename offset_list<T, Allocator>::const_reference offset_list<T, Allocator>::back( void ) const
{
	auto eit = end();
	--eit;
	return *eit;
}

template <typename T, typename Allocator>
void offset_list<T, Allocator>::push_front( const T& x )
{
#if 1
	auto p_node = uses_allocator_contruct_node( x );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, x } );
#endif
	insert_node_to_front( p_node );
}
template <typename T, typename Allocator>
void offset_list<T, Allocator>::push_front( T&& x )
{
#if 1
	auto p_node = uses_allocator_contruct_node( std::move( x ) );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, std::move( x ) } );
#endif
	insert_node_to_front( p_node );
}

template <typename T, typename Allocator>
void offset_list<T, Allocator>::push_back( const T& x )
{
#if 1
	auto p_node = uses_allocator_contruct_node( x );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, x } );
#endif
	insert_node_to_back( p_node );
}
template <typename T, typename Allocator>
void offset_list<T, Allocator>::push_back( T&& x )
{
#if 1
	auto p_node = uses_allocator_contruct_node( std::move( x ) );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, std::move( x ) } );
#endif
	insert_node_to_back( p_node );
}

template <typename T, typename Allocator>
template <class... Args>
typename offset_list<T, Allocator>::reference offset_list<T, Allocator>::emplace_front( Args&&... args )
{
#if 1
	auto p_node = uses_allocator_contruct_node( std::forward<Args>( args )... );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, value_type { std::forward<Args>( args )... } } );
#endif
	insert_node_to_front( p_node );
	return p_node->data_;
}

template <typename T, typename Allocator>
template <class... Args>
typename offset_list<T, Allocator>::reference offset_list<T, Allocator>::emplace_back( Args&&... args )
{
#if 1
	auto p_node = uses_allocator_contruct_node( std::forward<Args>( args )... );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, value_type { std::forward<Args>( args )... } } );
#endif
	insert_node_to_back( p_node );
	return p_node->data_;
}

template <typename T, typename Allocator>
typename offset_list<T, Allocator>::iterator offset_list<T, Allocator>::insert( const_iterator position, const T& x )
{
#if 1
	auto p_node = uses_allocator_contruct_node( x );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, x } );
#endif

	insert_node_before_position( position, p_node );

	return iterator( *this, p_node );
}
template <typename T, typename Allocator>
typename offset_list<T, Allocator>::iterator offset_list<T, Allocator>::insert( const_iterator position, T&& x )
{
#if 1
	auto p_node = uses_allocator_contruct_node( std::move( x ) );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, std::move( x ) } );
#endif

	insert_node_before_position( position, p_node );

	return iterator( *this, p_node );
}
template <typename T, typename Allocator>
template <class... Args>
typename offset_list<T, Allocator>::iterator offset_list<T, Allocator>::emplace( const_iterator position, Args&&... args )
{
#if 1
	auto p_node = uses_allocator_contruct_node( std::forward<Args>( args )... );
#else
	auto p_node = node_allocator_traits_type::allocate( alloc_, 1 );
	node_allocator_traits_type::construct( alloc_, p_node, node { nullptr, nullptr, value_type { std::forward<Args>( args )... } } );
#endif

	insert_node_before_position( position, p_node );

	return iterator( *this, p_node );
}
template <typename T, typename Allocator>
void offset_list<T, Allocator>::pop_front()
{
	erase( begin() );
}
template <typename T, typename Allocator>
void offset_list<T, Allocator>::pop_back()
{
	auto tail_it = end();
	--tail_it;
	erase( tail_it );
}

template <typename T, typename Allocator>
typename offset_list<T, Allocator>::iterator offset_list<T, Allocator>::erase( const_iterator position )
{
	if ( position == end() ) return end();
	const_iterator last = position;
	++last;
	return erase( position, last );
}
template <typename T, typename Allocator>
typename offset_list<T, Allocator>::iterator offset_list<T, Allocator>::erase( const_iterator position, const_iterator last )
{
	if ( position == end() ) return end();

	// TODO: pre-check that position/last belongs to this container instance, as debug support function
	iterator    ans;
	const node* p_const_target_node = position.op_cur_node_;
	node*       p_target_node       = const_cast<node*>( p_const_target_node );
	node*       p_pre_node          = p_target_node->op_pre_;
	const node* p_const_last_node   = last.op_cur_node_;
	node*       p_nxt_node          = const_cast<node*>( p_const_last_node );

	while ( p_target_node != p_nxt_node ) {
		node* p_next = p_target_node->op_nxt_;
		usee_allocator_destruct_node( p_target_node );
		p_target_node = p_next;
	}

	if ( ( p_pre_node == nullptr ) && ( p_nxt_node == nullptr ) ) {
		op_head_ = nullptr;
		op_tail_ = nullptr;
		ans      = end();
	} else if ( p_pre_node == nullptr ) {
		op_head_            = p_nxt_node;
		p_nxt_node->op_pre_ = nullptr;
		ans                 = iterator( *this, p_nxt_node );
	} else if ( p_nxt_node == nullptr ) {
		op_tail_            = p_pre_node;
		p_pre_node->op_nxt_ = nullptr;
		ans                 = end();
	} else {
		p_pre_node->op_nxt_ = p_nxt_node;
		p_nxt_node->op_pre_ = p_pre_node;
		ans                 = iterator( *this, p_nxt_node );
	}

	return ans;
}

template <typename T, typename Allocator>
void offset_list<T, Allocator>::clear( void ) noexcept
{
	node* p_cur_node = op_head_;
	while ( p_cur_node != nullptr ) {
		node* p_next = p_cur_node->op_nxt_;

		usee_allocator_destruct_node( p_cur_node );

		p_cur_node = p_next;
	}

	op_head_ = nullptr;
	op_tail_ = nullptr;
}

template <typename T, typename Allocator>
#ifdef __cpp_lib_allocator_traits_is_always_equal   // #if __cpp_lib_allocator_traits_is_always_equal >= 201411
void offset_list<T, Allocator>::swap( offset_list& x ) noexcept( std::allocator_traits<Allocator>::is_always_equal::value )
#else
void offset_list<T, Allocator>::swap( offset_list& x )
#endif
{
	if ( node_allocator_traits_type::propagate_on_container_swap::value ) {
		node_allocator_type tmp_alloc = alloc_;
		alloc_                        = x.alloc_;
		x.alloc_                      = tmp_alloc;
	}
	if ( alloc_ == x.alloc_ ) {
		op_head_.swap( x.op_head_ );
		op_tail_.swap( x.op_tail_ );
	} else {
		// 自分自身のデータは、退避する。
		offset_pointer_to_node op_orig_head = std::move( op_head_ );
		op_tail_                            = nullptr;

		// 本当は削除しながらmoveした方がキャッシュヒット率が高くなるので速くなると思うが、
		// わかりにくいので、2パス方式で実装する。
		for ( auto& e : x ) {
			push_back( std::move( e ) );
		}
		x.clear();

		for ( auto op_cur = op_orig_head; op_cur != nullptr; ) {
			x.push_back( std::move( op_cur->data_ ) );
			op_cur = op_cur->op_nxt_;
			usee_allocator_destruct_node( op_cur );
		}
	}
}

template <typename XT, typename XAllocator, bool XCT1, bool XCT2>
bool operator==( const iterator_impl<XCT1, XT, XAllocator>& a, const iterator_impl<XCT2, XT, XAllocator>& b )
{
	return ( a.op_cur_node_ == b.op_cur_node_ );
}
template <typename XT, typename XAllocator, bool XCT1, bool XCT2>
bool operator!=( const iterator_impl<XCT1, XT, XAllocator>& a, const iterator_impl<XCT2, XT, XAllocator>& b )
{
	return ( a.op_cur_node_ != b.op_cur_node_ );
}

template <typename XT, typename XAllocator, bool XCT1, bool XCT2>
bool operator==( const reverse_iterator_impl<XCT1, XT, XAllocator>& a, const reverse_iterator_impl<XCT2, XT, XAllocator>& b )
{
	return ( a.op_cur_node_ == b.op_cur_node_ );
}
template <typename XT, typename XAllocator, bool XCT1, bool XCT2>
bool operator!=( const reverse_iterator_impl<XCT1, XT, XAllocator>& a, const reverse_iterator_impl<XCT2, XT, XAllocator>& b )
{
	return ( a.op_cur_node_ != b.op_cur_node_ );
}

}   // namespace ipsm

#endif   // OFFSET_LIST_HPP_
