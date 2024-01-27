/**
 * @file procshared_mem_internal.hpp
 * @author PFA03027@nifty.com
 * @brief async open/close shared memory object
 * @version 0.1
 * @date 2023-12-17
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef MISC_UTILITY_HPP_
#define MISC_UTILITY_HPP_

#include <stdexcept>
#include <type_traits>

#include <errno.h>

#include <sys/types.h>
#include <unistd.h>

using type_of_errno = typename std::remove_reference<decltype( errno )>::type;

class procshared_mem_error : public std::runtime_error {
public:
	procshared_mem_error( type_of_errno e_v );
	procshared_mem_error( type_of_errno e_v, const std::string& additional_error_str );
	procshared_mem_error( const char* p_error_str );
	procshared_mem_error( const std::string& error_str );
};

static_assert( std::is_integral<ino_t>::value, "ino_t is not integral..." );
static_assert( std::is_integral<off_t>::value, "off_t is not integral..." );

std::string make_strerror( type_of_errno e_v );

ino_t get_inode_of_fd( int id_f_fd );

#endif   // MISC_UTILITY_HPP_
