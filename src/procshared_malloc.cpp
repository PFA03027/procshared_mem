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

#include "procshared_malloc.hpp"
#include "procshared_krmalloc.hpp"

class procshared_mem_malloc::procshared_mem_malloc_impl {
public:
	procshared_mem_malloc_impl( void* p_mem, size_t mem_bytes )
	  : p_mem_allocator_( procshared_mem_krmalloc::make( p_mem, mem_bytes ) )
	{
		return;
	}
	~procshared_mem_malloc_impl()
	{
		procshared_mem_krmalloc::teardown( p_mem_allocator_ );
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
	procshared_mem_krmalloc* p_mem_allocator_;
};

procshared_mem_malloc::procshared_mem_malloc( void* p_mem, size_t mem_bytes )
  : p_impl_( new procshared_mem_malloc_impl( p_mem, mem_bytes ) )
{
}

procshared_mem_malloc::~procshared_mem_malloc()
{
	delete p_impl_;
	p_impl_ = nullptr;
}

void* procshared_mem_malloc::allocate( size_t req_bytes, size_t alignment )
{
	return p_impl_->allocate( req_bytes, alignment );
}
void procshared_mem_malloc::deallocate( void* p, size_t alignment )
{
	p_impl_->deallocate( p, alignment );
}
