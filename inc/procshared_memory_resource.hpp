/**
 * @file procshared_memory_resource.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2023-10-21
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef PROCSHARED_MEMORY_RESOURCE_HPP
#define PROCSHARED_MEMORY_RESOURCE_HPP

#include <cstddef>
#include <memory_resource>

#include "procshared_malloc.hpp"

class procshared_memory_resource : public std::pmr::memory_resource {
public:
	~procshared_memory_resource() noexcept = default;

	constexpr procshared_memory_resource() noexcept
	  : p_malloc_( nullptr )
	{
	}

	constexpr procshared_memory_resource( const procshared_memory_resource& ) noexcept   = default;
	constexpr procshared_memory_resource& operator=( const procshared_memory_resource& ) = default;

private:
	void* do_allocate( std::size_t bytes, std::size_t alignment ) override
	{
		if ( p_malloc_ == nullptr ) {
			throw std::bad_alloc();
		}

		return p_malloc_->allocate( bytes, alignment );
	}

	void do_deallocate( void* p, std::size_t bytes, std::size_t alignment = alignof( std::max_align_t ) ) override
	{
		if ( p_malloc_ == nullptr ) {
			// TODO: メモリーリークになるが、とりあえず何もしない。例外を投げるとか(bad_free?)、エラーログを出力するとかできるけど。
			return;
		}
		p_malloc_->deallocate( p );
	}

	bool do_is_equal( const memory_resource& other ) const noexcept override
	{
		const procshared_memory_resource* p_other = dynamic_cast<const procshared_memory_resource*>( &other );
		if ( p_other == nullptr ) {
			return false;
		}

		return ( p_malloc_ == p_other->p_malloc_ );
	}

	procshared_mem_malloc* p_malloc_;
};

#endif   // PROCSHARED_MEMORY_RESOURCE_HPP
