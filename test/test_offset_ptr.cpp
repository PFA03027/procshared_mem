/**
 * @file test_offset_ptr.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief test offset base pointer
 * @version 0.1
 * @date 2023-10-02
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <atomic>
#include <concepts>
#include <cstdio>
#include <cstdlib>

#include "gtest/gtest.h"

#include "offset_ptr.hpp"

template <std::contiguous_iterator I>
bool f( I x )
{
	return true;
}

// template <typename I>
// bool f( I x )
// {
// 	return false;
// }

TEST( OffsetPtr, CanDefaultConstruct )
{
	// Arrange

	// Act
	offset_ptr<int> op_a;

	// Assert
	EXPECT_EQ( op_a.get(), nullptr );
	EXPECT_TRUE( f( op_a ) );
}

TEST( OffsetPtr, CanConstruct )
{
	// Arrange
	int a;

	// Act
	offset_ptr<int> op_a( &a );

	// Assert
	EXPECT_EQ( op_a.get(), &a );
}

TEST( OffsetPtr, CanCopyConstruct )
{
	// Arrange
	int             a;
	offset_ptr<int> op_a( &a );

	// Act
	offset_ptr<int> op_b( op_a );

	// Assert
	EXPECT_EQ( op_a.get(), op_b.get() );
}

TEST( OffsetPtr, CanMoveConstruct )
{
	// Arrange
	int             a;
	offset_ptr<int> op_a( &a );

	// Act
	offset_ptr<int> op_b( std::move( op_a ) );

	// Assert
	EXPECT_NE( op_a.get(), op_b.get() );
	EXPECT_EQ( op_a.get(), nullptr );
	EXPECT_EQ( op_b.get(), &a );
}

TEST( OffsetPtr, CanCopyAssingment )
{
	// Arrange
	int             a;
	offset_ptr<int> op_a( &a );
	offset_ptr<int> op_b;
	EXPECT_EQ( op_b.get(), nullptr );

	// Act
	op_b = op_a;

	// Assert
	EXPECT_EQ( op_a.get(), op_b.get() );
}

TEST( OffsetPtr, CanMoveAssingment )
{
	// Arrange
	int             a;
	offset_ptr<int> op_a( &a );
	offset_ptr<int> op_b;
	EXPECT_EQ( op_b.get(), nullptr );

	// Act
	op_b = std::move( op_a );

	// Assert
	EXPECT_NE( op_a.get(), op_b.get() );
	EXPECT_EQ( op_a.get(), nullptr );
	EXPECT_EQ( op_b.get(), &a );
}

TEST( OffsetPtr, CanSwap1 )
{
	// Arrange
	int             a;
	int             b;
	offset_ptr<int> op_a( &a );
	offset_ptr<int> op_b( &b );

	// Act
	op_b.swap( op_a );

	// Assert
	EXPECT_EQ( op_a.get(), &b );
	EXPECT_EQ( op_b.get(), &a );
}

TEST( OffsetPtr, CanSwap2 )
{
	// Arrange
	int             a;
	int             b;
	offset_ptr<int> op_a( &a );
	offset_ptr<int> op_b( &b );

	// Act
	swap( op_a, op_b );

	// Assert
	EXPECT_EQ( op_a.get(), &b );
	EXPECT_EQ( op_b.get(), &a );
}

struct ArrowOpTest {
	int x;
	int y;
};

TEST( OffsetPtr, CanArrowOperator )
{
	// Arrange
	ArrowOpTest a;

	// Act
	offset_ptr<ArrowOpTest> op_a( &a );

	// Assert
	EXPECT_EQ( &( op_a->x ), &( a.x ) );
}

TEST( OffsetPtr, CanRefOperator )
{
	// Arrange
	ArrowOpTest a;

	// Act
	offset_ptr<ArrowOpTest> op_a( &a );

	// Assert
	EXPECT_EQ( &( *op_a ), &( a ) );
}

TEST( OffsetPtr, CanBoolOperatorFalse )
{
	// Arrange

	// Act
	offset_ptr<ArrowOpTest> op_a;

	// Assert
	EXPECT_FALSE( op_a );
}

TEST( OffsetPtr, CanBoolOperatorTrue )
{
	// Arrange
	ArrowOpTest a;

	// Act
	offset_ptr<ArrowOpTest> op_a( &a );

	// Assert
	EXPECT_TRUE( op_a );
}

TEST( OffsetPtr, CanTpointerOperator )
{
	// Arrange
	ArrowOpTest             a;
	offset_ptr<ArrowOpTest> op_a( &a );

	// Act
	ArrowOpTest* p_a = (ArrowOpTest*)op_a;

	// Assert
	EXPECT_EQ( p_a, &a );
}

TEST( OffsetPtr, CanArrayOperator )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( a );

	// Act
	ArrowOpTest* p_a = &( op_a[1] );

	// Assert
	EXPECT_EQ( p_a, &( a[1] ) );
}

TEST( OffsetPtr, CanPreIncrementOperator )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( a );

	// Act
	offset_ptr<ArrowOpTest> p = ++op_a;

	// Assert
	EXPECT_EQ( p.get(), &( a[1] ) );
	EXPECT_EQ( op_a.get(), &( a[1] ) );
}

TEST( OffsetPtr, CanPostIncrementOperator )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( a );

	// Act
	offset_ptr<ArrowOpTest> p = op_a++;

	// Assert
	EXPECT_EQ( p.get(), &( a[0] ) );
	EXPECT_EQ( op_a.get(), &( a[1] ) );
}

TEST( OffsetPtr, CanPreDecrementOperator )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( &( a[1] ) );

	// Act
	offset_ptr<ArrowOpTest> p = --op_a;

	// Assert
	EXPECT_EQ( p.get(), &( a[0] ) );
	EXPECT_EQ( op_a.get(), &( a[0] ) );
}

TEST( OffsetPtr, CanPostDecrementOperator )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( &( a[1] ) );

	// Act
	offset_ptr<ArrowOpTest> p = op_a--;

	// Assert
	EXPECT_EQ( p.get(), &( a[1] ) );
	EXPECT_EQ( op_a.get(), &( a[0] ) );
}

TEST( OffsetPtr, CanCompareOperatorEQ )
{
	// Arrange
	ArrowOpTest             a;
	offset_ptr<ArrowOpTest> op_a( &a );
	offset_ptr<ArrowOpTest> op_b( &a );
	offset_ptr<ArrowOpTest> op_c( &a );
	op_c++;

	// Act

	// Assert
	EXPECT_TRUE( op_a == op_b );
	EXPECT_FALSE( op_a == op_c );
}

TEST( OffsetPtr, CanCompareOperatorNE )
{
	// Arrange
	ArrowOpTest             a;
	offset_ptr<ArrowOpTest> op_a( &a );
	offset_ptr<ArrowOpTest> op_b( &a );
	offset_ptr<ArrowOpTest> op_c( &a );
	op_c++;

	// Act

	// Assert
	EXPECT_FALSE( op_a != op_b );
	EXPECT_TRUE( op_a != op_c );
}

TEST( OffsetPtr, CanCompareOperatorGT )
{
	// Arrange
	ArrowOpTest             a;
	offset_ptr<ArrowOpTest> op_a( &a );
	offset_ptr<ArrowOpTest> op_b( &a );
	offset_ptr<ArrowOpTest> op_c( &a );
	op_c++;

	// Act

	// Assert
	EXPECT_FALSE( op_a > op_b );
	EXPECT_FALSE( op_a > op_c );
	EXPECT_FALSE( op_b > op_a );
	EXPECT_TRUE( op_c > op_a );
}

TEST( OffsetPtr, CanCompareOperatorGE )
{
	// Arrange
	ArrowOpTest             a;
	offset_ptr<ArrowOpTest> op_a( &a );
	offset_ptr<ArrowOpTest> op_b( &a );
	offset_ptr<ArrowOpTest> op_c( &a );
	op_c++;

	// Act

	// Assert
	EXPECT_TRUE( op_a >= op_b );
	EXPECT_FALSE( op_a >= op_c );
	EXPECT_TRUE( op_b >= op_a );
	EXPECT_TRUE( op_c >= op_a );
}

TEST( OffsetPtr, CanCompareOperatorLT )
{
	// Arrange
	ArrowOpTest             a;
	offset_ptr<ArrowOpTest> op_a( &a );
	offset_ptr<ArrowOpTest> op_b( &a );
	offset_ptr<ArrowOpTest> op_c( &a );
	op_c++;

	// Act

	// Assert
	EXPECT_FALSE( op_a < op_b );
	EXPECT_TRUE( op_a < op_c );
	EXPECT_FALSE( op_b < op_a );
	EXPECT_FALSE( op_c < op_a );
}

TEST( OffsetPtr, CanCompareOperatorLE )
{
	// Arrange
	ArrowOpTest             a;
	offset_ptr<ArrowOpTest> op_a( &a );
	offset_ptr<ArrowOpTest> op_b( &a );
	offset_ptr<ArrowOpTest> op_c( &a );
	op_c++;

	// Act

	// Assert
	EXPECT_TRUE( op_a <= op_b );
	EXPECT_TRUE( op_a <= op_c );
	EXPECT_TRUE( op_b <= op_a );
	EXPECT_FALSE( op_c <= op_a );
}

TEST( OffsetPtr, CanAddOperator1 )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( a );

	// Act
	op_a += 1;

	// Assert
	EXPECT_EQ( op_a.get(), &( a[1] ) );
}

TEST( OffsetPtr, CanAddOperator2 )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( a );

	// Act
	auto op_b = op_a + 1;

	// Assert
	EXPECT_EQ( op_a.get(), &( a[0] ) );
	EXPECT_EQ( op_b.get(), &( a[1] ) );
}

TEST( OffsetPtr, CanAddOperator3 )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( a );

	// Act
	auto op_b = 1 + op_a;

	// Assert
	EXPECT_EQ( op_a.get(), &( a[0] ) );
	EXPECT_EQ( op_b.get(), &( a[1] ) );
}

TEST( OffsetPtr, CanSubOperator1 )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( &( a[1] ) );

	// Act
	op_a -= 1;

	// Assert
	EXPECT_EQ( op_a.get(), &( a[0] ) );
}

TEST( OffsetPtr, CanSubOperator2 )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( &( a[1] ) );

	// Act
	auto op_b = op_a - 1;

	// Assert
	EXPECT_EQ( op_a.get(), &( a[1] ) );
	EXPECT_EQ( op_b.get(), &( a[0] ) );
}

TEST( OffsetPtr, CanSubOperator3 )
{
	// Arrange
	ArrowOpTest             a[2];
	offset_ptr<ArrowOpTest> op_a( &( a[0] ) );
	offset_ptr<ArrowOpTest> op_b( &( a[1] ) );

	// Act
	auto d = op_b - op_a;

	// Assert
	EXPECT_EQ( d, 1 );
}
