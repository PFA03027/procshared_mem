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

#include "procshared_mem.hpp"

using namespace ipsm;

const char* p_shm_obj_name = "/my_test_shm_obj";

int main( void )
{
	unsigned char exit_code = 1;
	{
		// child process side
		try {
			procshared_mem shm_obj(
				p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
				[]( void* p_mem, size_t len ) -> void* {
					std::atomic<unsigned char>* p_data = reinterpret_cast<std::atomic<unsigned char>*>( p_mem );
					p_data->store( 122 );
					return nullptr;
				},
				[]( void*, size_t ) { /* 何もしない */ } );
			shm_obj.set_teardown(
				[]( bool final_teardown, void* p_mem, size_t len ) {
					if ( final_teardown ) {
						std::atomic<unsigned char>* p_data = reinterpret_cast<std::atomic<unsigned char>*>( p_mem );
						p_data->store( 123 );
					}
				} );
			if ( not shm_obj.debug_test_integrity() ) {
				fprintf( stderr, "debug_test_integrity() return false\n" );
				abort();
			}
			std::atomic<unsigned char>* p_data = reinterpret_cast<std::atomic<unsigned char>*>( shm_obj.get() );
			exit_code                          = p_data->load();
			// std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		} catch ( std::runtime_error& e ) {
			fprintf( stderr, "procshared_mem throws std::runtime_error %s\n", e.what() );
			abort();
		}
		if ( exit_code != 122 ) {
			fprintf( stderr, "shared memory data is not expected %d\n", static_cast<int>( exit_code ) );
		}
	}

	return exit_code;
}
