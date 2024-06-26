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

using namespace ipsm;

struct DerivedArrowOpTest2 : public ArrowOpTest {
	std::atomic<int> at_z_;
};

TEST( OffsetSharedWeakHighLoad, CanDoMultConstruct )
{
	// Arrange
	constexpr unsigned int max_loop_cnt = 100;
	constexpr unsigned int max_threads  = 500;

	offset_shared_ptr<DerivedArrowOpTest2> sp_sut = make_offset_shared<DerivedArrowOpTest2>();

	// Act
	for ( unsigned int i = 0; i < max_loop_cnt; i++ ) {
		std::array<std::thread, max_threads> ta;
		for ( unsigned int j = 0; j < max_threads; j++ ) {
			ta[j] = std::thread( [&sp_sut]( void ) {
				offset_shared_ptr<DerivedArrowOpTest2> sp_tx( sp_sut );
				sp_tx->at_z_++;
			} );
		}
		for ( auto& e : ta ) {
			if ( e.joinable() ) {
				e.join();
			}
		}
	}

	// Assert
	EXPECT_EQ( sp_sut.use_count(), 1 );
	EXPECT_EQ( sp_sut->at_z_.load(), max_loop_cnt * max_threads );
}

TEST( OffsetSharedWeakHighLoad, CanDoMultConstruct2 )
{
	// Arrange
	constexpr unsigned int max_loop_cnt = 100;
	constexpr unsigned int max_threads  = 500;

	offset_shared_ptr<DerivedArrowOpTest2> sp_sut = make_offset_shared<DerivedArrowOpTest2>();
	EXPECT_EQ( sp_sut.use_count(), 1 );
	offset_weak_ptr<DerivedArrowOpTest2> wp_sut( sp_sut );
	EXPECT_EQ( wp_sut.use_count(), 1 );

	// Act
	for ( unsigned int i = 0; i < max_loop_cnt; i++ ) {
		std::array<std::thread, max_threads> ta;
		for ( unsigned int j = 0; j < max_threads; j++ ) {
			ta[j] = std::thread( [wp_sut]( void ) {
				offset_shared_ptr<DerivedArrowOpTest2> sp_tx( wp_sut );
				sp_tx->at_z_++;
			} );
		}
		for ( auto& e : ta ) {
			if ( e.joinable() ) {
				e.join();
			}
		}
	}

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_EQ( sp_sut->at_z_.load(), max_loop_cnt * max_threads );
	sp_sut.reset();
	EXPECT_EQ( wp_sut.use_count(), 0 );
}
