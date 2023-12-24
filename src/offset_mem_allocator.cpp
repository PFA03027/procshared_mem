/**
 * @file procshared_malloc.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief malloc that is shareable b/w processes
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include "offset_mem_allocator.hpp"
#include "offset_krmalloc.hpp"

class offset_mem_allocator::offset_mem_malloc_impl {
public:
	offset_mem_malloc_impl( void* p_mem, size_t mem_bytes )
	  : p_mem_allocator_( offset_mem_krmalloc::make( p_mem, mem_bytes ) )
	{
		return;
	}
	~offset_mem_malloc_impl()
	{
		offset_mem_krmalloc::teardown( p_mem_allocator_ );
		p_mem_allocator_ = nullptr;
	}

	void* allocate( size_t req_bytes, size_t alignment = alignof( std::max_align_t ) )
	{
		return p_mem_allocator_->allocate( req_bytes, alignment );
	}
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) )
	{
		p_mem_allocator_->deallocate( p, alignment );
	}

private:
	offset_mem_krmalloc* p_mem_allocator_;
};

offset_mem_allocator::offset_mem_allocator( void )
  : p_impl_( nullptr )
{
}
offset_mem_allocator::offset_mem_allocator( offset_mem_allocator&& src )
  : p_impl_( src.p_impl_ )
{
	src.p_impl_ = nullptr;
}
offset_mem_allocator& offset_mem_allocator::operator=( offset_mem_allocator&& src )
{
	if ( this == &src ) return *this;

	delete p_impl_;
	p_impl_     = src.p_impl_;
	src.p_impl_ = nullptr;

	return *this;
}

offset_mem_allocator::offset_mem_allocator( void* p_mem, size_t mem_bytes )
  : p_impl_( new offset_mem_malloc_impl( p_mem, mem_bytes ) )
{
}

offset_mem_allocator::~offset_mem_allocator()
{
	delete p_impl_;
	p_impl_ = nullptr;
}

void* offset_mem_allocator::allocate( size_t req_bytes, size_t alignment )
{
	if ( p_impl_ == nullptr ) return nullptr;

	return p_impl_->allocate( req_bytes, alignment );
}
void offset_mem_allocator::deallocate( void* p, size_t alignment )
{
	if ( p_impl_ == nullptr ) return;

	p_impl_->deallocate( p, alignment );
}
