/**
 * @file logger.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2023-12-24
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef PROCSHARED_LOGGER_HPP_
#define PROCSHARED_LOGGER_HPP_

enum class psm_log_lv {
	kTest,
	kDebug,
	kInfo,
	kWarn,
	kErr
};

constexpr bool psm_filter( psm_log_lv ll )
{
	bool is_output = false;
	switch ( ll ) {
		case psm_log_lv::kTest:
		case psm_log_lv::kInfo:
		case psm_log_lv::kWarn:
		case psm_log_lv::kErr: {
			is_output = true;
		} break;

		case psm_log_lv::kDebug:
		default: {
		} break;
	}

	return is_output;
}

template <typename... Args>
inline void psm_logoutput( psm_log_lv ll, const char* p_fmt, Args... args )
{
	if ( psm_filter( ll ) ) {
		fprintf( stderr, p_fmt, args... );
		fprintf( stderr, "\n" );
	}
}

template <>
inline void psm_logoutput( psm_log_lv ll, const char* p_fmt )
{
	if ( psm_filter( ll ) ) {
		fprintf( stderr, "%s\n", p_fmt );
	}
}

#endif   // PROCSHARED_LOGGER_HPP_