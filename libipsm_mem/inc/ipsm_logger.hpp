/**
 * @file ipsm_logger.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-14
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef IPSM_LOGGER_HPP_
#define IPSM_LOGGER_HPP_

namespace ipsm {

enum class psm_log_lv {
	kTest,    //!< log level for ipsm internal test. this log level is not output by default.
	kDebug,   //!< log level for debug. this log level is not output by default.
	kInfo,    //!< log level for general information. this log level is output by default.
	kWarn,    //!< log level for warning. this log level is output by default.
	kErr      //!< log level for error. this log level is output by default.
};

/**
 * @brief Set the logger function.
 *
 * This function allows you to set a custom logger function that will be called when logging messages.
 * The logger function should be exception-safe, even if p_logger throws an exception, the logging system will not throw an exception.
 *
 * @param p_logger A pointer to the logger function. if nullptr is passed, the default logger will be used, which outputs to stderr.
 *
 * @note If you would like to handle the filtering by psm_log_lv fully, please set compile definition ENABLE_DELEGATION_OF_FILTERING
 */
void set_psm_logger( void ( *p_logger )( psm_log_lv, const char* ) );

}   // namespace ipsm

#endif   // IPSM_LOGGER_HPP_
