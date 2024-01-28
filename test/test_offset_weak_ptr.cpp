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

#include <cstdio>
#include <cstdlib>

#include "gtest/gtest.h"

#include "offset_shared_ptr.hpp"

using namespace ipsm;

struct ArrowOpTest {
	int x_;
	int y_;
};

struct DerivedArrowOpTest : public ArrowOpTest {
	int z_;
};

TEST( TestOffsetWeakPtr, CanDoDefaultConstruct )
{
	// Arrange

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 0 );
	EXPECT_TRUE( wp_sut.expired() );
}

TEST( TestOffsetWeakPtr, CanDoCopyConstructWithNull1 )
{
	// Arrange
	offset_weak_ptr<ArrowOpTest> wp_t1;

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 0 );
	EXPECT_TRUE( wp_sut.expired() );
}

TEST( TestOffsetWeakPtr, CanDoCopyConstructWithNull2 )
{
	// Arrange
	offset_weak_ptr<DerivedArrowOpTest> wp_t1;

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 0 );
	EXPECT_TRUE( wp_sut.expired() );
}

TEST( TestOffsetWeakPtr, CanDoTransConstructFromShared )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( sp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
}

TEST( TestOffsetWeakPtr, CanDoCopyConstructWithValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyConstructWithValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>          wp_t1( sp_t1 );

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyConstructWithValidResource3 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<DerivedArrowOpTest>   wp_t1( sp_t1 );

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyConstructWithValidResource_Then_Reset1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( wp_t1 );
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );

	// Act
	wp_sut.reset();

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 0 );
	EXPECT_TRUE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
}

TEST( TestOffsetWeakPtr, CanDoCopyConstructWithValidResource_Then_Reset2 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( wp_t1 );
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );

	// Act
	sp_t1.reset();

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 0 );
	EXPECT_TRUE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );
	EXPECT_TRUE( wp_t1.expired() );
}

TEST( TestOffsetWeakPtr, CanDoMoveConstructWithValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( std::move( wp_t1 ) );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveConstructWithValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>          wp_t1( sp_t1 );

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( std::move( wp_t1 ) );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveConstructWithValidResource3 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<DerivedArrowOpTest>   wp_t1( sp_t1 );

	// Act
	offset_weak_ptr<ArrowOpTest> wp_sut( std::move( wp_t1 ) );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentWithValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut;

	// Act
	wp_sut = wp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentWithValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>          wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>          wp_sut;

	// Act
	wp_sut = wp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentWithValidResource3 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<DerivedArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>          wp_sut;

	// Act
	wp_sut = wp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentWithValidToValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_t2( p2 );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( sp_t2 );

	// Act
	wp_sut = wp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentWithValidToValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p1 = new DerivedArrowOpTest;
	ArrowOpTest*                          p2 = new ArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest>        sp_t2( p2 );
	offset_weak_ptr<ArrowOpTest>          wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>          wp_sut( sp_t2 );

	// Act
	wp_sut = wp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentWithValidToValidResource3 )
{
	// Arrange
	DerivedArrowOpTest*                   p1 = new DerivedArrowOpTest;
	ArrowOpTest*                          p2 = new ArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest>        sp_t2( p2 );
	offset_weak_ptr<DerivedArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>          wp_sut( sp_t2 );

	// Act
	wp_sut = wp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 1 );
	EXPECT_FALSE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentFromSharedWithValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>   wp_sut;

	// Act
	wp_sut = sp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentFromSharedWithValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>          wp_sut;

	// Act
	wp_sut = sp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentFromSharedWithValidToValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_t2( p2 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( sp_t2 );

	// Act
	wp_sut = sp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoCopyAssignmentFromSharedWithValidToValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p1 = new DerivedArrowOpTest;
	ArrowOpTest*                          p2 = new ArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest>        sp_t2( p2 );
	offset_weak_ptr<ArrowOpTest>          wp_sut( sp_t2 );

	// Act
	wp_sut = sp_t1;

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentWithValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut;

	// Act
	wp_sut = std::move( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentWithValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>          wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>          wp_sut;

	// Act
	wp_sut = std::move( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentWithValidResource3 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<DerivedArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>          wp_sut;

	// Act
	wp_sut = std::move( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentWithValidToValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_t2( p2 );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( sp_t2 );

	// Act
	wp_sut = std::move( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentWithValidToValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p1 = new DerivedArrowOpTest;
	ArrowOpTest*                          p2 = new ArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest>        sp_t2( p2 );
	offset_weak_ptr<ArrowOpTest>          wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>          wp_sut( sp_t2 );

	// Act
	wp_sut = std::move( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentWithValidToValidResource3 )
{
	// Arrange
	DerivedArrowOpTest*                   p1 = new DerivedArrowOpTest;
	ArrowOpTest*                          p2 = new ArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest>        sp_t2( p2 );
	offset_weak_ptr<DerivedArrowOpTest>   wp_t1( sp_t1 );
	offset_weak_ptr<ArrowOpTest>          wp_sut( sp_t2 );

	// Act
	wp_sut = std::move( wp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( wp_t1.use_count(), 0 );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
	EXPECT_TRUE( wp_t1.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

#if 0
TEST( TestOffsetWeakPtr, CanDoMoveAssignmentFromSharedWithValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>   wp_sut;

	// Act
	wp_sut = std::move( sp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentFromSharedWithValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );
	offset_weak_ptr<ArrowOpTest>          wp_sut;

	// Act
	wp_sut = std::move( sp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentFromSharedWithValidToValidResource1 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_t2( p2 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( sp_t2 );

	// Act
	wp_sut = std::move( sp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}

TEST( TestOffsetWeakPtr, CanDoMoveAssignmentFromSharedWithValidToValidResource2 )
{
	// Arrange
	DerivedArrowOpTest*                   p1 = new DerivedArrowOpTest;
	ArrowOpTest*                          p2 = new ArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest>        sp_t2( p2 );
	offset_weak_ptr<ArrowOpTest>          wp_sut( sp_t2 );

	// Act
	wp_sut = std::move( sp_t1 );

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 1 );
	EXPECT_FALSE( wp_sut.expired() );
	EXPECT_EQ( sp_t1.use_count(), 1 );
}
#endif

TEST( TestOffsetWeakPtr, CanDoDefaultConstruct_Then_Lock )
{
	// Arrange
	offset_weak_ptr<ArrowOpTest> wp_sut;

	// Act
	offset_shared_ptr<ArrowOpTest> sp_ret = wp_sut.lock();

	// Assert
	EXPECT_EQ( sp_ret.use_count(), 0 );
	EXPECT_EQ( sp_ret.get(), nullptr );
}

TEST( TestOffsetWeakPtr, ValidWeakPtr_Then_Lock )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( sp_t1 );

	// Act
	offset_shared_ptr<ArrowOpTest> sp_ret = wp_sut.lock();

	// Assert
	EXPECT_EQ( sp_ret.use_count(), 2 );
	EXPECT_EQ( sp_ret.get(), p1 );
}

TEST( TestOffsetWeakPtr, ValidWeakPtr_Then_Reset1 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( sp_t1 );

	// Act
	wp_sut.reset();

	// Assert
	EXPECT_EQ( sp_t1.use_count(), 1 );
	EXPECT_EQ( sp_t1.get(), p1 );
}

TEST( TestOffsetWeakPtr, ValidWeakPtr_Then_Reset2 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_weak_ptr<ArrowOpTest>   wp_sut( sp_t1 );

	// Act
	sp_t1.reset();

	// Assert
	EXPECT_EQ( wp_sut.use_count(), 0 );
}

TEST( TestOffsetWeakPtr, CanDoOwnerBefore )
{
	// Arrange
	ArrowOpTest*                          p1 = new ArrowOpTest;
	DerivedArrowOpTest*                   p2 = new DerivedArrowOpTest;
	offset_shared_ptr<ArrowOpTest>        sp_t1( p1 );
	offset_shared_ptr<DerivedArrowOpTest> sp_sut( p2 );
	offset_weak_ptr<ArrowOpTest>          wp_t1( sp_t1 );
	offset_weak_ptr<DerivedArrowOpTest>   wp_sut( sp_sut );

	// Act

	// Assert
	EXPECT_NO_THROW( sp_sut.owner_before( sp_t1 ) );
	EXPECT_FALSE( wp_t1.owner_before( sp_t1 ) );
	EXPECT_NO_THROW( wp_sut.owner_before( sp_t1 ) );
	EXPECT_NO_THROW( wp_t1.owner_before( sp_sut ) );
	EXPECT_FALSE( wp_sut.owner_before( sp_sut ) );
	EXPECT_NO_THROW( wp_sut.owner_before( wp_t1 ) );
}
