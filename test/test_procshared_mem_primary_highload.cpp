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

void make_shm_and_close( void )
{
	pid_t child_pid = fork();
	if ( child_pid == 0 ) {
		execl( "build/test/test_procshared_mem_secondary_highload", "test_procshared_mem_secondary_highload", (char*)NULL );
		perror( "fail execl to launch test_procshared_mem_secondary_highload\n" );
		abort();
	} else {
		// parent process side
		if ( child_pid < 0 ) {
			fprintf( stderr, "fail fork()\n" );
			abort();
		}
		int   wstatus_code;
		pid_t child_wait_ret = waitpid( child_pid, &wstatus_code, 0 );
		if ( child_wait_ret < 0 ) {
			fprintf( stderr, "fail waitpid()\n" );
			abort();
		}
		if ( WIFEXITED( wstatus_code ) ) {
			unsigned char exit_code = WEXITSTATUS( wstatus_code );
			if ( exit_code != 122 ) {
				fprintf( stderr, "shared memory data is incorrect. rcv data is %d\n", (int)exit_code );
				abort();
			}
		} else {
			if ( WIFSIGNALED( wstatus_code ) ) {
				auto signum = WTERMSIG( wstatus_code );
				fprintf( stderr, "child process has exited by signal(%d)\n", (int)signum );
			}
			fprintf( stderr, "child process has exited abnormally\n" );
			abort();
		}
	}
}

void test_func( void )
{
	std::thread thread_pool[num_of_threads];

	// procshared_mem shm_obj( p_shm_obj_name, 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, []( void* p_mem, off_t len ) {
	// 	std::atomic<unsigned char>* p_data = reinterpret_cast<std::atomic<unsigned char>*>( p_mem );
	// 	p_data->store( 122 );
	// } );

	for ( int i = 0; i < num_of_threads; i++ ) {
		thread_pool[i] = std::thread( make_shm_and_close );
	}
	for ( int i = 0; i < num_of_threads; i++ ) {
		thread_pool[i].join();
	}
}

int main( void )
{
	procshared_mem::debug_force_cleanup( p_shm_obj_name );   // to remove ghost data

	for ( int i = 0; i < 10000; i++ ) {
		if ( ( i == 0 ) || ( ( i % 1000 ) == 999 ) ) {
			printf( "count=%d\n", i );
		}
		test_func();
	}

	return EXIT_SUCCESS;
}