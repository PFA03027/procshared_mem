/**
 * @file procshared_malloc.cpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-12-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <memory>
#include <type_traits>

#include "procshared_logger.hpp"
#include "procshared_malloc.hpp"

struct msg_channel {
	procshared_mutex                        mtx_;
	procshared_condition_variable           cond_;
	offset_list<int, offset_allocator<int>> msgch_[2];

	msg_channel( const offset_allocator<int> a )
	  : mtx_()
	  , cond_()
	  , msgch_ { offset_list<int, offset_allocator<int>>( a ), offset_list<int, offset_allocator<int>>( a ) }
	{
	}
};

procshared_malloc::procshared_malloc( void )
  : shm_obj_()
  , shm_heap_()
  , p_msgch_( nullptr )
{
}

procshared_malloc::~procshared_malloc()
{
	shm_obj_.set_teardown(
		[this]( bool final_teardown, void* p_mem, size_t len ) {
			shm_heap_ = offset_malloc();
		} );
	shm_obj_ = procshared_mem();
	p_msgch_ = nullptr;   // p_msgch_の実体は、shm_obj_内に存在しているため、とくに解放処理は行わない。
}

procshared_malloc& procshared_malloc::operator=( procshared_malloc&& src )
{
	if ( this == &src ) return *this;

	// メンバ変数の開放順を守るため、デストラクタの処理を利用する。
	procshared_malloc( std::move( src ) ).swap( *this );

	return *this;
}

procshared_malloc::procshared_malloc( const char* p_shm_name, const char* p_id_dirname, size_t length, mode_t mode )
  : shm_obj_()
  , shm_heap_()
  , p_msgch_( nullptr )
{
	shm_obj_ = procshared_mem(
		p_shm_name, p_id_dirname, length, mode,
		[this]( void* p_mem, size_t len ) -> void* {
			shm_heap_ = offset_malloc( p_mem, len );
			offset_allocator<msg_channel> msg_channel_allocator_obj( shm_heap_ );
			offset_allocator<int>         int_allocator_obj( shm_heap_ );

			using target_allocator_traits_type = std::allocator_traits<offset_allocator<msg_channel>>;

			p_msgch_ = target_allocator_traits_type::allocate( msg_channel_allocator_obj, 1 );
			target_allocator_traits_type::construct( msg_channel_allocator_obj, p_msgch_, int_allocator_obj );

			return reinterpret_cast<void*>( p_msgch_ );
		},
		[this]( void* p_mem, size_t len ) {
			shm_heap_ = offset_malloc( p_mem );
		} );

	if ( p_msgch_ == nullptr ) {
		p_msgch_ = reinterpret_cast<msg_channel*>( shm_obj_.get_opt_info() );
	}
}

#if __has_cpp_attribute( nodiscard )
[[nodiscard]]
#endif
void*
procshared_malloc::allocate( size_t n, size_t alignment )
{
	return shm_heap_.allocate( n, alignment );
}

void procshared_malloc::deallocate( void* p, size_t alignment )
{
	shm_heap_.deallocate( p, alignment );
}

void procshared_malloc::swap( procshared_malloc& src )
{
	shm_obj_.swap( src.shm_obj_ );
	shm_heap_.swap( src.shm_heap_ );
	std::swap( p_msgch_, src.p_msgch_ );
}

int procshared_malloc::get_bind_count( void ) const
{
	return shm_obj_.get_bind_count();
}

void procshared_malloc::send( int ch, int sending_value )
{
	if ( p_msgch_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in procshared_malloc::send(), p_msgch_ of procshared_malloc is nullptr" );
		return;
	}

	std::lock_guard<procshared_mutex> lk( p_msgch_->mtx_ );
	p_msgch_->msgch_[ch].emplace_back( sending_value );
	return;
}
int procshared_malloc::receive( int ch )
{
	if ( p_msgch_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in procshared_malloc::receive(), p_msgch_ of procshared_malloc is nullptr" );
		// TODO: should throw exception?
		return 0;
	}

	std::unique_lock<procshared_mutex> lk( p_msgch_->mtx_ );
	p_msgch_->cond_.wait( lk, [this, ch]() -> bool {
		return !( p_msgch_->msgch_[ch].empty() );
	} );
	int ans = p_msgch_->msgch_[ch].front();
	p_msgch_->msgch_[ch].pop_front();
	return ans;
}
