/**
 * @file ipsm_logger.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-14
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <cstdio>

#include "ipsm_logger.hpp"

namespace ipsm {

void default_ipsm_logger( psm_log_lv, const char* p_log ) noexcept
{
	fputs( p_log, stderr );
	fputs( "\n", stderr );
}

void ( *p_ipsm_logger )( psm_log_lv, const char* ) = default_ipsm_logger;

void set_psm_logger( void ( *p_logger )( psm_log_lv, const char* ) )
{
	if ( p_logger == nullptr ) {
		// fputs( "ipsm::set_psm_logger() is called with nullptr\n", stderr );
		p_ipsm_logger = default_ipsm_logger;
	} else {
		p_ipsm_logger = p_logger;
	}
}

void call_logger( psm_log_lv ll, const char* p_log ) noexcept
{
	try {
		p_ipsm_logger( ll, p_log );
	} catch ( ... ) {
		// ロガーで例外が発生しても、例外を投げないようにする。
		// 例外が発生した場合は、stderrに出力する。
		fprintf( stderr, "Exception occurred in logger: %s\n", p_log );
	}
}

}   // namespace ipsm
