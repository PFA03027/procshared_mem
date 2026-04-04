/**
 * @file sample_03_offset_shared_ptr.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-04-04
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ipsm_malloc.hpp"
#include "offset_list.hpp"
#include "offset_shared_ptr.hpp"

struct msg_sample {
	unsigned int v_;
	char         str_[100];

	msg_sample()
	  : v_( 0 )
	  , str_ { 0 }
	{
	}
};

/**
 * @brief Set the up shared memory object b/w processes
 *
 * @return ipsm::ipsm_malloc shared memory object
 */
ipsm::ipsm_malloc setup_shared_memory( void )
{
	// Create a shared memory object with the name "/my_shared_memory", a lifetime control file named "lifetime_control_file", a size of 1024 bytes, and read/write permissions for the owner and group.
	ipsm::ipsm_malloc shared_memory(
		"/my_shared_memory",
		"/tmp/lifetime_control_file",
		1024 * 16,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	return shared_memory;
}

using sample_deleter        = ipsm::deleter_by_offset_malloc<msg_sample>;
using sample_list_element   = ipsm::offset_shared_ptr<msg_sample>;
using sample_list_allocator = ipsm::offset_allocator<sample_list_element>;
using sample_list           = ipsm::offset_list<sample_list_element, sample_list_allocator>;

unsigned char sub_process_func( void )
{
	ipsm::ipsm_malloc                     sm       = setup_shared_memory();
	std::optional<ipsm::offset_ptr<void>> opt_op_v = sm.try_receive_for( 0, std::chrono::seconds( 1 ) );
	if ( !opt_op_v.has_value() ) {
		return EXIT_FAILURE;
	}

	ipsm::offset_ptr<sample_list> op_sl   = reinterpret_cast<sample_list*>( opt_op_v->get() );
	sample_list_element           osp_msg = std::move( op_sl->front() );
	op_sl->front();

	return static_cast<unsigned char>( osp_msg->v_ );
}

pid_t fork_sub_process_func( void )
{
	pid_t child_pid = fork();
	if ( child_pid == 0 ) {
		// closeall_except_stdinouterr();
		unsigned char exit_code = sub_process_func();
		exit( exit_code );
	} else {
		// parent process side
		if ( child_pid < 0 ) {
			fprintf( stderr, "fail fork()\n" );
			abort();
		}
	}
	return child_pid;
}

unsigned char wait_sub_process( pid_t child_pid )
{
	int   wstatus_code;
	pid_t child_wait_ret = waitpid( child_pid, &wstatus_code, 0 );
	if ( child_wait_ret < 0 ) {
		fprintf( stderr, "fail waitpid()\n" );
		return 127;
	}
	if ( WIFEXITED( wstatus_code ) ) {
		unsigned char exit_code = static_cast<unsigned char>( WEXITSTATUS( wstatus_code ) );
		return exit_code;
	} else {
		if ( WIFSIGNALED( wstatus_code ) ) {
			auto signum = WTERMSIG( wstatus_code );
			fprintf( stderr, "child process has exited by signal(%d)\n", static_cast<int>( signum ) );
		}
		fprintf( stderr, "child process has exited abnormally\n" );
		abort();
	}
}

sample_list* make_sample_list_on_ipsm_mem( ipsm::ipsm_malloc& shared_memory )
{
	sample_list* p_ans = shared_memory.new_instance<sample_list>( sample_list_allocator( shared_memory.get_offset_malloc() ) );
	return p_ans;
}

int main( void )
{
	pid_t sub_proc_pid = fork_sub_process_func();

	ipsm::ipsm_malloc sample_shared_memory = setup_shared_memory();

	sample_list* p_que_list = make_sample_list_on_ipsm_mem( sample_shared_memory );

	msg_sample mm;
	mm.v_        = 42;
	auto osp_msg = ipsm::allocate_offset_shared<msg_sample>( sample_shared_memory.get_offset_malloc(), mm );
	p_que_list->push_back( std::move( osp_msg ) );

	sample_shared_memory.send( 0, p_que_list );

	unsigned char exit_code = wait_sub_process( sub_proc_pid );
	if ( exit_code != 42 ) {
		fprintf( stderr, "child process has exited with unexpected exit code\n" );
		return EXIT_FAILURE;
	}

	fprintf( stdout, "success to communicate to child process\n" );

	sample_shared_memory.delete_instance( p_que_list );

	return EXIT_SUCCESS;
}
