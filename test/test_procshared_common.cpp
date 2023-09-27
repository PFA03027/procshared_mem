/**
 * @file test_procshared_common.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <system_error>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct child_proc_return_t {
	bool          is_exit_normaly_;
	unsigned char exit_code_;
	int           signal_code_;
};

child_proc_return_t call_pred_on_child_process( std::function<int()> pred_func_in_child_process )
{
	child_proc_return_t ans = { false, 0, 0 };

	pid_t child_pid = fork();
	if ( child_pid == 0 ) {
		// child process side
		exit( pred_func_in_child_process() );
	} else {
		// parent process side
		if ( child_pid < 0 ) {
			auto cur_errno = errno;
			fprintf( stderr, "fail fork()\n" );
			std::error_code ec( cur_errno, std::system_category() );
			throw std::system_error( ec, "fork() fail" );
		}
		int   wstatus_code;
		pid_t child_wait_ret = waitpid( child_pid, &wstatus_code, 0 );
		if ( child_wait_ret < 0 ) {
			auto cur_errno = errno;
			fprintf( stderr, "fail waitpid()\n" );
			std::error_code ec( cur_errno, std::system_category() );
			throw std::system_error( ec, "waitpid() fail" );
		}
		if ( WIFEXITED( wstatus_code ) ) {
			unsigned char exit_code = WEXITSTATUS( wstatus_code );
			ans.is_exit_normaly_    = true;
			ans.exit_code_          = exit_code;
		} else {
			ans.is_exit_normaly_ = false;
			if ( WIFSIGNALED( wstatus_code ) ) {
				auto signum = WTERMSIG( wstatus_code );
				fprintf( stderr, "child process has exited by signal(%d)\n", (int)signum );
				ans.signal_code_ = signum;
			}
			fprintf( stderr, "child process has exited abnormally\n" );
		}
	}

	return ans;
}
