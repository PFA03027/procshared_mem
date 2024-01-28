/**
 * @file test_procshared_mem.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-03
 *
 * @copyright Copyright (c) 2023-2024
 *
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "procshared_malloc.hpp"

using namespace ipsm;

const char* p_shm_obj_name = "/my_test_shm_malloc_highload";

int main( void )
{
	unsigned char exit_code = 1;
	{
		// child process side
		try {
			procshared_malloc shm_heap( p_shm_obj_name, "/tmp", 1024UL * 100UL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
			exit_code = 2;
			for ( int i = 0; i < 100; i++ ) {
				exit_code = 3;
				auto p    = shm_heap.allocate( 11 );
				exit_code = 4;
				if ( p == nullptr ) {
					fprintf( stderr, "shm_heap return nullptr\n" );
					return exit_code;
				}
				shm_heap.deallocate( p );
				exit_code = 5;
			}
			exit_code = 122;
		} catch ( std::runtime_error& e ) {
			fprintf( stderr, "procshared_malloc throws std::runtime_error: %s, code=%d\n", e.what(), exit_code );
			abort();
		}
		if ( exit_code != 122 ) {
			fprintf( stderr, "procshared_malloc is not expected %d\n", static_cast<int>( exit_code ) );
		}
	}

	return exit_code;
}
