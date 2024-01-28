/**
 * @file test_ipsm_mutex.cpp
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

#include "ipsm_time_util.hpp"

using namespace ipsm;

TEST( Test_ipsm_time_util_meta_func, CanConvert )
{
	static_assert( time_util::is_exchangable_timediff<std::chrono::steady_clock, CLOCK_MONOTONIC>::value, "OK to exchange" );
	static_assert( time_util::is_exchangable_timediff<std::chrono::steady_clock, CLOCK_MONOTONIC_COARSE>::value, "OK to exchange" );
	static_assert( time_util::is_exchangable_timediff<std::chrono::steady_clock, CLOCK_MONOTONIC_RAW>::value, "OK to exchange" );
	static_assert( !time_util::is_exchangable_timediff<std::chrono::steady_clock, CLOCK_BOOTTIME>::value, "Not OK to exchange" );
	static_assert( !time_util::is_exchangable_timediff<std::chrono::steady_clock, CLOCK_REALTIME>::value, "Not OK to exchange" );
	static_assert( !time_util::is_exchangable_timediff<std::chrono::steady_clock, CLOCK_REALTIME_COARSE>::value, "Not OK to exchange" );

	static_assert( !time_util::is_exchangable_timediff<std::chrono::system_clock, CLOCK_MONOTONIC>::value, "OK to exchange" );
	static_assert( !time_util::is_exchangable_timediff<std::chrono::system_clock, CLOCK_MONOTONIC_COARSE>::value, "OK to exchange" );
	static_assert( !time_util::is_exchangable_timediff<std::chrono::system_clock, CLOCK_MONOTONIC_RAW>::value, "OK to exchange" );
	static_assert( !time_util::is_exchangable_timediff<std::chrono::system_clock, CLOCK_BOOTTIME>::value, "Not OK to exchange" );
	static_assert( time_util::is_exchangable_timediff<std::chrono::system_clock, CLOCK_REALTIME>::value, "Not OK to exchange" );
	static_assert( time_util::is_exchangable_timediff<std::chrono::system_clock, CLOCK_REALTIME_COARSE>::value, "Not OK to exchange" );
}

TEST( Test_ipsm_time_util_meta_func, IsSteadyClock )
{
	static_assert( decltype( time_util::is_steady_clock_impl::chk<std::chrono::steady_clock>( nullptr ) )::value, "Steady clock" );
	static_assert( !( decltype( time_util::is_steady_clock_impl::chk<std::chrono::system_clock>( nullptr ) )::value ), "Not steady clock" );

	static_assert( decltype( time_util::is_steady_clock_impl::chk<CLOCK_MONOTONIC>( nullptr ) )::value, "Steady clock" );
	static_assert( decltype( time_util::is_steady_clock_impl::chk<CLOCK_MONOTONIC_COARSE>( nullptr ) )::value, "Steady clock" );
	static_assert( decltype( time_util::is_steady_clock_impl::chk<CLOCK_MONOTONIC_RAW>( nullptr ) )::value, "Steady clock" );
	static_assert( decltype( time_util::is_steady_clock_impl::chk<CLOCK_BOOTTIME>( nullptr ) )::value, "Steady clock" );
	static_assert( !( decltype( time_util::is_steady_clock_impl::chk<CLOCK_REALTIME>( nullptr ) )::value ), "Not steady clock" );
	static_assert( !( decltype( time_util::is_steady_clock_impl::chk<CLOCK_REALTIME_COARSE>( nullptr ) )::value ), "Not steady clock" );

	// static_assert( time_util::is_steady_clock<std::chrono::steady_clock>::value, "Steady clock" );
	// static_assert( time_util::is_steady_clock<CLOCK_MONOTONIC>::value, "Steady clock" );
	// static_assert( !time_util::is_steady_clock<CLOCK_REALTIME>::value, "Not steady clock" );
}

TEST( Test_ipsm_time_util_timespec_ct, CanConstruct_CanDestruct_MONOTONIC )
{
	ASSERT_NO_THROW( time_util::timespec_monotonic sut );   // NOLINT(clang-diagnostic-unused-variable)
}

TEST( Test_ipsm_time_util_timespec_ct, CanConstruct_CanDestruct_MONOTONIC_From_steady_clock1 )
{
	ASSERT_NO_THROW( time_util::timespec_monotonic sut( std::chrono::steady_clock::now() ) );
}

TEST( Test_ipsm_time_util_timespec_ct, CanConstruct_CanDestruct_MONOTONIC_From_steady_clock2 )
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

TEST( Test_ipsm_time_util_timespec_ct, CanConvert_steady_clock2_From_MONOTONIC )
{
	// Arrange
	time_util::timespec_monotonic sut;
	ASSERT_NO_THROW( sut = time_util::timespec_monotonic( std::chrono::steady_clock::now() ) );

	// Act
	std::chrono::steady_clock::time_point ret = sut;

	// Assert
	auto diff = time_util::timespec_monotonic::now() - ret;
	EXPECT_LE( diff, std::chrono::milliseconds( 50 ) );
	EXPECT_LE( std::chrono::milliseconds( 0 ), diff );
}

TEST( Test_ipsm_time_util_timespec_ct, CanConstruct_CanDestruct_REALTIME )
{
	ASSERT_NO_THROW( time_util::timespec_realtime sut );   // NOLINT(clang-diagnostic-unused-variable)
}
