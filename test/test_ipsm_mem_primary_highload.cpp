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

using namespace ipsm;

const char*   p_shm_obj_name = "/my_test_shm_obj";
constexpr int num_of_threads = 100;

void closeall_except_stdinouterr( void )
{
	int maxfd = static_cast<int>( sysconf( _SC_OPEN_MAX ) );
	for ( int fd = 0; fd < maxfd; ++fd ) {
		if ( ( fd == STDIN_FILENO ) || ( fd == STDOUT_FILENO ) || ( fd == STDERR_FILENO ) ) continue;
		close( fd );
	}
}

void make_shm_and_close( void )
{
	pid_t child_pid = fork();
	if ( child_pid == 0 ) {
		// closeall_except_stdinouterr();
		execl( "build/test/loadtest_ipsm_mem_secondary_highload", "loadtest_ipsm_mem_secondary_highload", reinterpret_cast<char*>( NULL ) );
		perror( "fail execl to launch loadtest_ipsm_mem_secondary_highload\n" );
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
			unsigned char exit_code = static_cast<unsigned char>( WEXITSTATUS( wstatus_code ) );
			if ( exit_code != 122 ) {
				fprintf( stderr, "shared memory data is incorrect. rcv data is %d\n", static_cast<int>( exit_code ) );
				abort();
			}
		} else {
			if ( WIFSIGNALED( wstatus_code ) ) {
				auto signum = WTERMSIG( wstatus_code );
				fprintf( stderr, "child process has exited by signal(%d)\n", static_cast<int>( signum ) );
			}
			fprintf( stderr, "child process has exited abnormally\n" );
			abort();
		}
	}
}

void test_func( void )
{
	std::thread thread_pool[num_of_threads];

	for ( int i = 0; i < num_of_threads; i++ ) {
		thread_pool[i] = std::thread( make_shm_and_close );
		// printf( "thread_pool[%d]\n", i );
	}
	for ( int i = 0; i < num_of_threads; i++ ) {
		thread_pool[i].join();
	}
}

int main( int argc, char* args[] )
{
	printf( "%s\n", args[0] );
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data

	ipsm_mem shm_obj(
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
	printf( "%s\n", shm_obj.debug_dump_string().c_str() );

	for ( int i = 0; i < 10000; i++ ) {
		if ( ( i == 0 ) || ( ( i % 1000 ) == 999 ) || ( i < 999 ) ) {
			printf( "count=%d\n", i );
			fflush( NULL );
		}
		test_func();
	}

	return EXIT_SUCCESS;
}
