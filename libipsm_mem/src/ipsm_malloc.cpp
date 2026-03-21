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

#include "ipsm_condition_variable.hpp"
#include "ipsm_logger_internal.hpp"
#include "ipsm_malloc.hpp"

namespace ipsm {

struct msg_channels {
	using data_type              = offset_ptr<void>;
	using channel_container_type = offset_list<data_type, offset_allocator<data_type>>;

	const size_t                      channel_size_;
	ipsm_condition_variable_monotonic cond_;
	ipsm_mutex                        mtx_;
	channel_container_type            msgch_[0];

	msg_channels( const offset_allocator<data_type> a, size_t channel_size_arg )
	  : channel_size_( channel_size_arg )
	  , cond_()
	  , mtx_()
	  , msgch_ {}
	{
		for ( size_t i = 0; i < channel_size_arg; ++i ) {
			new ( &msgch_[i] ) channel_container_type( a );
		}
	}

	static size_t calc_required_bytes( size_t channel_size_arg );
};

size_t msg_channels::calc_required_bytes( size_t channel_size_arg )
{
	size_t required_bytes = sizeof( msg_channels ) + sizeof( channel_container_type ) * channel_size_arg;
	return required_bytes;
}

// ======================================================================

ipsm_malloc::impl::~impl()
{
}
ipsm_malloc::impl::impl( void )
  : shm_obj_()
  , shm_heap_()
  , p_msgch_( nullptr )
{
}

ipsm_malloc::impl::impl(
	const char* p_shm_name,              //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
	const char* p_lifetime_ctrl_fname,   //!< [in] lifetime control file name.
	size_t      length,                  //!< [in] shared memory size
	mode_t      mode,                    //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	size_t      channel_size,            //!< [in] the number of channels for message passing. this value must be agreed upon in advance between communicating processes.
	int         timeout_msec,            //!< [in] timeout in milliseconds for waiting for shared memory initialization.
	int         retry_interval_msec      //!< [in] retry interval in milliseconds for waiting for shared memory initialization.
	)
  : shm_obj_()
  , shm_heap_()
  , p_msgch_( nullptr )
{
	size_t actual_request_length = length + msg_channels::calc_required_bytes( channel_size ) + alignof( msg_channels );
	bool   setup_ret             = shm_obj_.setup(
        p_shm_name, p_lifetime_ctrl_fname, actual_request_length, mode,
        [channel_size]( void* p_mem, size_t len ) -> std::uintptr_t {
            offset_malloc                      shm_heap_setup = offset_malloc( p_mem, len );
            offset_allocator<msg_channels>     msg_channels_allocator_obj( shm_heap_setup );
            offset_allocator<offset_ptr<void>> chdata_t_allocator_obj( shm_heap_setup );

            using target_allocator_traits_type = std::allocator_traits<offset_allocator<msg_channels>>;

            // msg_channels* p_msgch_setup = target_allocator_traits_type::allocate( msg_channels_allocator_obj, 1 );
            msg_channels* p_msgch_setup = reinterpret_cast<msg_channels*>( shm_heap_setup.allocate( msg_channels::calc_required_bytes( channel_size ), alignof( msg_channels ) ) );
            target_allocator_traits_type::construct( msg_channels_allocator_obj, p_msgch_setup, chdata_t_allocator_obj, channel_size );

            std::uintptr_t p_msgch_offset = reinterpret_cast<std::uintptr_t>( p_msgch_setup ) - reinterpret_cast<std::uintptr_t>( p_mem );

            return p_msgch_offset;   // セカンダリ側に通知する情報は、message channelへのオフセット。
        },
        timeout_msec, retry_interval_msec );

	if ( !setup_ret ) {
		psm_logoutput( ipsm::psm_log_lv::kWarn, "fail to construct offset_malloc on shared memory: %s", p_shm_name );
		throw std::runtime_error( "fail to construct offset_malloc on shared memory: " + std::string( p_shm_name ) );
	}

	shm_heap_ = offset_malloc( shm_obj_.get() );   // setup()では、必ずしもコールバック関数が呼び出されるとは限らないため、get()で取得したアドレスを利用して、改めてoffset_mallocを初期化する。
	p_msgch_  = reinterpret_cast<msg_channels*>( reinterpret_cast<std::uintptr_t>( shm_obj_.get() ) + reinterpret_cast<std::uintptr_t>( shm_obj_.get_hint_value() ) );
}

#if __has_cpp_attribute( nodiscard )
[[nodiscard]]
#endif
void* ipsm_malloc::impl::allocate( size_t n, size_t alignment )
{
	return shm_heap_.allocate( n, alignment );
}

void ipsm_malloc::impl::deallocate( void* p, size_t alignment )
{
	shm_heap_.deallocate( p, alignment );
}

int ipsm_malloc::impl::get_bind_count( void ) const
{
	return shm_heap_.get_bind_count();
}

void ipsm_malloc::impl::send( unsigned int ch, offset_ptr<void> sending_value )
{
	if ( ch >= p_msgch_->channel_size_ ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::send(), ch is too big, requested ch=%u, actual channel_size=%u", ch, p_msgch_->channel_size_ );
		return;
	}

	{
		std::lock_guard<ipsm_mutex> lk( p_msgch_->mtx_ );
		p_msgch_->msgch_[ch].emplace_back( sending_value );
	}
	p_msgch_->cond_.notify_all();
	return;
}
offset_ptr<void> ipsm_malloc::impl::receive( unsigned int ch )
{
	if ( p_msgch_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::receive(), p_msgch_ of ipsm_malloc is nullptr" );
		// TODO: should throw exception?
		return nullptr;
	}
	if ( ch >= p_msgch_->channel_size_ ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::receive(), ch is too big, requested ch=%u, actual channel_size=%u", ch, p_msgch_->channel_size_ );
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

std::optional<offset_ptr<void>> ipsm_malloc::impl::try_receive( unsigned int ch )
{
	if ( p_msgch_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::try_receive(), p_msgch_ of ipsm_malloc is nullptr" );
		return std::nullopt;
	}
	if ( ch >= p_msgch_->channel_size_ ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::try_receive(), ch is too big, requested ch=%u, actual channel_size=%u", ch, p_msgch_->channel_size_ );
		return std::nullopt;
	}

	std::lock_guard<ipsm_mutex> lk( p_msgch_->mtx_ );
	if ( p_msgch_->msgch_[ch].empty() ) {
		return std::nullopt;
	}
	offset_ptr<void> ans = p_msgch_->msgch_[ch].front();
	p_msgch_->msgch_[ch].pop_front();
	return ans;
}

std::optional<offset_ptr<void>> ipsm_malloc::impl::try_receive_until( unsigned int ch, const time_util::timespec_monotonic& abs_timeout_time )
{
	if ( p_msgch_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::try_receive_until(), p_msgch_ of ipsm_malloc is nullptr" );
		return std::nullopt;
	}
	if ( ch >= p_msgch_->channel_size_ ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::try_receive_until(), ch is too big, requested ch=%u, actual channel_size=%u", ch, p_msgch_->channel_size_ );
		return std::nullopt;
	}

	std::unique_lock<ipsm_mutex> lk( p_msgch_->mtx_ );
	bool                         has_msg = p_msgch_->cond_.wait_until( lk, abs_timeout_time, [this, ch]() -> bool {
        return !( p_msgch_->msgch_[ch].empty() );
    } );
	if ( !has_msg ) {
		return std::nullopt;
	}
	offset_ptr<void> ans = p_msgch_->msgch_[ch].front();
	p_msgch_->msgch_[ch].pop_front();
	return ans;
}

size_t ipsm_malloc::impl::channel_size( void ) const
{
	if ( p_msgch_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::channel_size(), p_msgch_ of ipsm_malloc is nullptr" );
		return 0;
	}
	return p_msgch_->channel_size_;
}

// ======================================================================

void ipsm_malloc::swap( ipsm_malloc& src )
{
	sp_impl_.swap( src.sp_impl_ );
}

ipsm_malloc::ipsm_malloc(
	const char* p_shm_name,
	const char* p_lifetime_ctrl_fname,
	size_t      length,
	mode_t      mode,
	size_t      channel_size,
	int         timeout_msec,
	int         retry_interval_msec )
  : sp_impl_( std::make_shared<impl>( p_shm_name, p_lifetime_ctrl_fname, length, mode, channel_size, timeout_msec, retry_interval_msec ) )
{
}

#if __has_cpp_attribute( nodiscard )
[[nodiscard]]
#endif
void* ipsm_malloc::allocate( size_t n, size_t alignment )
{
	if ( sp_impl_ == nullptr ) return nullptr;
	return sp_impl_->allocate( n, alignment );
}

void ipsm_malloc::deallocate( void* p, size_t alignment )
{
	if ( sp_impl_ == nullptr ) return;
	sp_impl_->deallocate( p, alignment );
}

int ipsm_malloc::get_bind_count( void ) const
{
	if ( sp_impl_ == nullptr ) return 0;
	return sp_impl_->get_bind_count();
}

void ipsm_malloc::send( unsigned int ch, offset_ptr<void> sending_value )
{
	if ( sp_impl_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::send(), sp_impl_ of ipsm_malloc is nullptr" );
		return;
	}
	sp_impl_->send( ch, sending_value );
	return;
}
offset_ptr<void> ipsm_malloc::receive( unsigned int ch )
{
	if ( sp_impl_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::receive(), sp_impl_ of ipsm_malloc is nullptr" );
		return nullptr;
	}
	return sp_impl_->receive( ch );
}

std::optional<offset_ptr<void>> ipsm_malloc::try_receive( unsigned int ch )
{
	if ( sp_impl_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::try_receive(), sp_impl_ of ipsm_malloc is nullptr" );
		return nullptr;
	}
	return sp_impl_->try_receive( ch );
}

std::optional<offset_ptr<void>> ipsm_malloc::try_receive_until( unsigned int ch, const time_util::timespec_monotonic& abs_timeout_time )
{
	if ( sp_impl_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::try_receive_until(), sp_impl_ of ipsm_malloc is nullptr" );
		return nullptr;
	}
	return sp_impl_->try_receive_until( ch, abs_timeout_time );
}

size_t ipsm_malloc::channel_size( void ) const
{
	if ( sp_impl_ == nullptr ) {
		psm_logoutput( psm_log_lv::kErr, "Error: in ipsm_malloc::channel_size(), sp_impl_ of ipsm_malloc is nullptr" );
		return 0;
	}
	return sp_impl_->channel_size();
}

}   // namespace ipsm
