/**
 * @file test_ipsm_mem.cpp
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

#include "ipsm_mem.hpp"

const char* p_shm_obj_name        = "/my_test_shm_obj";
const char* p_lifetime_ctrl_fname = "/tmp/my_test_shm_obj_lifetime_ctrl";

int main( void )
{
	ipsm::ipsm_mem shm_obj;
	unsigned char  exit_code = 1;

	// child process side
	try {
		shm_obj.setup(
			p_shm_obj_name,
			p_lifetime_ctrl_fname,
			1024,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			[]( void* p, size_t s ) -> size_t { return 0; },
			1000,
			1 );

		if ( shm_obj.get_status() != ipsm::ipsm_mem::status::ready ) {
			fprintf( stderr, "shared memory is not ready\n" );
			abort();
		}
		exit_code = 122;
	} catch ( std::runtime_error& e ) {
		fprintf( stderr, "ipsm_mem throws std::runtime_error %s\n", e.what() );
		abort();
	}
	if ( exit_code != 122 ) {
		fprintf( stderr, "shared memory data is not expected %d\n", static_cast<int>( exit_code ) );
	}

	return exit_code;
}
