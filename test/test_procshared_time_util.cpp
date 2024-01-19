/**
 * @file test_procshared_mutex.cpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <chrono>
#include <ctime>

#include <pthread.h>

#include "gtest/gtest.h"

#include "procshared_time_util.hpp"

TEST( Test_procshared_time_util_meta_func, CanConvert )
{
	static_assert( time_util::is_allowed_convertion<std::chrono::steady_clock, CLOCK_MONOTONIC>::value, "OK" );
	static_assert( time_util::is_allowed_convertion<std::chrono::steady_clock, CLOCK_MONOTONIC_COARSE>::value, "OK" );
	static_assert( time_util::is_allowed_convertion<std::chrono::steady_clock, CLOCK_MONOTONIC_RAW>::value, "OK" );
	static_assert( !time_util::is_allowed_convertion<std::chrono::steady_clock, CLOCK_REALTIME>::value, "Not OK" );
	static_assert( !time_util::is_allowed_convertion<std::chrono::steady_clock, CLOCK_REALTIME_COARSE>::value, "Not OK" );

	static_assert( !time_util::is_allowed_convertion<std::chrono::system_clock, CLOCK_MONOTONIC>::value, "OK" );
	static_assert( !time_util::is_allowed_convertion<std::chrono::system_clock, CLOCK_MONOTONIC_COARSE>::value, "OK" );
	static_assert( !time_util::is_allowed_convertion<std::chrono::system_clock, CLOCK_MONOTONIC_RAW>::value, "OK" );
	static_assert( time_util::is_allowed_convertion<std::chrono::system_clock, CLOCK_REALTIME>::value, "Not OK" );
	static_assert( time_util::is_allowed_convertion<std::chrono::system_clock, CLOCK_REALTIME_COARSE>::value, "Not OK" );
}

TEST( Test_procshared_time_util_timespec_ct, CanConstruct_CanDestruct_MONOTONIC )
{
	ASSERT_NO_THROW( time_util::timespec_monotonic sut );
}

TEST( Test_procshared_time_util_timespec_ct, CanConstruct_CanDestruct_MONOTONIC_From_steady_clock1 )
{
	ASSERT_NO_THROW( time_util::timespec_monotonic sut( std::chrono::steady_clock::now() ) );
}

TEST( Test_procshared_time_util_timespec_ct, CanConstruct_CanDestruct_MONOTONIC_From_steady_clock2 )
{
	// Arrange
	time_util::timespec_monotonic sut;

	// Act
	ASSERT_NO_THROW( sut = time_util::timespec_monotonic( std::chrono::steady_clock::now() ) );

	// Assert
	auto diff = time_util::timespec_monotonic::now() - sut;
	EXPECT_LE( diff, std::chrono::milliseconds( 50 ) );
	EXPECT_LE( std::chrono::milliseconds( 0 ), diff );
}

TEST( Test_procshared_time_util_timespec_ct, CanConstruct_CanDestruct_REALTIME )
{
	ASSERT_NO_THROW( time_util::timespec_realtime sut );
}
