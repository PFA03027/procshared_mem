/**
 * @file test_ipsm_logger.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-14
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <string>

#include "ipsm_logger.hpp"
#include "ipsm_logger_internal.hpp"

#include <gtest/gtest.h>

struct log_test_record {
	bool             is_called_;
	ipsm::psm_log_lv log_lv_;
	std::string      log_;
};

log_test_record g_log_test_record;

void reset_log_test_record()
{
	g_log_test_record.is_called_ = false;
	g_log_test_record.log_lv_    = ipsm::psm_log_lv::kTest;
	g_log_test_record.log_.clear();
}

void test_logger( ipsm::psm_log_lv ll, const char* p_log )
{
	g_log_test_record.is_called_ = true;
	g_log_test_record.log_lv_    = ll;
	g_log_test_record.log_       = p_log;
}

TEST( Test_ipsm_logger, CanSetPSMLoggerAndCallWithNoArguments )
{
	// Arrange
	reset_log_test_record();

	// Act
	ipsm::set_psm_logger( test_logger );

	// Assert
	psm_logoutput( ipsm::psm_log_lv::kInfo, "Test log message" );
	EXPECT_TRUE( g_log_test_record.is_called_ );
	EXPECT_EQ( g_log_test_record.log_lv_, ipsm::psm_log_lv::kInfo );
	EXPECT_EQ( g_log_test_record.log_, "Test log message" );

	// Cleanup
	ipsm::set_psm_logger( nullptr );
}

TEST( Test_ipsm_logger, CanSetPSMLoggerAndCallWithTheArguments )
{
	// Arrange
	int a = 10;
	reset_log_test_record();

	// Act
	ipsm::set_psm_logger( test_logger );

	// Assert
	psm_logoutput( ipsm::psm_log_lv::kErr, "Test log message %d", a );
	EXPECT_TRUE( g_log_test_record.is_called_ );
	EXPECT_EQ( g_log_test_record.log_lv_, ipsm::psm_log_lv::kErr );
	EXPECT_EQ( g_log_test_record.log_, "Test log message 10" );

	// Cleanup
	ipsm::set_psm_logger( nullptr );
}

TEST( Test_ipsm_logger, CanSetPSMLoggerWithNullptr_ThenBackToDefaultLogger )
{
	// Arrange
	reset_log_test_record();
	ipsm::set_psm_logger( test_logger );
	int a = 10;
	psm_logoutput( ipsm::psm_log_lv::kErr, "Test log message %d", a );
	EXPECT_TRUE( g_log_test_record.is_called_ );
	reset_log_test_record();

	// Act
	ipsm::set_psm_logger( nullptr );

	// Assert
	psm_logoutput( ipsm::psm_log_lv::kInfo, "This should go to default logger (stderr)" );
	EXPECT_FALSE( g_log_test_record.is_called_ );
}
