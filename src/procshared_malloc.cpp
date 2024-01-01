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

#include "procshared_malloc.hpp"

procshared_malloc::procshared_malloc( void )
  : shm_obj_()
  , shm_heap_()
{
}

procshared_malloc::~procshared_malloc()
{
}

procshared_malloc& procshared_malloc::operator=( procshared_malloc&& src )
{
	if ( this == &src ) return *this;

	// メンバ変数の開放順を守るため、デストラクタの処理を利用する。
	procshared_malloc( std::move( src ) ).swap( *this );

	return *this;
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
		[]( void* p_mem, size_t len ) {
		} );
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
}
