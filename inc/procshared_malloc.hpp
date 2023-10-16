/**
 * @file procshared_malloc.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief malloc that is shareable b/w processes
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

/**
 * @file procshared_krmalloc.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief process sharable malloc based on K&R malloc algorithm
 * @version 0.1
 * @date 2023-10-07
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef PROCSHARED_MALLOC_HPP_
#define PROCSHARED_MALLOC_HPP_

#include <cstddef>

class procshared_mem_malloc {
public:
	procshared_mem_malloc( void* p_mem, size_t mem_bytes );
	~procshared_mem_malloc();

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void*
		 allocate( size_t req_bytes, size_t alignment = alignof( std::max_align_t ) );
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

private:
	class procshared_mem_malloc_impl;

	procshared_mem_malloc( const procshared_mem_malloc& )            = delete;
	procshared_mem_malloc( procshared_mem_malloc&& )                 = delete;
	procshared_mem_malloc& operator=( const procshared_mem_malloc& ) = delete;
	procshared_mem_malloc& operator=( procshared_mem_malloc&& )      = delete;

	procshared_mem_malloc_impl* p_impl_;
};

#endif   // PROCSHARED_MALLOC_HPP_