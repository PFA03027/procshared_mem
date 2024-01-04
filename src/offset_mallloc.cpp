/**
 * @file offset_malloc.cpp
 * @author PFA03027@nifty.com
 * @brief malloc that is shareable b/w processes
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include "offset_mallloc_impl.hpp"
#include "offset_malloc.hpp"
#include "procshared_logger.hpp"

offset_malloc::~offset_malloc()
{
	offset_malloc_impl::teardown( p_impl_ );
	p_impl_ = nullptr;
}

offset_malloc::offset_malloc( const offset_malloc& src )
  : p_impl_( offset_malloc_impl::bind( src.p_impl_ ) )
{
}

offset_malloc::offset_malloc( offset_malloc&& src )
  : p_impl_( src.p_impl_ )
{
	src.p_impl_ = nullptr;
}

offset_malloc& offset_malloc::operator=( const offset_malloc& src )
{
	if ( this == &src ) return *this;

	offset_malloc_impl::teardown( p_impl_ );
	p_impl_ = offset_malloc_impl::bind( src.p_impl_ );

	return *this;
}

offset_malloc& offset_malloc::operator=( offset_malloc&& src )
{
	if ( this == &src ) return *this;

	offset_malloc_impl::teardown( p_impl_ );
	p_impl_     = src.p_impl_;
	src.p_impl_ = nullptr;

	return *this;
}

offset_malloc::offset_malloc( void* p_mem, size_t mem_bytes )
  : p_impl_( offset_malloc_impl::placement_new( p_mem, reinterpret_cast<void*>( reinterpret_cast<uintptr_t>( p_mem ) + mem_bytes ) ) )
{
}

offset_malloc::offset_malloc( void* p_mem )
  : p_impl_( offset_malloc_impl::bind( reinterpret_cast<offset_malloc_impl*>( p_mem ) ) )
{
}

void* offset_malloc::allocate( size_t req_bytes, size_t alignment )
{
	if ( p_impl_ == nullptr ) {
		psm_logoutput( psm_log_lv::kWarn, "Warning: offset_malloc(%p) is required to allocate, but p_impl_ is nullptr", this );
		return nullptr;
	}
	return p_impl_->allocate( req_bytes, alignment );
}
void offset_malloc::deallocate( void* p, size_t alignment )
{
	if ( p_impl_ == nullptr ) {
		psm_logoutput( psm_log_lv::kWarn, "Warning: offset_malloc(%p) is required to deallocate, but p_impl_ is nullptr", this );
		return;
	}

	p_impl_->deallocate( p, alignment );
}

void offset_malloc::swap( offset_malloc& src )
{
	if ( this == &src ) return;

	offset_malloc_impl* p_tmp = p_impl_;
	p_impl_                   = src.p_impl_;
	src.p_impl_               = p_tmp;
}

int offset_malloc::get_bind_count( void ) const
{
	if ( p_impl_ == nullptr ) {
		psm_logoutput( psm_log_lv::kDebug, "Debug: p_impl_ = offset_malloc(%p) is nullptr", this );
		return 0;
	}
	return p_impl_->get_bind_count();
}
