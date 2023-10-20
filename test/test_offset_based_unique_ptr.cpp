/**
 * @file test_offset_based_unique_ptr.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2023-10-16
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <cstdio>
#include <cstdlib>

#include "gtest/gtest.h"

#include "offset_based_unique_ptr.hpp"

struct ArrowOpTest {
	int x;
	int y;
};

template <typename T>
struct test_no_deleter {
	test_no_deleter( int mark )
	  : test_mark_( mark )
	{
	}
	test_no_deleter( const test_no_deleter& )            = default;
	test_no_deleter( test_no_deleter&& )                 = default;
	test_no_deleter& operator=( const test_no_deleter& ) = default;
	test_no_deleter& operator=( test_no_deleter&& )      = default;

	void operator()( T* p )
	{
	}

	int test_mark_;
};
template <typename T>
struct test_deleter {
	test_deleter( int mark )
	  : test_mark_( mark )
	{
	}
	test_deleter( const test_deleter& )            = default;
	test_deleter( test_deleter&& )                 = default;
	test_deleter& operator=( const test_deleter& ) = default;
	test_deleter& operator=( test_deleter&& )      = default;

	void operator()( T* p )
	{
		delete p;
	}

	int test_mark_;
};
template <typename T>
struct test_deleter<T[]> {
	test_deleter( int mark )
	  : test_mark_( mark )
	{
	}
	test_deleter( const test_deleter& )            = delete;
	test_deleter( test_deleter&& )                 = delete;
	test_deleter& operator=( const test_deleter& ) = delete;
	test_deleter& operator=( test_deleter&& )      = delete;

	void operator()( T* p )
	{
		delete[] p;
	}

	int test_mark_;
};

TEST( OffsetBasedUniquePtr, CanDefaultConstruct )
{
	// Arrange

	// Act
	offset_based_unique_ptr<int> oup_sut;

	// Assert
	EXPECT_EQ( oup_sut, nullptr );
	EXPECT_EQ( nullptr, oup_sut );
}

TEST( OffsetBasedUniquePtr, CanConstructByPtr )
{
	// Arrange
	int* p = new int;

	// Act
	offset_based_unique_ptr<int> oup_sut( p );

	// Assert
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
}

TEST( OffsetBasedUniquePtr, CanConstructByPtrWithDeleterCp )
{
	// Arrange
	int*              p = new int;
	test_deleter<int> sut_dt( 1 );

	// Act
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut( p, sut_dt );

	// Assert
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
	EXPECT_EQ( 1, oup_sut.get_deleter().test_mark_ );
}

TEST( OffsetBasedUniquePtr, CanConstructByPtrWithDeleterMv )
{
	// Arrange
	int* p = new int;

	// Act
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut( p, test_deleter<int>( 1 ) );

	// Assert
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
	EXPECT_EQ( 1, oup_sut.get_deleter().test_mark_ );
}

TEST( OffsetBasedUniquePtr, CanConstructByNullPtr )
{
	// Arrange

	// Act
	offset_based_unique_ptr<int> oup_sut( nullptr );

	// Assert
	EXPECT_EQ( oup_sut, nullptr );
	EXPECT_EQ( nullptr, oup_sut );
}

TEST( OffsetBasedUniquePtr, CanConstructByOffsetUp1 )
{
	// Arrange
	int*                         p = new int;
	offset_based_unique_ptr<int> oup_tt( p );

	// Act
	offset_based_unique_ptr<int> oup_sut( std::move( oup_tt ) );

	// Assert
	EXPECT_EQ( oup_tt, nullptr );
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
}

TEST( OffsetBasedUniquePtr, CanConstructByOffsetUp2 )
{
	// Arrange
	int*                                            p = new int;
	offset_based_unique_ptr<int, test_deleter<int>> oup_tt( p, test_deleter<int>( 1 ) );
	EXPECT_EQ( 1, oup_tt.get_deleter().test_mark_ );

	// Act
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut( std::move( oup_tt ) );

	// Assert
	EXPECT_EQ( oup_tt, nullptr );
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
	EXPECT_EQ( 1, oup_sut.get_deleter().test_mark_ );
}

TEST( OffsetBasedUniquePtr, CanNotCopyConstruct )
{
	// Arrange

	// Act
	EXPECT_FALSE( std::is_copy_constructible<offset_based_unique_ptr<int>>::value );

	// Assert
}

TEST( OffsetBasedUniquePtr, CanMoveConstruct1 )
{
	EXPECT_TRUE( std::is_move_constructible<offset_based_unique_ptr<int>>::value );

	// Arrange
	int*                         p = new int;
	offset_based_unique_ptr<int> oup_tt( p );
	EXPECT_EQ( oup_tt.get(), p );

	// Act
	offset_based_unique_ptr<int> oup_sut( std::move( oup_tt ) );

	// Assert
	EXPECT_EQ( oup_tt, nullptr );
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
}

TEST( OffsetBasedUniquePtr, CanMoveConstruct2 )
{

	EXPECT_TRUE( ( std::is_move_constructible<offset_based_unique_ptr<int, test_deleter<int>>>::value ) );

	// Arrange
	int*                                            p = new int;
	offset_based_unique_ptr<int, test_deleter<int>> oup_tt( p, test_deleter<int>( 1 ) );
	EXPECT_EQ( oup_tt.get(), p );
	EXPECT_EQ( 1, oup_tt.get_deleter().test_mark_ );

	// Act
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut( std::move( oup_tt ) );

	// Assert
	EXPECT_EQ( oup_tt, nullptr );
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
	EXPECT_EQ( 1, oup_sut.get_deleter().test_mark_ );
}

TEST( OffsetBasedUniquePtr, CanNotCopyAssign )
{
	// Arrange

	// Act
	EXPECT_FALSE( std::is_copy_assignable<offset_based_unique_ptr<int>>::value );

	// Assert
}

TEST( OffsetBasedUniquePtr, CanMoveAssign1 )
{
	EXPECT_TRUE( std::is_move_assignable<offset_based_unique_ptr<int>>::value );

	// Arrange
	int*                         p = new int;
	offset_based_unique_ptr<int> oup_tt( p );
	EXPECT_EQ( oup_tt.get(), p );
	offset_based_unique_ptr<int> oup_sut;
	EXPECT_EQ( oup_sut, nullptr );

	// Act
	oup_sut = std::move( oup_tt );

	// Assert
	EXPECT_EQ( oup_tt, nullptr );
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
}

TEST( OffsetBasedUniquePtr, CanMoveAssign2 )
{
	EXPECT_TRUE( ( std::is_move_assignable<offset_based_unique_ptr<int, test_deleter<int>>>::value ) );

	// Arrange
	int*                                            p = new int;
	offset_based_unique_ptr<int, test_deleter<int>> oup_tt( p, test_deleter<int>( 1 ) );
	EXPECT_EQ( oup_tt.get(), p );
	EXPECT_EQ( 1, oup_tt.get_deleter().test_mark_ );
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut( nullptr, test_deleter<int>( 2 ) );
	EXPECT_EQ( oup_sut, nullptr );
	EXPECT_EQ( 2, oup_sut.get_deleter().test_mark_ );

	// Act
	oup_sut = std::move( oup_tt );

	// Assert
	EXPECT_EQ( oup_tt, nullptr );
	EXPECT_EQ( oup_sut.get(), p );
	EXPECT_EQ( p, oup_sut.get() );
	EXPECT_EQ( 1, oup_sut.get_deleter().test_mark_ );
}

TEST( OffsetBasedUniquePtr, CanRelease )
{
	// Arrange
	int*                         p = new int;
	offset_based_unique_ptr<int> oup_sut( p );
	EXPECT_EQ( oup_sut.get(), p );

	// Act
	int* p_ret = oup_sut.release();

	// Assert
	EXPECT_EQ( p_ret, p );
	EXPECT_EQ( oup_sut.get(), nullptr );
	EXPECT_EQ( nullptr, oup_sut.get() );

	// Cleanup
	delete p;
}

TEST( OffsetBasedUniquePtr, CanReset1 )
{
	// Arrange
	int*                         p = new int;
	offset_based_unique_ptr<int> oup_sut( p );
	EXPECT_EQ( oup_sut.get(), p );

	// Act
	oup_sut.reset();

	// Assert
	EXPECT_EQ( oup_sut.get(), nullptr );
	EXPECT_EQ( nullptr, oup_sut.get() );
}

TEST( OffsetBasedUniquePtr, CanReset2 )
{
	// Arrange
	int*                         p1 = new int;
	int*                         p2 = new int;
	offset_based_unique_ptr<int> oup_sut( p1 );
	EXPECT_EQ( oup_sut.get(), p1 );

	// Act
	oup_sut.reset( p2 );

	// Assert
	EXPECT_EQ( oup_sut.get(), p2 );
	EXPECT_EQ( p2, oup_sut.get() );
}

TEST( OffsetBasedUniquePtr, CanSwap1 )
{
	// Arrange
	int*                         p1 = new int;
	int*                         p2 = new int;
	offset_based_unique_ptr<int> oup_sut1( p1 );
	EXPECT_EQ( oup_sut1.get(), p1 );
	offset_based_unique_ptr<int> oup_sut2( p2 );
	EXPECT_EQ( oup_sut2.get(), p2 );

	// Act
	oup_sut1.swap( oup_sut2 );

	// Assert
	EXPECT_EQ( oup_sut1.get(), p2 );
	EXPECT_EQ( p2, oup_sut1.get() );
	EXPECT_EQ( oup_sut2.get(), p1 );
	EXPECT_EQ( p1, oup_sut2.get() );
}

TEST( OffsetBasedUniquePtr, CanSwap2 )
{
	// Arrange
	int*                                            p1 = new int;
	int*                                            p2 = new int;
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut1( p1, test_deleter<int>( 1 ) );
	EXPECT_EQ( oup_sut1.get(), p1 );
	EXPECT_EQ( 1, oup_sut1.get_deleter().test_mark_ );
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut2( p2, test_deleter<int>( 2 ) );
	EXPECT_EQ( oup_sut2.get(), p2 );
	EXPECT_EQ( 2, oup_sut2.get_deleter().test_mark_ );

	// Act
	oup_sut1.swap( oup_sut2 );

	// Assert
	EXPECT_EQ( oup_sut1.get(), p2 );
	EXPECT_EQ( p2, oup_sut1.get() );
	EXPECT_EQ( 2, oup_sut1.get_deleter().test_mark_ );
	EXPECT_EQ( oup_sut2.get(), p1 );
	EXPECT_EQ( p1, oup_sut2.get() );
	EXPECT_EQ( 1, oup_sut2.get_deleter().test_mark_ );
}

TEST( OffsetBasedUniquePtr, CanSwap3 )
{
	// Arrange
	int*                         p1 = new int;
	int*                         p2 = new int;
	offset_based_unique_ptr<int> oup_sut1( p1 );
	EXPECT_EQ( oup_sut1.get(), p1 );
	offset_based_unique_ptr<int> oup_sut2( p2 );
	EXPECT_EQ( oup_sut2.get(), p2 );

	// Act
	swap( oup_sut1, oup_sut2 );

	// Assert
	EXPECT_EQ( oup_sut1.get(), p2 );
	EXPECT_EQ( p2, oup_sut1.get() );
	EXPECT_EQ( oup_sut2.get(), p1 );
	EXPECT_EQ( p1, oup_sut2.get() );
}

TEST( OffsetBasedUniquePtr, CanSwap4 )
{
	// Arrange
	int*                                            p1 = new int;
	int*                                            p2 = new int;
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut1( p1, test_deleter<int>( 1 ) );
	EXPECT_EQ( oup_sut1.get(), p1 );
	EXPECT_EQ( 1, oup_sut1.get_deleter().test_mark_ );
	offset_based_unique_ptr<int, test_deleter<int>> oup_sut2( p2, test_deleter<int>( 2 ) );
	EXPECT_EQ( oup_sut2.get(), p2 );
	EXPECT_EQ( 2, oup_sut2.get_deleter().test_mark_ );

	// Act
	swap( oup_sut1, oup_sut2 );

	// Assert
	EXPECT_EQ( oup_sut1.get(), p2 );
	EXPECT_EQ( p2, oup_sut1.get() );
	EXPECT_EQ( 2, oup_sut1.get_deleter().test_mark_ );
	EXPECT_EQ( oup_sut2.get(), p1 );
	EXPECT_EQ( p1, oup_sut2.get() );
	EXPECT_EQ( 1, oup_sut2.get_deleter().test_mark_ );
}

TEST( OffsetBasedUniquePtr, CanCastBool1 )
{
	// Arrange
	int*                         p = new int;
	offset_based_unique_ptr<int> oup_sut( p );
	EXPECT_EQ( oup_sut.get(), p );

	// Act
	bool test_ret = (bool)oup_sut;

	// Assert
	EXPECT_TRUE( test_ret );
}

TEST( OffsetBasedUniquePtr, CanCastBool2 )
{
	// Arrange
	offset_based_unique_ptr<int> oup_sut;
	EXPECT_EQ( oup_sut.get(), nullptr );

	// Act
	bool test_ret = (bool)oup_sut;

	// Assert
	EXPECT_FALSE( test_ret );
}

TEST( OffsetBasedUniquePtr, CanRefOp )
{
	// Arrange

	// Act
	offset_based_unique_ptr<ArrowOpTest> oup_sut = make_offset_based_unique<ArrowOpTest>( ArrowOpTest { 1, 2 } );

	// Assert
	ASSERT_NE( oup_sut.get(), nullptr );
	EXPECT_EQ( ( *oup_sut ).x, 1 );
	EXPECT_EQ( ( *oup_sut ).y, 2 );
}

TEST( OffsetBasedUniquePtr, CanArrowOp )
{
	// Arrange

	// Act
	offset_based_unique_ptr<ArrowOpTest> oup_sut = make_offset_based_unique<ArrowOpTest>( ArrowOpTest { 1, 2 } );

	// Assert
	ASSERT_NE( oup_sut.get(), nullptr );
	EXPECT_EQ( oup_sut->x, 1 );
	EXPECT_EQ( oup_sut->y, 2 );
}

TEST( OffsetBasedUniquePtr, CanArrayOperator )
{
	// Arrange
	ArrowOpTest* p_raw = new ArrowOpTest[10];
	p_raw[0].x         = 1;
	p_raw[0].y         = 2;
	p_raw[1].x         = 3;
	p_raw[1].y         = 4;

	// Act
	offset_based_unique_ptr<ArrowOpTest[]> oup_sut( p_raw );

	// Assert
	ASSERT_NE( oup_sut.get(), nullptr );
	EXPECT_EQ( oup_sut[0].x, 1 );
	EXPECT_EQ( oup_sut[0].y, 2 );
	EXPECT_EQ( oup_sut[1].x, 3 );
	EXPECT_EQ( oup_sut[1].y, 4 );
}

TEST( OffsetBasedUniquePtr, CanOperators )
{
	// Arrange
	int                                                test_buff[3];
	offset_based_unique_ptr<int, test_no_deleter<int>> oup_sut1( &( test_buff[0] ), test_no_deleter<int>( 0 ) );
	offset_based_unique_ptr<int, test_no_deleter<int>> oup_sut2( &( test_buff[1] ), test_no_deleter<int>( 1 ) );
	offset_based_unique_ptr<int, test_no_deleter<int>> oup_sut3( &( test_buff[2] ), test_no_deleter<int>( 2 ) );
	offset_based_unique_ptr<int, test_no_deleter<int>> oup_sut22( &( test_buff[1] ), test_no_deleter<int>( 4 ) );

	// Act

	// Assert
	EXPECT_TRUE( ( oup_sut2 == oup_sut22 ) );
	EXPECT_FALSE( ( oup_sut2 != oup_sut22 ) );
	EXPECT_FALSE( ( oup_sut1 == oup_sut2 ) );
	EXPECT_TRUE( ( oup_sut1 != oup_sut2 ) );

	EXPECT_TRUE( ( oup_sut1 < oup_sut2 ) );
	EXPECT_FALSE( ( oup_sut2 < oup_sut22 ) );
	EXPECT_FALSE( ( oup_sut3 < oup_sut2 ) );

	EXPECT_TRUE( ( oup_sut1 <= oup_sut2 ) );
	EXPECT_TRUE( ( oup_sut2 <= oup_sut22 ) );
	EXPECT_FALSE( ( oup_sut3 <= oup_sut2 ) );

	EXPECT_FALSE( ( oup_sut1 > oup_sut2 ) );
	EXPECT_FALSE( ( oup_sut2 > oup_sut22 ) );
	EXPECT_TRUE( ( oup_sut3 > oup_sut2 ) );

	EXPECT_FALSE( ( oup_sut1 >= oup_sut2 ) );
	EXPECT_TRUE( ( oup_sut2 >= oup_sut22 ) );
	EXPECT_TRUE( ( oup_sut3 >= oup_sut2 ) );
}

TEST( OffsetBasedUniquePtr, CanOperatorsWithNullPtr )
{
	// Arrange
	int                                                test_buff;
	offset_based_unique_ptr<int, test_no_deleter<int>> oup_sut( &( test_buff ), test_no_deleter<int>( 0 ) );

	// Act

	// Assert
	EXPECT_FALSE( ( oup_sut == nullptr ) );
	EXPECT_FALSE( ( nullptr == oup_sut ) );
	EXPECT_TRUE( ( oup_sut != nullptr ) );
	EXPECT_TRUE( ( nullptr != oup_sut ) );

	EXPECT_FALSE( ( oup_sut < nullptr ) );
	EXPECT_TRUE( ( nullptr < oup_sut ) );

	EXPECT_FALSE( ( oup_sut <= nullptr ) );
	EXPECT_TRUE( ( nullptr <= oup_sut ) );

	EXPECT_TRUE( ( oup_sut > nullptr ) );
	EXPECT_FALSE( ( nullptr > oup_sut ) );

	EXPECT_TRUE( ( oup_sut >= nullptr ) );
	EXPECT_FALSE( ( nullptr >= oup_sut ) );
}
