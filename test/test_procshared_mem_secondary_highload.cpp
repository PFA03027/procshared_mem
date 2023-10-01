/**
 * @file test_procshared_mem.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-03
 *
 * @copyright Copyright (c) 2023
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

const char*   p_shm_obj_name = "/my_test_shm_obj";
constexpr int num_of_threads = 100;

int main( void )
{
	unsigned char exit_code = 1;
	{
		// child process side
		try {
			procshared_mem shm_obj( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, []( void* p_mem, off_t len ) {
				std::atomic<unsigned char>* p_data = reinterpret_cast<std::atomic<unsigned char>*>( p_mem );
				p_data->store( 122 );
			} );
			// std::cout << "inode: " << std::to_string( shm_obj.debug_get_id_file_inode() ) << std::endl;
			if ( not shm_obj.debug_test_integrity() ) {
				fprintf( stderr, "debug_test_integrity() return false\n" );
				abort();
			}
			std::atomic<unsigned char>* p_data = reinterpret_cast<std::atomic<unsigned char>*>( shm_obj.get() );
			exit_code                          = p_data->load();
		} catch ( std::runtime_error& e ) {
			fprintf( stderr, "procshared_mem throws std::runtime_error %s\n", e.what() );
			abort();
		}
		if ( exit_code != 122 ) {
			fprintf( stderr, "shared memory data is not expected %d\n", (int)exit_code );
		}
		// std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}

	return exit_code;
}
