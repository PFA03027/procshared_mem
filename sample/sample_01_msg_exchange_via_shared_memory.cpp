/**
 * @file sample_msg_exchange_via_shared_memory.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-15
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "ipsm_malloc.hpp"

ipsm::ipsm_malloc setup_shared_memory( void )
{
	// Create a shared memory object with the name "/my_shared_memory", a lifetime control file named "lifetime_control_file", a size of 1024 bytes, and read/write permissions for the owner and group.
	ipsm::ipsm_malloc shared_memory(
		"/my_shared_memory",
		"/tmp/lifetime_control_file",
		1024,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	return shared_memory;
}

void allocate_and_send_message( ipsm::ipsm_malloc& shared_memory )
{
	// Allocate memory for a message in the shared memory
	const char* message        = "Hello from process via 0ch!";
	size_t      message_length = std::strlen( message ) + 1;   // Include null terminator

	void* p_allocated_memory = shared_memory.allocate( message_length );
	if ( p_allocated_memory == nullptr ) {
		throw std::runtime_error( "Failed to allocate memory in shared memory" );
	}

	// Copy the message to the allocated memory
	std::memcpy( p_allocated_memory, message, message_length );

	// Send the message using the shared memory's send function (assuming channel 0 is used for communication)
	shared_memory.send( 0, p_allocated_memory );
}

void receive_and_print_message( ipsm::ipsm_malloc& shared_memory )
{
	// Receive the message using the shared memory's receive function (assuming channel 0 is used for communication)
	ipsm::offset_ptr<void> received_message_ptr = shared_memory.receive( 0 );

	// Cast the received message pointer to a char* and print the message
	const char* received_message = received_message_ptr.reinterpret_to_offset_ptr<const char>().get();
	std::printf( "Received message: %s\n", received_message );
}

void fork_and_run( void )
{
	pid_t pid = fork();
	if ( pid < 0 ) {
		throw std::runtime_error( "Failed to fork process" );
	} else if ( pid == 0 ) {
		// Child process: allocate and send message
		ipsm::ipsm_malloc shared_memory_child_side = setup_shared_memory();
		allocate_and_send_message( shared_memory_child_side );
		exit( EXIT_SUCCESS );
	}
}

int main( void )
{
	try {
		ipsm::ipsm_malloc shared_memory = setup_shared_memory();
		fork_and_run();
		receive_and_print_message( shared_memory );
	} catch ( const std::exception& e ) {
		std::fprintf( stderr, "Error: %s\n", e.what() );
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}