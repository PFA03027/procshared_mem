/**
 * @file offset_shared_ptr.hpp
 * @author PFA03027@nifty.com
 * @brief Shared pointer based on offset_ptr
 * @version 0.1
 * @date 2023-10-20
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef OFFSET_SHARED_PTR_HPP_
#define OFFSET_SHARED_PTR_HPP_

#include <memory>
#include <mutex>

#include "offset_unique_ptr.hpp"
#include "procshared_mutex.hpp"

namespace ipsm {

namespace offset_shared_ptr_detail {

class alloc_if_abst;

class offset_shared_ptr_impl_if {
public:
	struct ctrl_data {
		ctrl_data( void )
		  : weak_refc_( 0 )
		  , shrd_refc_( 0 )
		{
		}

		long weak_refc_;   // reference conut of this class instance by weak pointer
		long shrd_refc_;   // reference conut of this class instance by shared pointer
	};

	class ascer {
	public:
		ascer( ascer&& a )
		  : lk_( std::move( a.lk_ ) )
		  , p_ctrl_( a.p_ctrl_ )
		{
			a.p_ctrl_ = nullptr;
		}

		ascer& operator=( ascer&& a )
		{
			if ( this == &a ) return *this;

			if ( lk_.owns_lock() ) {
				lk_.unlock();
			}
			p_ctrl_   = a.p_ctrl_;
			a.p_ctrl_ = nullptr;
			lk_       = std::move( a.lk_ );   // ここで例外が発生する場合、再帰mutexで複数回lockされている状態が考えられる。

			return *this;
		}

		ctrl_data& ref( void )
		{
			return *p_ctrl_;
		}

		const ctrl_data& ref( void ) const
		{
			return *p_ctrl_;
		}

	private:
		explicit ascer( offset_shared_ptr_impl_if& a )
		  : lk_( a.mtx_ )
		  , p_ctrl_( &( a.ctrl_ ) )
		{
		}

		friend offset_shared_ptr_impl_if;

		std::unique_lock<procshared_mutex> lk_;
		ctrl_data*                         p_ctrl_;
	};

	class const_ascer {
	public:
		const_ascer( const_ascer&& a )
		  : lk_( std::move( a.lk_ ) )
		  , p_ctrl_( a.p_ctrl_ )
		{
			a.p_ctrl_ = nullptr;
		}
		const_ascer& operator=( const_ascer&& a )
		{
			if ( this == &a ) return *this;

			if ( lk_.owns_lock() ) {
				lk_.unlock();
			}
			p_ctrl_   = a.p_ctrl_;
			a.p_ctrl_ = nullptr;
			lk_       = std::move( a.lk_ );   // ここで例外が発生する場合、再帰mutexで複数回lockされている状態が考えられる。

			return *this;
		}

		const ctrl_data& ref( void ) const
		{
			return *p_ctrl_;
		}

	private:
		explicit const_ascer( const offset_shared_ptr_impl_if& a )
		  : lk_( a.mtx_ )
		  , p_ctrl_( &( a.ctrl_ ) )
		{
		}

		friend offset_shared_ptr_impl_if;

		std::unique_lock<procshared_mutex> lk_;
		const ctrl_data*                   p_ctrl_;
	};

	virtual ~offset_shared_ptr_impl_if()              = default;
	virtual alloc_if_abst* get_alloc_if( void ) const = 0;   // 取得したオブジェクトは、使い終わったらdeleteで破棄すること。
	virtual void           dispose_resource( void )   = 0;

	ascer get_ascer( void )
	{
		return ascer( *this );
	}

	const_ascer get_ascer( void ) const
	{
		return const_ascer( *this );
	}

	static inline void try_dispose_shared( offset_ptr<offset_shared_ptr_impl_if>& p )
	{
		try_dispose_com( p, &offset_shared_ptr_impl_if::ctrl_data::shrd_refc_ );
	}
	static inline void try_dispose_weak( offset_ptr<offset_shared_ptr_impl_if>& p )
	{
		try_dispose_com( p, &offset_shared_ptr_impl_if::ctrl_data::weak_refc_ );
	}

private:
	static void try_dispose_com( offset_ptr<offset_shared_ptr_impl_if>& p, long ctrl_data::*mp_tcnt );

	mutable procshared_mutex mtx_;   // mutex for exclusive access control
	ctrl_data                ctrl_;
};

class alloc_if_abst {
public:
	virtual ~alloc_if_abst()                                             = default;
	virtual void call_destroy_deallocate( offset_shared_ptr_impl_if* p ) = 0;
};

template <typename Alloc, typename IFConcrete>
class alloc_if_concrete_of_Alloc : public alloc_if_abst {
public:
	using value_type            = typename Alloc::value_type;
	using size_type             = typename Alloc::size_type;
	using difference_type       = typename Alloc::difference_type;
	using allocator_type        = typename std::allocator_traits<Alloc>::template rebind_alloc<IFConcrete>;
	using allocator_traits_type = std::allocator_traits<allocator_type>;

	alloc_if_concrete_of_Alloc( const Alloc& al );
	~alloc_if_concrete_of_Alloc() = default;

	void call_destroy_deallocate( offset_shared_ptr_impl_if* p ) override;

private:
	allocator_type allocator_;
};

template <typename R, typename D, typename AllocRT>
class offset_shared_ptr_impl : public offset_shared_ptr_impl_if {
public:
	using resource_type           = R;
	using resource_pointer        = resource_type*;
	using deleter_type            = D;
	using resource_allocator_type = AllocRT;

	offset_shared_ptr_impl( resource_pointer p_r_arg, D& del, AllocRT& allc )
	  : offset_shared_ptr_impl_if()
	  , op_to_obj_( p_r_arg, del )
	  , alloc_( allc )
	{
	}

	alloc_if_abst* get_alloc_if( void ) const override;

	void dispose_resource( void ) override
	{
		op_to_obj_.reset();
	}

private:
	offset_unique_ptr<R, D> op_to_obj_;   // offset pointer to an object of T

	AllocRT alloc_;   // allocator
};

template <typename Alloc, typename IFConcrete>
alloc_if_concrete_of_Alloc<Alloc, IFConcrete>::alloc_if_concrete_of_Alloc( const Alloc& al )
  : allocator_( al )
{
}

template <typename Alloc, typename IFConcrete>
void alloc_if_concrete_of_Alloc<Alloc, IFConcrete>::call_destroy_deallocate( offset_shared_ptr_impl_if* p )
{
	if ( p == nullptr ) {
		// 呼び出し側のエラーだけど、例外を投げるわけにもいかないので、そのまま終了する。
		return;
	}

	IFConcrete* p_tmp = dynamic_cast<IFConcrete*>( p );
	if ( p_tmp == nullptr ) {
		// エラーだけど、例外を投げるわけにもいかないので、そのまま終了する。
		return;
	}
	allocator_traits_type::destroy( allocator_, p_tmp );
	allocator_traits_type::deallocate( allocator_, p_tmp, 1 );
}

template <typename R, typename D, typename AllocRT>
alloc_if_abst* offset_shared_ptr_impl<R, D, AllocRT>::get_alloc_if( void ) const
{
	using concrete_alloc = alloc_if_concrete_of_Alloc<AllocRT, offset_shared_ptr_impl<R, D, AllocRT>>;
	auto p_ans           = new concrete_alloc( alloc_ );
	return p_ans;
}

}   // namespace offset_shared_ptr_detail

template <typename T>
class offset_weak_ptr;

template <typename T>
class offset_shared_ptr {
public:
	using element_type = T;
	using weak_type    = offset_weak_ptr<T>;

	~offset_shared_ptr()
	{
		offset_shared_ptr_detail::offset_shared_ptr_impl_if::try_dispose_shared( p_r_impl_ );
		p_r_impl_ = nullptr;
		p_        = nullptr;
	}

	constexpr offset_shared_ptr( void ) noexcept
	  : p_r_impl_( nullptr )
	  , p_( nullptr )
	{
	}

	template <typename Y>
	offset_shared_ptr( Y* p_arg )
	  : offset_shared_ptr( p_arg, std::default_delete<Y>(), std::allocator<Y>() )
	{
	}

	template <typename Y, typename D>
	offset_shared_ptr( Y* p_arg, D del )
	  : offset_shared_ptr( p_arg, del, std::allocator<Y>() )
	{
	}

	template <typename Y, typename D, typename Alloc>
	offset_shared_ptr( Y* p_arg, D del, Alloc alc )
	  : p_r_impl_( nullptr )
	  , p_( p_arg )
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		using concrete_type     = offset_shared_ptr_detail::offset_shared_ptr_impl<Y, D, Alloc>;
		using impl_alloc        = std::allocator<concrete_type>;
		using impl_alloc_traits = std::allocator_traits<impl_alloc>;

		impl_alloc     cur_alloc;
		concrete_type* p_tmp = nullptr;
		try {
			p_tmp = impl_alloc_traits::allocate( cur_alloc, 1 );
		} catch ( ... ) {
			del( p_arg );
			throw;
		}
		try {
			impl_alloc_traits::construct( cur_alloc, p_tmp, p_arg, del, alc );
			p_tmp->get_ascer().ref().shrd_refc_++;
		} catch ( ... ) {
			impl_alloc_traits::deallocate( cur_alloc, p_tmp, 1 );
			del( p_arg );
			throw;
		}
		p_r_impl_ = p_tmp;
	}

	offset_shared_ptr( const offset_shared_ptr& orig )
	  : p_r_impl_( nullptr )
	  , p_( nullptr )
	{
		if ( orig.p_r_impl_ == nullptr ) return;

		auto ac   = orig.p_r_impl_->get_ascer();
		p_r_impl_ = orig.p_r_impl_;
		p_        = orig.p_;
		ac.ref().shrd_refc_++;
	}

	template <typename Y>
	offset_shared_ptr( const offset_shared_ptr<Y>& orig )
	  : p_r_impl_( nullptr )
	  , p_( nullptr )
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		if ( orig.p_r_impl_ == nullptr ) return;

		auto ac   = orig.p_r_impl_->get_ascer();
		p_r_impl_ = orig.p_r_impl_;
		p_        = orig.p_.get();
		ac.ref().shrd_refc_++;
	}

	template <typename Y>
	offset_shared_ptr( const offset_weak_ptr<Y>& orig );

	offset_shared_ptr( offset_shared_ptr&& orig )
	  : p_r_impl_( nullptr )
	  , p_( nullptr )
	{
		if ( orig.p_r_impl_ == nullptr ) return;

		p_r_impl_      = orig.p_r_impl_;
		p_             = orig.p_;
		orig.p_r_impl_ = nullptr;
		orig.p_        = nullptr;
	}

	template <typename Y>
	offset_shared_ptr( offset_shared_ptr<Y>&& orig )
	  : p_r_impl_( nullptr )
	  , p_( nullptr )
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		if ( orig.p_r_impl_ == nullptr ) return;

		p_r_impl_      = orig.p_r_impl_;
		p_             = orig.p_.get();
		orig.p_r_impl_ = nullptr;
		orig.p_        = nullptr;
	}

	template <typename Y, typename Deleter>
	offset_shared_ptr( offset_unique_ptr<Y, Deleter>&& orig )
	  : p_r_impl_( nullptr )
	  , p_( nullptr )
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		offset_shared_ptr( orig.release(), orig.get_deleter() ).swap( *this );
	}

	offset_shared_ptr& operator=( const offset_shared_ptr& orig )
	{
		if ( this == &orig ) return *this;
		if ( p_r_impl_ == orig.p_r_impl_ ) return *this;

		offset_shared_ptr( orig ).swap( *this );

		return *this;
	}

	template <typename Y>
	offset_shared_ptr& operator=( const offset_shared_ptr<Y>& orig )
	{
		if ( this == &orig ) return *this;
		if ( p_r_impl_ == orig.p_r_impl_ ) return *this;

		offset_shared_ptr( orig ).swap( *this );

		return *this;
	}

	offset_shared_ptr& operator=( offset_shared_ptr&& orig )
	{
		if ( this == &orig ) return *this;
		if ( p_r_impl_ == orig.p_r_impl_ ) return *this;

		offset_shared_ptr( std::move( orig ) ).swap( *this );

		return *this;
	}

	template <typename Y>
	offset_shared_ptr& operator=( offset_shared_ptr<Y>&& orig )
	{
		if ( this == &orig ) return *this;
		if ( p_r_impl_ == orig.p_r_impl_ ) return *this;

		offset_shared_ptr( std::move( orig ) ).swap( *this );

		return *this;
	}

	void swap( offset_shared_ptr& b ) noexcept
	{
		if ( this == &b ) return;

		element_type* p_backup = p_;
		p_                     = b.p_;
		b.p_                   = p_backup;

		offset_shared_ptr_detail::offset_shared_ptr_impl_if* p_r_impl_backup = p_r_impl_;
		p_r_impl_                                                            = b.p_r_impl_;
		b.p_r_impl_                                                          = p_r_impl_backup;

		return;
	}

	void reset( void ) noexcept
	{
		offset_shared_ptr().swap( *this );
	}

	template <typename Y>
	void reset( Y* p_arg )
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		offset_shared_ptr<element_type>( p_arg ).swap( *this );
	}

	template <typename Y, typename D>
	void reset( Y* p_arg, D del )
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		offset_shared_ptr<element_type>( p_arg, del ).swap( *this );
	}

	template <typename Y, typename D, typename Alloc>
	void reset( Y* p_arg, D del, Alloc alc )
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		offset_shared_ptr<element_type>( p_arg, del, alc ).swap( *this );
	}

	element_type* get( void ) const noexcept
	{
		return p_.get();
	}

	element_type& operator*() const noexcept
	{
		return *( p_.get() );
	}

	element_type* operator->() const noexcept
	{
		return p_.get();
	}

	element_type& operator[]( ptrdiff_t i ) const
	{
		return get()[i];
	}

	long use_count() const noexcept
	{
		if ( p_r_impl_ == nullptr ) return 0;
		return p_r_impl_->get_ascer().ref().shrd_refc_;
	}

	explicit operator bool() const noexcept
	{
		return ( p_ != nullptr );
	}

	template <class U>
	bool owner_before( const offset_shared_ptr<U>& b ) const noexcept
	{
		return ( p_r_impl_ < b.p_r_impl_ );
	}

private:
	offset_ptr<offset_shared_ptr_detail::offset_shared_ptr_impl_if> p_r_impl_;
	offset_ptr<element_type>                                        p_;

	template <typename U>
	friend class offset_shared_ptr;
	template <typename U>
	friend class offset_weak_ptr;
};

template <typename T>
class offset_weak_ptr {
public:
	using element_type = T;

	~offset_weak_ptr()
	{
		offset_shared_ptr_detail::offset_shared_ptr_impl_if::try_dispose_weak( p_r_impl_ );
		p_r_impl_ = nullptr;
		p_        = nullptr;
	}

	constexpr offset_weak_ptr( void ) noexcept
	  : p_r_impl_( nullptr )
	  , p_( nullptr )
	{
	}

	offset_weak_ptr( const offset_weak_ptr& r ) noexcept
	  : p_r_impl_( r.p_r_impl_ )
	  , p_( r.p_ )
	{
		if ( p_r_impl_ == nullptr ) return;

		p_r_impl_->get_ascer().ref().weak_refc_++;
	}

	template <class Y>
	offset_weak_ptr( const offset_weak_ptr<Y>& r ) noexcept
	  : p_r_impl_( r.p_r_impl_ )
	  , p_( r.p_.get() )   // Y*からT*への暗黙変換
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		if ( p_r_impl_ == nullptr ) return;

		p_r_impl_->get_ascer().ref().weak_refc_++;
	}

	template <class Y>
	offset_weak_ptr( const offset_shared_ptr<Y>& r ) noexcept
	  : p_r_impl_( r.p_r_impl_ )
	  , p_( r.p_.get() )   // Y*からT*への暗黙変換
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		if ( p_r_impl_ == nullptr ) return;

		p_r_impl_->get_ascer().ref().weak_refc_++;
	}

	offset_weak_ptr( offset_weak_ptr&& r ) noexcept = default;

	template <class Y>
	offset_weak_ptr( offset_weak_ptr<Y>&& r ) noexcept
	  : p_r_impl_( r.p_r_impl_ )
	  , p_( r.p_.get() )   // Y*からT*への暗黙変換
	{
		static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

		r.p_r_impl_ = nullptr;
		r.p_        = nullptr;
	}

	offset_weak_ptr& operator=( const offset_weak_ptr& r ) noexcept
	{
		if ( this == &r ) return *this;
		if ( p_r_impl_ == r.p_r_impl_ ) return *this;

		offset_weak_ptr( r ).swap( *this );

		return *this;
	}

	template <class Y>
	offset_weak_ptr& operator=( const offset_weak_ptr<Y>& r ) noexcept
	{
		offset_weak_ptr( r ).swap( *this );

		return *this;
	}

	template <class Y>
	offset_weak_ptr& operator=( const offset_shared_ptr<Y>& r ) noexcept
	{
		offset_weak_ptr( r ).swap( *this );

		return *this;
	}

	offset_weak_ptr& operator=( offset_weak_ptr&& r ) noexcept
	{
		if ( this == &r ) return *this;
		if ( p_r_impl_ == r.p_r_impl_ ) return *this;

		offset_weak_ptr( std::move( r ) ).swap( *this );

		return *this;
	}

	template <class Y>
	offset_weak_ptr& operator=( offset_weak_ptr<Y>&& r ) noexcept
	{
		offset_weak_ptr( std::move( r ) ).swap( *this );

		return *this;
	}

	void swap( offset_weak_ptr& r ) noexcept
	{
		if ( this == &r ) return;

		offset_shared_ptr_detail::offset_shared_ptr_impl_if* p_tmp_impl_ = p_r_impl_;
		element_type*                                        p_tmp       = p_;

		p_r_impl_ = r.p_r_impl_;
		p_        = r.p_;

		r.p_r_impl_ = p_tmp_impl_;
		r.p_        = p_tmp;
	}

	void reset( void ) noexcept
	{
		offset_weak_ptr().swap( *this );
	}

	long use_count( void ) const noexcept
	{
		if ( p_r_impl_ == nullptr ) return 0L;
		auto ac = p_r_impl_->get_ascer();
		return ac.ref().shrd_refc_;
	}

	bool expired( void ) const noexcept
	{
		return ( use_count() == 0 );
	}

	offset_shared_ptr<T> lock( void ) const noexcept
	{
		offset_shared_ptr<T> ans;

		if ( p_r_impl_ == nullptr ) return ans;

		auto ac = p_r_impl_->get_ascer();

		if ( ac.ref().shrd_refc_ == 0 ) return ans;

		ans.p_r_impl_ = p_r_impl_;
		ans.p_        = p_;
		ac.ref().shrd_refc_++;

		return ans;
	}

	template <class U>
	bool owner_before( const offset_shared_ptr<U>& b ) const noexcept
	{
		return ( p_r_impl_ < b.p_r_impl_ );
	}

	template <class U>
	bool owner_before( const offset_weak_ptr<U>& b ) const noexcept
	{
		return ( p_r_impl_ < b.p_r_impl_ );
	}

private:
	offset_ptr<offset_shared_ptr_detail::offset_shared_ptr_impl_if> p_r_impl_;
	offset_ptr<element_type>                                        p_;

	template <typename U>
	friend class offset_weak_ptr;
	template <typename U>
	friend class offset_shared_ptr;
};

template <typename T, typename... Args>
offset_shared_ptr<T> make_offset_shared( Args&&... args )
{
	// 仮実装
	return offset_shared_ptr<T>( new T( std::forward<Args>( args )... ) );
}

template <typename T>
template <typename Y>
offset_shared_ptr<T>::offset_shared_ptr( const offset_weak_ptr<Y>& orig )
  : p_r_impl_( nullptr )
  , p_( nullptr )
{
	static_assert( std::is_convertible<Y*, T*>::value, "Y* should be convertible to T*" );

	if ( orig.p_r_impl_ == nullptr ) throw std::bad_weak_ptr();

	auto ac = orig.p_r_impl_->get_ascer();
	if ( ac.ref().shrd_refc_ == 0 ) throw std::bad_weak_ptr();

	p_r_impl_ = orig.p_r_impl_;
	p_        = orig.p_;
	ac.ref().shrd_refc_++;
}

}   // namespace ipsm

#endif   // OFFSET_SHARED_PTR_HPP_
