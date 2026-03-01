/**
 * @file ipsm_time_util.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2024-01-19
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <cerrno>

#include <chrono>
#include <stdexcept>
#include <system_error>

#include <pthread.h>

#include "ipsm_time_util.hpp"

namespace ipsm {

namespace time_util {
namespace internal {

struct timespec call_clock_gettime( const clockid_t clkid )
{
	struct timespec cur_time;
	int             ret = clock_gettime( clkid, &cur_time );
	if ( ret != 0 ) {
		auto            cur_errno = errno;
		std::error_code ec( cur_errno, std::system_category() );
		switch ( cur_errno ) {
			case EFAULT: {
				throw std::system_error( ec, "clock_gettime() return error(EFAULT). this may happened memory crash. this should not happen" );
			} break;
			case EINVAL: {
				std::string msg = "clock_gettime() return error(EINVAL). invalid clock_id may be used. clkid=";
				msg += std::to_string( clkid );
				throw std::system_error( ec, msg );
			} break;

			case EPERM: {
				throw std::system_error( ec, "clock_gettime() return error(EPERM). this should not happen" );
			} break;

			default: {
				throw std::system_error( ec, " clock_gettime() return unknown error code" );
			} break;
		}
	}

	return cur_time;
}

}   // namespace internal
}   // namespace time_util
}   // namespace ipsm
