/**
 * @file procshared_malloc.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2023-12-24
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include "procshared_malloc.hpp"

procshared_malloc::procshared_malloc( void )
  : shm_obj_()
  , shm_heap_()
{
}

procshared_malloc::~procshared_malloc()
{
}

procshared_malloc::procshared_malloc( const char* p_shm_name, const char* p_id_dirname, size_t length, mode_t mode )
{
	shm_obj_ = procshared_mem(
		p_shm_name, p_id_dirname, length, mode,
		[this]( void* p_mem, size_t len ) {
			shm_heap_ = offset_malloc( p_mem, len );
		},
		[this]( void* p_mem, size_t len ) {
			shm_heap_ = offset_malloc( p_mem );
		},
		[this]( void* p_mem, size_t len ) {
		} );
}

#if __has_cpp_attribute( nodiscard )
[[nodiscard]]
#endif
void*
procshared_malloc::allocate( size_t n )
{
	return shm_heap_.allocate( n );
}

void procshared_malloc::deallocate( void* p, size_t n )
{
	shm_heap_.deallocate( p, n );
}
