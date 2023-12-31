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

#include "offset_krmalloc.hpp"
#include "offset_malloc.hpp"

/**
 * @brief memory allocator that implemented by base class
 *
 * このクラスは、メモリ領域を所有しない設計としているため、placement_new()で指定された領域にクラス構造を構築する。
 */
class offset_malloc::offset_mem_malloc_impl : public offset_mem_krmalloc {
public:
	class deleter {
	public:
		void operator()( offset_malloc::offset_mem_malloc_impl* p_mem )
		{
			offset_malloc::offset_mem_malloc_impl::teardown( p_mem );
		}
	};

	static offset_mem_malloc_impl* placement_new( void* begin_pointer, void* end_pointer )
	{
		if ( begin_pointer == nullptr ) {
			throw std::bad_alloc();
		}
		if ( end_pointer == nullptr ) {
			throw std::bad_alloc();
		}
		if ( begin_pointer >= end_pointer ) {
			throw std::bad_alloc();
		}
		if ( ( reinterpret_cast<uintptr_t>( begin_pointer ) + sizeof( offset_mem_malloc_impl ) ) >= reinterpret_cast<uintptr_t>( end_pointer ) ) {
			throw std::bad_alloc();
		}
		return new ( begin_pointer ) offset_mem_malloc_impl( end_pointer );
	}
	static offset_mem_malloc_impl* bind( void* p_mem )
	{
		return reinterpret_cast<offset_mem_malloc_impl*>( p_mem );
	}

private:
	static void teardown( offset_mem_malloc_impl* p_mem )
	{
		if ( p_mem == nullptr ) return;

		p_mem->~offset_mem_malloc_impl();
	}

	offset_mem_malloc_impl( void* end_pointer )
	  : offset_mem_krmalloc( end_pointer )
	{
	}
	~offset_mem_malloc_impl() = default;

	offset_mem_malloc_impl( const offset_mem_malloc_impl& )            = delete;
	offset_mem_malloc_impl( offset_mem_malloc_impl&& )                 = delete;
	offset_mem_malloc_impl& operator=( const offset_mem_malloc_impl& ) = delete;
	offset_mem_malloc_impl& operator=( offset_mem_malloc_impl&& )      = delete;
};

offset_malloc::offset_malloc( void )
  : sp_impl_( nullptr )
{
}

offset_malloc::offset_malloc( void* p_mem, size_t mem_bytes )
  : sp_impl_( offset_mem_malloc_impl::placement_new( p_mem, reinterpret_cast<void*>( reinterpret_cast<uintptr_t>( p_mem ) + mem_bytes ) ),
              offset_malloc::offset_mem_malloc_impl::deleter {} )
{
}

offset_malloc::~offset_malloc()
{
}

void* offset_malloc::allocate( size_t req_bytes, size_t alignment )
{
	if ( sp_impl_ == nullptr ) return nullptr;

	return sp_impl_->allocate( req_bytes, alignment );
}
void offset_malloc::deallocate( void* p, size_t alignment )
{
	if ( sp_impl_ == nullptr ) return;

	sp_impl_->deallocate( p, alignment );
}
