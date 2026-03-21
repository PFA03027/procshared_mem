/**
 * @file sample_02_list_of_unique_ptr.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-18
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 * サンプルの内容
 * * ipsm::ipsm_mallocのメモリ領域上に、offset_listを構築し、プロセス間で共有する。
 * * offset_listは、offset_unique_ptrを保持する。
 * * offset_unique_ptrはipsm::ipsm_mallocから取得した領域を保持する。
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "ipsm_malloc.hpp"
#include "offset_unique_ptr.hpp"

struct msg_sample {
	int  len_;
	char str_[100];

	msg_sample()
	  : len_( 0 )
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
using sample_list_element   = ipsm::offset_unique_ptr<msg_sample, sample_deleter>;
using sample_list_allocator = ipsm::offset_allocator<sample_list_element>;
using sample_list           = ipsm::offset_list<sample_list_element, sample_list_allocator>;

sample_list* setup_shared_list( ipsm::ipsm_malloc& shared_memory )
{
	sample_list* p_ans = shared_memory.new_instance<sample_list>( sample_list_allocator( shared_memory.get_offset_malloc() ) );
	return p_ans;
}

int main( void )
{
	return EXIT_SUCCESS;
}