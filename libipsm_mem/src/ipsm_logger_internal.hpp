/**
 * @file logger.hpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-12-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef IPSM_LOGGER_INTERNAL_HPP_
#define IPSM_LOGGER_INTERNAL_HPP_

#include <cstdio>

#include "ipsm_logger.hpp"

namespace ipsm {

constexpr size_t log_buff_size = 1024;

extern void ( *p_ipsm_logger )( psm_log_lv, const char* );
void call_logger( psm_log_lv ll, const char* p_log ) noexcept;

constexpr inline bool psm_filter( psm_log_lv ll )
{
#if defined( ENABLE_DEBUG_LOGOUTPUT ) || defined( ENABLE_DELEGATION_OF_FILTERING )
	return true;
#else
	// C++11 and after
	return ( ll == psm_log_lv::kTest ) ||
	       ( ll == psm_log_lv::kInfo ) ||
	       ( ll == psm_log_lv::kWarn ) ||
	       ( ll == psm_log_lv::kErr );
#endif
}

template <typename... Args>
inline void psm_logoutput( psm_log_lv ll, const char* p_fmt, Args... args ) noexcept
{
	if ( psm_filter( ll ) ) {
		char log_buff[log_buff_size];
		snprintf( log_buff, sizeof( log_buff ), p_fmt, args... );
		call_logger( ll, log_buff );
	}
}

template <>
inline void psm_logoutput( psm_log_lv ll, const char* p_fmt ) noexcept
{
	if ( psm_filter( ll ) ) {
		char log_buff[log_buff_size];
		snprintf( log_buff, sizeof( log_buff ), "%s", p_fmt );
		call_logger( ll, log_buff );
	}
}

}   // namespace ipsm

#endif   // IPSM_LOGGER_INTERNAL_HPP_
