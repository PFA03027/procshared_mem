/**
 * @file test_offset_shared_ptr.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2023-11-19
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <cstdio>
#include <cstdlib>

#include "gtest/gtest.h"

#include "offset_shared_ptr.hpp"

struct ArrowOpTest {
	int x;
	int y;
};

struct DerivedArrowOpTest : public ArrowOpTest {
	int z;
};

TEST( TestOffsetSharedPtr, CanDoDefaultConstruct )
{
	// Arrange

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut;

	// Assert
	EXPECT_EQ( sp_sut.get(), nullptr );
	EXPECT_EQ( sp_sut.use_count(), 0 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_FALSE( tb );
}

TEST( TestOffsetSharedPtr, CanDoDefaultConstruct_Then_Reset )
{
	// Arrange
	offset_shared_ptr<ArrowOpTest> sp_sut;

	// Act
	sp_sut.reset();

	// Assert
	EXPECT_EQ( sp_sut.get(), nullptr );
	EXPECT_EQ( sp_sut.use_count(), 0 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_FALSE( tb );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTPtr )
{
	// Arrange
	ArrowOpTest* p = new ArrowOpTest;

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( p );

	// Assert
	EXPECT_EQ( sp_sut.get(), p );
	EXPECT_EQ( sp_sut.use_count(), 1 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_TRUE( tb );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTPtr_Then_Reset1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_sut( p );

	// Act
	sp_sut.reset();

	// Assert
	EXPECT_EQ( sp_sut.get(), nullptr );
	EXPECT_EQ( sp_sut.use_count(), 0 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_FALSE( tb );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTPtr_Then_Reset2_1 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_sut( p1 );

	// Act
	sp_sut.reset( p2 );

	// Assert
	EXPECT_EQ( sp_sut.get(), p2 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_TRUE( tb );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTPtr_Then_Reset2_2 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_sut( p1 );

	// Act
	sp_sut.reset( p2, std::default_delete<ArrowOpTest>() );

	// Assert
	EXPECT_EQ( sp_sut.get(), p2 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_TRUE( tb );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTPtr_Then_Reset2_3 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_sut( p1 );

	// Act
	sp_sut.reset( p2, std::default_delete<ArrowOpTest>(), std::allocator<ArrowOpTest>() );

	// Assert
	EXPECT_EQ( sp_sut.get(), p2 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_TRUE( tb );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTPtr_Then_Reset3_1 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	DerivedArrowOpTest*            p2 = new DerivedArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_sut( p1 );

	// Act
	sp_sut.reset( p2 );

	// Assert
	ArrowOpTest* p3 = p2;
	EXPECT_EQ( sp_sut.get(), p3 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_TRUE( tb );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTPtr_Then_Reset3_2 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	DerivedArrowOpTest*            p2 = new DerivedArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_sut( p1 );

	// Act
	sp_sut.reset( p2, std::default_delete<DerivedArrowOpTest>() );

	// Assert
	ArrowOpTest* p3 = p2;
	EXPECT_EQ( sp_sut.get(), p3 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_TRUE( tb );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTPtr_Then_Reset3_3 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	DerivedArrowOpTest*            p2 = new DerivedArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_sut( p1 );

	// Act
	sp_sut.reset( p2, std::default_delete<DerivedArrowOpTest>(), std::allocator<DerivedArrowOpTest>() );

	// Assert
	ArrowOpTest* p3 = p2;
	EXPECT_EQ( sp_sut.get(), p3 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_TRUE( tb );
}

TEST( TestOffsetSharedPtr, CanDoSwap1 )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p );
	offset_shared_ptr<ArrowOpTest> sp_sut;

	// Act
	sp_sut.swap( sp_t1 );

	// Assert
	EXPECT_EQ( sp_t1.get(), nullptr );
	EXPECT_EQ( sp_t1.use_count(), 0 );
	EXPECT_EQ( sp_sut.get(), p );
	EXPECT_EQ( sp_sut.use_count(), 1 );
}

TEST( TestOffsetSharedPtr, CanDoSwap2 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_sut( p2 );

	// Act
	sp_sut.swap( sp_t1 );

	// Assert
	EXPECT_EQ( sp_t1.get(), p2 );
	EXPECT_EQ( sp_t1.use_count(), 1 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTArrayPtrDeleter )
{
	// Arrange
	ArrowOpTest* p = new ArrowOpTest[10];

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( p, std::default_delete<ArrowOpTest[]>() );

	// Assert
	EXPECT_EQ( sp_sut.get(), p );
	EXPECT_EQ( sp_sut.use_count(), 1 );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTArrayPtrDeleterAlloc )
{
	// Arrange
	ArrowOpTest* p = new ArrowOpTest[10];

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( p, std::default_delete<ArrowOpTest[]>(), std::allocator<ArrowOpTest>() );

	// Assert
	EXPECT_EQ( sp_sut.get(), p );
	EXPECT_EQ( sp_sut.use_count(), 1 );
}

TEST( TestOffsetSharedPtr, CanDoConstructWithTArrayPtrDeleter_Then_Reset )
{
	// Arrange
	ArrowOpTest*                   p = new ArrowOpTest[10];
	offset_shared_ptr<ArrowOpTest> sp_sut( p, std::default_delete<ArrowOpTest[]>() );

	// Act
	sp_sut.reset();

	// Assert
	EXPECT_EQ( sp_sut.get(), nullptr );
	EXPECT_EQ( sp_sut.use_count(), 0 );
}

TEST( TestOffsetSharedPtr, CanDoOperator_Arrow )
{
	// Arrange
	ArrowOpTest* p = new ArrowOpTest;
	p->x           = 1;
	p->y           = 2;
	offset_shared_ptr<ArrowOpTest> sp_sut( p );

	// Act

	// Assert
	EXPECT_EQ( sp_sut->x, 1 );
	EXPECT_EQ( sp_sut->y, 2 );
}

TEST( TestOffsetSharedPtr, CanDoOperator_Ref )
{
	// Arrange
	ArrowOpTest* p = new ArrowOpTest;
	p->x           = 1;
	p->y           = 2;
	offset_shared_ptr<ArrowOpTest> sp_sut( p );

	// Act

	// Assert
	EXPECT_EQ( ( *sp_sut ).x, 1 );
	EXPECT_EQ( ( *sp_sut ).y, 2 );
}

TEST( TestOffsetSharedPtr, CanDoOperatorArray )
{
	// Arrange
	ArrowOpTest* p = new ArrowOpTest[2];
	p[0].x         = 1;
	p[0].y         = 2;
	p[1].x         = 3;
	p[1].y         = 4;
	offset_shared_ptr<ArrowOpTest> sp_sut( p, std::default_delete<ArrowOpTest[]>() );

	// Act

	// Assert
	EXPECT_EQ( sp_sut[0].x, 1 );
	EXPECT_EQ( sp_sut[0].y, 2 );
	EXPECT_EQ( sp_sut[1].x, 3 );
	EXPECT_EQ( sp_sut[1].y, 4 );
}

TEST( TestOffsetSharedPtr, CanDoOwnerBefore )
{
	// Arrange
	ArrowOpTest*                          p1 = new ArrowOpTest;
	DerivedArrowOpTest*                   p2 = new DerivedArrowOpTest;
	offset_shared_ptr<ArrowOpTest>        sp_t1( p1 );
	offset_shared_ptr<DerivedArrowOpTest> sp_sut( p2 );

	// Act

	// Assert
	EXPECT_NO_THROW( sp_sut.owner_before( sp_t1 ) );
}

TEST( TestOffsetSharedPtr, CanDoCopyConstruct )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( sp_t1 );

	// Assert
	EXPECT_EQ( sp_t1.get(), p1 );
	EXPECT_EQ( sp_t1.use_count(), 2 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 2 );
}

TEST( TestOffsetSharedPtr, CanDoCopyConstruct_Then_Reset )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_sut( sp_t1 );
	EXPECT_EQ( sp_t1.get(), p1 );
	EXPECT_EQ( sp_t1.use_count(), 2 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 2 );

	// Act
	sp_t1.reset();

	// Assert
	EXPECT_EQ( sp_t1.get(), nullptr );
	EXPECT_EQ( sp_t1.use_count(), 0 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
}

TEST( TestOffsetSharedPtr, CanDoCopyConstructByDerivedDefaultConstructed )
{
	// Arrange
	offset_shared_ptr<DerivedArrowOpTest> sp_t1;

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( sp_t1 );

	// Assert
	EXPECT_EQ( sp_sut.get(), nullptr );
	EXPECT_EQ( sp_sut.use_count(), 0 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_FALSE( tb );
}

TEST( TestOffsetSharedPtr, CanDoMoveConstructByDerivedDefaultConstructed )
{
	// Arrange
	offset_shared_ptr<DerivedArrowOpTest> sp_t1;

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( std::move( sp_t1 ) );

	// Assert
	EXPECT_EQ( sp_sut.get(), nullptr );
	EXPECT_EQ( sp_sut.use_count(), 0 );
	bool tb = static_cast<bool>( sp_sut );
	EXPECT_FALSE( tb );
}

TEST( TestOffsetSharedPtr, CanDoCopyConstructedByConstructedWithDerivedTPtr )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( sp_t1 );

	// Assert
	EXPECT_EQ( sp_sut.get(), p );
	EXPECT_EQ( sp_sut.use_count(), 2 );
	EXPECT_TRUE( static_cast<bool>( sp_sut ) );

	EXPECT_EQ( sp_t1.get(), p );
	EXPECT_EQ( sp_t1.use_count(), 2 );
	EXPECT_TRUE( static_cast<bool>( sp_t1 ) );
}

TEST( TestOffsetSharedPtr, CanDoMoveConstructedByConstructedWithDerivedTPtr )
{
	// Arrange
	DerivedArrowOpTest*                   p = new DerivedArrowOpTest;
	offset_shared_ptr<DerivedArrowOpTest> sp_t1( p );

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( std::move( sp_t1 ) );

	// Assert
	EXPECT_EQ( sp_sut.get(), p );
	EXPECT_EQ( sp_sut.use_count(), 1 );
	EXPECT_TRUE( static_cast<bool>( sp_sut ) );

	EXPECT_EQ( sp_t1.get(), nullptr );
	EXPECT_EQ( sp_t1.use_count(), 0 );
	EXPECT_FALSE( static_cast<bool>( sp_t1 ) );
}

TEST( TestOffsetSharedPtr, CanDoMoveConstruct )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( std::move( sp_t1 ) );

	// Assert
	EXPECT_EQ( sp_t1.get(), nullptr );
	EXPECT_EQ( sp_t1.use_count(), 0 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
}

TEST( TestOffsetSharedPtr, CanDoCopyAssignmentFromNullToValid )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_sut;

	// Act
	sp_sut = sp_t1;

	// Assert
	EXPECT_EQ( sp_t1.get(), p1 );
	EXPECT_EQ( sp_t1.use_count(), 2 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 2 );
}

TEST( TestOffsetSharedPtr, CanDoCopyAssignmentFromValidToValid )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_sut( p2 );

	// Act
	sp_sut = sp_t1;

	// Assert
	EXPECT_EQ( sp_t1.get(), p1 );
	EXPECT_EQ( sp_t1.use_count(), 2 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 2 );
}

TEST( TestOffsetSharedPtr, CanDoMoveAssignmentFromNullToValid )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_sut;

	// Act
	sp_sut = std::move( sp_t1 );

	// Assert
	EXPECT_EQ( sp_t1.get(), nullptr );
	EXPECT_EQ( sp_t1.use_count(), 0 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
}

TEST( TestOffsetSharedPtr, CanDoMoveAssignmentFromValidToValid )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	ArrowOpTest*                   p2 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_shared_ptr<ArrowOpTest> sp_sut( p2 );

	// Act
	sp_sut = std::move( sp_t1 );

	// Assert
	EXPECT_EQ( sp_t1.get(), nullptr );
	EXPECT_EQ( sp_t1.use_count(), 0 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 1 );
}

TEST( TestOffsetSharedPtr, CanDoConstructFromWeakPtr1 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut( wp_t1 );

	// Assert
	EXPECT_EQ( sp_t1.get(), p1 );
	EXPECT_EQ( sp_t1.use_count(), 2 );
	EXPECT_EQ( sp_sut.get(), p1 );
	EXPECT_EQ( sp_sut.use_count(), 2 );
}

TEST( TestOffsetSharedPtr, CanDoConstructFromExpiredWeakPtr1 )
{
	// Arrange
	offset_weak_ptr<ArrowOpTest> wp_t1;

	// Act
	// Assert
	EXPECT_THROW( offset_shared_ptr<ArrowOpTest> sp_sut( wp_t1 ), std::bad_weak_ptr );
}

TEST( TestOffsetSharedPtr, CanDoConstructFromExpiredWeakPtr2 )
{
	// Arrange
	ArrowOpTest*                   p1 = new ArrowOpTest;
	offset_shared_ptr<ArrowOpTest> sp_t1( p1 );
	offset_weak_ptr<ArrowOpTest>   wp_t1( sp_t1 );
	sp_t1.reset();

	// Act
	// Assert
	EXPECT_THROW( offset_shared_ptr<ArrowOpTest> sp_sut( wp_t1 ), std::bad_weak_ptr );
}

TEST( TestOffsetSharedPtr, CanDoMakeOffsetShared )
{
	// Arrange
	ArrowOpTest t { 1, 2 };

	// Act
	offset_shared_ptr<ArrowOpTest> sp_sut = make_offset_shared<ArrowOpTest>( t );

	// Assert
	EXPECT_EQ( sp_sut.use_count(), 1 );
	ASSERT_NE( sp_sut.get(), nullptr );
	EXPECT_EQ( sp_sut->x, 1 );
	EXPECT_EQ( sp_sut->y, 2 );
}
