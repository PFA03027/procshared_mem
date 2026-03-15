/**
 * @file test_offset_shared_ptr.cpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-11-19
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <array>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "gtest/gtest.h"

#include "offset_shared_ptr.hpp"

#include "test_ipsm_common.hpp"

struct DerivedArrowOpTest2 : public ArrowOpTest {
	std::atomic<int> at_z_;
};

TEST( OffsetSharedWeakHighLoad, CanDoMultConstruct )
{
	// Arrange
	constexpr unsigned int max_threads = 20;
	std::atomic<bool>      loop_flag( true );

	ipsm::offset_shared_ptr<DerivedArrowOpTest2> sp_sut = ipsm::make_offset_shared<DerivedArrowOpTest2>();

	// Act
	std::array<std::thread, max_threads>    ta;
	std::array<std::uintptr_t, max_threads> loop_count_results;
	for ( unsigned int j = 0; j < max_threads; j++ ) {
		ta[j] = std::thread(
			[&sp_sut, &loop_flag]( std::uintptr_t* p_loop_count ) {
				*p_loop_count = 0;
				while ( loop_flag.load() ) {
					ipsm::offset_shared_ptr<DerivedArrowOpTest2> sp_tx( sp_sut );
					sp_tx->at_z_++;
					( *p_loop_count )++;
				}
			},
			&loop_count_results[j] );
	}
	std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
	loop_flag.store( false );

	// Assert
	for ( auto& e : ta ) {
		if ( e.joinable() ) {
			e.join();
		}
	}
	std::uintptr_t total_loop_count = 0;
	for ( auto& e : loop_count_results ) {
		total_loop_count += e;
	}
	EXPECT_EQ( sp_sut.use_count(), 1 );
	EXPECT_EQ( sp_sut->at_z_.load(), total_loop_count );
}

TEST( OffsetSharedWeakHighLoad, CanDoMultConstruct2 )
{
	// Arrange
	constexpr unsigned int max_threads = 20;
	std::atomic<bool>      loop_flag( true );

	ipsm::offset_shared_ptr<DerivedArrowOpTest2> sp_sut = ipsm::make_offset_shared<DerivedArrowOpTest2>();
	EXPECT_EQ( sp_sut.use_count(), 1 );
	ipsm::offset_weak_ptr<DerivedArrowOpTest2> wp_sut( sp_sut );
	EXPECT_EQ( wp_sut.use_count(), 1 );

	// Act
	std::array<std::thread, max_threads>    ta;
	std::array<std::uintptr_t, max_threads> loop_count_results;
	for ( unsigned int j = 0; j < max_threads; j++ ) {
		ta[j] = std::thread(
			[wp_sut, &loop_flag]( std::uintptr_t* p_loop_count ) {
				*p_loop_count = 0;
				while ( loop_flag.load() ) {
					ipsm::offset_shared_ptr<DerivedArrowOpTest2> sp_tx( wp_sut );
					sp_tx->at_z_++;
					( *p_loop_count )++;
				}
			},
			&loop_count_results[j] );
	}
	std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
	loop_flag.store( false );

	// Assert
	for ( auto& e : ta ) {
		if ( e.joinable() ) {
			e.join();
		}
	}
	std::uintptr_t total_loop_count = 0;
	for ( auto& e : loop_count_results ) {
		total_loop_count += e;
	}
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_EQ( sp_sut->at_z_.load(), total_loop_count );
	sp_sut.reset();
	EXPECT_EQ( wp_sut.use_count(), 0 );
}
