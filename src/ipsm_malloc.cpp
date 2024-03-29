/**
 * @file ipsm_malloc.cpp
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

#include "ipsm_logger.hpp"
#include "ipsm_malloc.hpp"

namespace ipsm {

constexpr unsigned int default_channel_size = 2;

unsigned int ipsm_malloc::channel_size( void )
{
	return default_channel_size;
}

struct msg_channel {
	using data_type = offset_ptr<void>;
	ipsm_mutex                                    mtx_;
	ipsm_condition_variable_monotonic             cond_;
	offset_list<data_type, offset_allocator<data_type>> msgch_[default_channel_size];

	msg_channel( const offset_allocator<data_type> a )
	  : mtx_()
	  , cond_()
	  , msgch_ { offset_list<data_type, offset_allocator<data_type>>( a ), offset_list<data_type, offset_allocator<data_type>>( a ) }
	{
	}
};

ipsm_malloc::ipsm_malloc( void )
  : shm_obj_()
  , shm_heap_()
  , p_msgch_( nullptr )
{
}

ipsm_malloc::~ipsm_malloc()
{
	shm_obj_.set_teardown(
		[this]( bool final_teardown, void* p_mem, size_t len ) {
			shm_heap_ = offset_malloc();
		} );
	shm_obj_ = ipsm_mem();
	p_msgch_ = nullptr;   // p_msgch_の実体は、shm_obj_内に存在しているため、とくに解放処理は行わない。
}

ipsm_malloc& ipsm_malloc::operator=( ipsm_malloc&& src )
{
	if ( this == &src ) return *this;

	// メンバ変数の開放順を守るため、デストラクタの処理を利用する。
	ipsm_malloc( std::move( src ) ).swap( *this );

	return *this;
}

ipsm_malloc::ipsm_malloc( const char* p_shm_name, const char* p_id_dirname, size_t length, mode_t mode )
  : shm_obj_()
  , shm_heap_()
  , p_msgch_( nullptr )
{
	shm_obj_ = ipsm_mem(
		p_shm_name, p_id_dirname, length, mode,
		[this]( void* p_mem, size_t len ) -> void* {
			shm_heap_ = offset_malloc( p_mem, len );
			offset_allocator<msg_channel>      msg_channel_allocator_obj( shm_heap_ );
			offset_allocator<offset_ptr<void>> int_allocator_obj( shm_heap_ );

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
ipsm_malloc::allocate( size_t n, size_t alignment )
{
	return shm_heap_.allocate( n, alignment );
}

void ipsm_malloc::deallocate( void* p, size_t alignment )
{
	shm_heap_.deallocate( p, alignment );
}

void ipsm_malloc::swap( ipsm_malloc& src )
{
	shm_obj_.swap( src.shm_obj_ );
	shm_heap_.swap( src.shm_heap_ );
	std::swap( p_msgch_, src.p_msgch_ );
}

int ipsm_malloc::get_bind_count( void ) const
{
	return shm_obj_.get_bind_count();
}

void ipsm_malloc::send( unsigned int ch, offset_ptr<void> sending_value )
{
	if ( p_msgch_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::send(), p_msgch_ of ipsm_malloc is nullptr" );
		return;
	}
	if ( ch >= default_channel_size ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::send(), ch is too big, ch=%u", ch );
		return;
	}

	std::lock_guard<ipsm_mutex> lk( p_msgch_->mtx_ );
	p_msgch_->msgch_[ch].emplace_back( sending_value );
	return;
}
offset_ptr<void> ipsm_malloc::receive( unsigned int ch )
{
	if ( p_msgch_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::receive(), p_msgch_ of ipsm_malloc is nullptr" );
		// TODO: should throw exception?
		return nullptr;
	}
	if ( ch >= default_channel_size ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::send(), ch is too big, ch=%u", ch );
		return nullptr;
	}

	std::unique_lock<ipsm_mutex> lk( p_msgch_->mtx_ );
	p_msgch_->cond_.wait( lk, [this, ch]() -> bool {
		return !( p_msgch_->msgch_[ch].empty() );
	} );
	offset_ptr<void> ans = p_msgch_->msgch_[ch].front();
	p_msgch_->msgch_[ch].pop_front();
	return ans;
}

}   // namespace ipsm
