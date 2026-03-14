/**
 * @file ipsm_mem.cpp
 * @author PFA03027@nifty.com
 * @brief async open/close shared memory object
 * @version 0.1
 * @date 2023-09-03
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <cerrno>

#include <stdexcept>
#include <string>
#include <type_traits>

#include <string.h>

#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

#include "ipsm_logger_internal.hpp"
#include "misc_utility.hpp"

namespace ipsm {

std::string make_strerror( type_of_errno e_v )
{
#if ( _POSIX_C_SOURCE >= 200112L ) && !_GNU_SOURCE
	// int strerror_r(int errnum, char *buf, size_t buflen);
	//             /* XSI-compliant */
	char str_buff[128];
	int  ret = strerror_r( e_v, str_buff, 128 );
	if ( ret == 0 ) {
		return std::string( str_buff );
	} else {
		return std::string( "errno=" ) + std::to_string( e_v ) + std::string( " (strerror_r() is fail)" );
	}
#else
	// char *strerror_r(int errnum, char *buf, size_t buflen);
	//             /* GNU-specific */
	char        str_buff[128];
	const char* p_ret = strerror_r( e_v, str_buff, 128 );
	if ( p_ret != NULL ) {
		return std::string( "(errno=" ) + std::to_string( e_v ) + std::string( ")" ) + std::string( p_ret );
	} else {
		return std::string( "errno=" ) + std::to_string( e_v ) + std::string( " (strerror_r() is fail)" );
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////
ipsm_mem_error::ipsm_mem_error( type_of_errno e_v )
  : std::runtime_error( make_strerror( e_v ) )
  , e_( e_v )
{
}
ipsm_mem_error::ipsm_mem_error( type_of_errno e_v, const std::string& additional_error_str )
  : std::runtime_error( make_strerror( e_v ) + additional_error_str )
  , e_( e_v )
{
}
ipsm_mem_error::ipsm_mem_error( const char* p_error_str )
  : std::runtime_error( p_error_str )
  , e_( 0 )
{
}
ipsm_mem_error::ipsm_mem_error( const std::string& error_str )
  : std::runtime_error( error_str )
  , e_( 0 )
{
}

}   // namespace ipsm
