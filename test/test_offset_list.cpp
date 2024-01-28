/**
 * @file test_offset_ptr.cpp
 * @author PFA03027@nifty.com
 * @brief test offset base pointer
 * @version 0.1
 * @date 2023-10-02
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <cstdio>
#include <cstdlib>

#include "gtest/gtest.h"

#include "offset_allocator.hpp"
#include "offset_list.hpp"

using namespace ipsm;

struct EmplacementTestData {
	int    x_;
	double y_;
};

bool operator==( const EmplacementTestData& a, const EmplacementTestData& b )
{
	return ( a.x_ == b.x_ ) &&
	       ( a.y_ == b.y_ );
}

TEST( OffsetList_CanDefaultConstruct, CanDefaultConstruct1 )
{
	// Arrange

	// Act
	ASSERT_NO_THROW( offset_list<int> sut; );

	// Assert
}

TEST( OffsetList_CanDefaultConstruct, CanDefaultConstruct2 )
{
	// Arrange
	offset_list<int> sut;

	// Act
	auto ret = sut.size();

	// Assert
	EXPECT_EQ( ret, 0 );
	EXPECT_TRUE( sut.empty() );
}

TEST( OffsetList_CanCopyConstruct, FromEmpty1 )
{
	// Arrange
	offset_list<int> src;

	// Act
	ASSERT_NO_THROW( offset_list<int> sut( src ); );

	// Assert
}

TEST( OffsetList_CanCopyConstruct, FromEmpty2 )
{
	// Arrange
	offset_list<int> src;

	// Act
	offset_list<int> sut( src );

	// Assert
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( src.size(), 0 );
	EXPECT_TRUE( sut.empty() );
}

TEST( OffsetList_CanCopyConstruct, From1Node )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );

	// Act
	offset_list<int> sut( src );

	// Assert
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 1 );
	EXPECT_EQ( src.size(), 1 );
	EXPECT_EQ( src.front(), 1 );
	EXPECT_EQ( src.back(), 1 );
}

TEST( OffsetList_CanCopyConstruct, From2Nodes )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	src.push_back( 2 );

	// Act
	offset_list<int> sut( src );

	// Assert
	EXPECT_EQ( sut.size(), 2 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 2 );
	EXPECT_EQ( src.size(), 2 );
	EXPECT_EQ( src.front(), 1 );
	EXPECT_EQ( src.back(), 2 );
}

TEST( OffsetList_CanCopyConstruct, From3Nodes )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	src.push_back( 2 );
	src.push_back( 3 );

	// Act
	offset_list<int> sut( src );

	// Assert
	EXPECT_EQ( sut.size(), 3 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 3 );
	EXPECT_EQ( src.size(), 3 );
	EXPECT_EQ( src.front(), 1 );
	EXPECT_EQ( src.back(), 3 );
}

TEST( OffsetList_CanMoveConstruct, FromEmpty1 )
{
	// Arrange
	offset_list<int> src;

	// Act
	ASSERT_NO_THROW( offset_list<int> sut( std::move( src ) ); );

	// Assert
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanMoveConstruct, FromEmpty2 )
{
	// Arrange
	offset_list<int> src;

	// Act
	offset_list<int> sut( std::move( src ) );

	// Assert
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanMoveConstruct, From1Node )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );

	// Act
	offset_list<int> sut( std::move( src ) );

	// Assert
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 1 );
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanMoveConstruct, From2Nodes )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	src.push_back( 2 );

	// Act
	offset_list<int> sut( std::move( src ) );

	// Assert
	EXPECT_EQ( sut.size(), 2 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 2 );
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanMoveConstruct, From3Nodes )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	src.push_back( 2 );
	src.push_back( 3 );

	// Act
	offset_list<int> sut( std::move( src ) );

	// Assert
	EXPECT_EQ( sut.size(), 3 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 3 );
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanCopyAssingment, FromEmpty )
{
	// Arrange
	offset_list<int> src;
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut = src; );

	// Assert
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( src.size(), 0 );
}

TEST( OffsetList_CanCopyAssingment, From1Node )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut = src; );

	// Assert
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 1 );
	EXPECT_EQ( src.size(), 1 );
	EXPECT_EQ( src.front(), 1 );
	EXPECT_EQ( src.back(), 1 );
}

TEST( OffsetList_CanCopyAssingment, From2Nodes )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	src.push_back( 2 );
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut = src; );

	// Assert
	EXPECT_EQ( sut.size(), 2 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 2 );
	EXPECT_EQ( src.size(), 2 );
	EXPECT_EQ( src.front(), 1 );
	EXPECT_EQ( src.back(), 2 );
}

TEST( OffsetList_CanCopyAssingment, From3Nodes )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	src.push_back( 2 );
	src.push_back( 3 );
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut = src; );

	// Assert
	EXPECT_EQ( sut.size(), 3 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 3 );
	EXPECT_EQ( src.size(), 3 );
	EXPECT_EQ( src.front(), 1 );
	EXPECT_EQ( src.back(), 3 );
}

TEST( OffsetList_CanMoveAssingment, FromEmpty )
{
	// Arrange
	offset_list<int> src;
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut = std::move( src ); );

	// Assert
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanMoveAssingment, From1Node )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut = std::move( src ); );

	// Assert
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 1 );
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanMoveAssingment, From2Nodes )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	src.push_back( 2 );
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut = std::move( src ); );

	// Assert
	EXPECT_EQ( sut.size(), 2 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 2 );
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanMoveAssingment, From3Nodes )
{
	// Arrange
	offset_list<int> src;
	src.push_back( 1 );
	src.push_back( 2 );
	src.push_back( 3 );
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut = std::move( src ); );

	// Assert
	EXPECT_EQ( sut.size(), 3 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 3 );
	EXPECT_EQ( src.size(), 0 );   // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST( OffsetList_CanConstructInitializerlist, FromEmpty )
{
	// Arrange

	// Act
	offset_list<int> sut = {};

	// Assert
	EXPECT_EQ( sut.size(), 0 );
}

TEST( OffsetList_CanConstructInitializerlist, From1Item )
{
	// Arrange

	// Act
	offset_list<int> sut = { 1 };

	// Assert
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 1 );
}

TEST( OffsetList_CanConstructInitializerlist, From2Item )
{
	// Arrange

	// Act
	offset_list<int> sut = { 1, 2 };

	// Assert
	EXPECT_EQ( sut.size(), 2 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 2 );
}

TEST( OffsetList_CanConstructInitializerlist, From3Item )
{
	// Arrange

	// Act
	offset_list<int> sut = { 1, 2, 3 };

	// Assert
	EXPECT_EQ( sut.size(), 3 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 3 );
}

TEST( OffsetList_CanInitializerlistAssignment, Assign3ItemToEmpty )
{
	// Arrange
	offset_list<int> sut;

	// Act
	sut = { 1, 2, 3 };

	// Assert
	EXPECT_EQ( sut.size(), 3 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 3 );
}

TEST( OffsetList_CanInitializerlistAssignment, Assign3ItemTo1ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 10 );

	// Act
	sut = { 1, 2, 3 };

	// Assert
	EXPECT_EQ( sut.size(), 3 );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 3 );
}

TEST( OffsetList_Construct, CanTypeSelf )
{
	// Arrange

	// Act
	offset_list<offset_list<int>> src;

	// Assert
}

TEST( OffsetList_Construct, CanPushTypeSelf )
{
	// Arrange
	offset_list<offset_list<int>> src;

	// Act
	src.push_back( offset_list<int>() );

	// Assert
}

TEST( OffsetList_Push, CanPushBack1 )
{
	// Arrange
	offset_list<int> sut;

	// Act
	sut.push_back( 1 );

	// Assert
	EXPECT_EQ( sut.front(), 1 );
	auto ret = sut.size();
	EXPECT_EQ( ret, 1 );
	EXPECT_EQ( sut.back(), 1 );
}

TEST( OffsetList_Push, CanPushBack2 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );

	// Act
	sut.push_back( 2 );

	// Assert
	EXPECT_EQ( sut.front(), 1 );
	auto ret = sut.size();
	EXPECT_EQ( ret, 2 );
	EXPECT_EQ( sut.back(), 2 );
}

TEST( OffsetList_Push, CanPushBack3 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	sut.push_back( 2 );

	// Act
	sut.push_back( 3 );

	// Assert
	EXPECT_EQ( sut.front(), 1 );
	auto ret = sut.size();
	EXPECT_EQ( ret, 3 );
	EXPECT_EQ( sut.back(), 3 );
}

TEST( OffsetList_Push, CanPushFront1 )
{
	// Arrange
	offset_list<int> sut;

	// Act
	sut.push_front( 1 );

	// Assert
	EXPECT_EQ( sut.front(), 1 );
	auto ret = sut.size();
	EXPECT_EQ( ret, 1 );
	EXPECT_EQ( sut.back(), 1 );
}

TEST( OffsetList_Push, CanPushFront2 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );

	// Act
	sut.push_front( 2 );

	// Assert
	EXPECT_EQ( sut.front(), 2 );
	auto ret = sut.size();
	EXPECT_EQ( ret, 2 );
	EXPECT_EQ( sut.back(), 1 );
}

TEST( OffsetList_Push, CanPushFront3 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_front( 2 );

	// Act
	sut.push_front( 3 );

	// Assert
	EXPECT_EQ( sut.front(), 3 );
	auto ret = sut.size();
	EXPECT_EQ( ret, 3 );
	EXPECT_EQ( sut.back(), 1 );
}

TEST( OffsetList_Push, CanEmplaceFront1 )
{
	// Arrange
	offset_list<EmplacementTestData> sut;

	// Act
	sut.emplace_front( 1, 2.0 );

	// Assert
	EXPECT_EQ( sut.front().x_, 1 );
	EXPECT_EQ( sut.front().y_, 2.0 );
	EXPECT_EQ( sut.back().x_, 1 );
	EXPECT_EQ( sut.back().y_, 2.0 );
}

TEST( OffsetList_Push, CanEmplaceFront2 )
{
	// Arrange
	offset_list<EmplacementTestData> sut;
	sut.emplace_front( 1, 2.0 );

	// Act
	sut.emplace_front( 3, 4.0 );

	// Assert
	EXPECT_EQ( sut.front().x_, 3 );
	EXPECT_EQ( sut.front().y_, 4.0 );
	EXPECT_EQ( sut.back().x_, 1 );
	EXPECT_EQ( sut.back().y_, 2.0 );
}

TEST( OffsetList_Push, CanEmplaceFront3 )
{
	// Arrange
	offset_list<EmplacementTestData> sut;
	sut.emplace_front( 1, 2.0 );
	sut.emplace_front( 3, 4.0 );

	// Act
	sut.emplace_front( 5, 6.0 );

	// Assert
	EXPECT_EQ( sut.front().x_, 5 );
	EXPECT_EQ( sut.front().y_, 6.0 );
	EXPECT_EQ( sut.back().x_, 1 );
	EXPECT_EQ( sut.back().y_, 2.0 );
}

TEST( OffsetList_Push, CanEmplaceBack1 )
{
	// Arrange
	offset_list<EmplacementTestData> sut;

	// Act
	sut.emplace_back( 1, 2.0 );

	// Assert
	EXPECT_EQ( sut.front().x_, 1 );
	EXPECT_EQ( sut.front().y_, 2.0 );
	EXPECT_EQ( sut.back().x_, 1 );
	EXPECT_EQ( sut.back().y_, 2.0 );
}

TEST( OffsetList_Push, CanEmplaceBack2 )
{
	// Arrange
	offset_list<EmplacementTestData> sut;
	sut.emplace_back( 1, 2.0 );

	// Act
	sut.emplace_back( 3, 4.0 );

	// Assert
	EXPECT_EQ( sut.front().x_, 1 );
	EXPECT_EQ( sut.front().y_, 2.0 );
	EXPECT_EQ( sut.back().x_, 3 );
	EXPECT_EQ( sut.back().y_, 4.0 );
}

TEST( OffsetList_Push, CanEmplaceBack3 )
{
	// Arrange
	offset_list<EmplacementTestData> sut;
	sut.emplace_back( 1, 2.0 );
	sut.emplace_back( 3, 4.0 );

	// Act
	sut.emplace_back( 5, 6.0 );

	// Assert
	EXPECT_EQ( sut.front().x_, 1 );
	EXPECT_EQ( sut.front().y_, 2.0 );
	EXPECT_EQ( sut.back().x_, 5 );
	EXPECT_EQ( sut.back().y_, 6.0 );
}

TEST( OffsetList_BeginEnd, CanBeginEnd1 )
{
	// Arrange
	offset_list<int> sut;

	// Act
	auto bit = sut.begin();
	auto eit = sut.end();

	// Assert
	EXPECT_EQ( bit, eit );
}

TEST( OffsetList_BeginEnd, CanBeginEnd2 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );

	// Act
	auto bit = sut.begin();
	auto eit = sut.end();
	eit--;

	// Assert
	EXPECT_NE( bit, eit );
	EXPECT_EQ( *bit, 1 );
	EXPECT_EQ( *eit, 2 );
}

TEST( OffsetList_BeginEnd, CanBeginEnd3 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto bit = sut.begin();
	auto eit = sut.end();
	eit--;

	// Assert
	EXPECT_NE( bit, eit );
	EXPECT_EQ( *bit, 1 );
	EXPECT_EQ( *eit, 3 );
}

TEST( OffsetList_BeginEnd, CanRbeginRend1 )
{
	// Arrange
	offset_list<int> sut;

	// Act
	auto bit = sut.rbegin();
	auto eit = sut.rend();

	// Assert
	EXPECT_EQ( bit, eit );
}

TEST( OffsetList_BeginEnd, CanRbeginRend2 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );

	// Act
	auto bit = sut.rbegin();
	auto eit = sut.rend();
	eit--;

	// Assert
	EXPECT_NE( bit, eit );
	EXPECT_EQ( *bit, 2 );
	EXPECT_EQ( *eit, 1 );
}

TEST( OffsetList_BeginEnd, CanRbeginRend3 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto bit = sut.rbegin();
	auto eit = sut.rend();
	eit--;

	// Assert
	EXPECT_NE( bit, eit );
	EXPECT_EQ( *bit, 3 );
	EXPECT_EQ( *eit, 1 );
}

TEST( OffsetList_BeginEnd, CanCBeginCEnd1 )
{
	// Arrange
	offset_list<int> sut;

	// Act
	auto bit = sut.cbegin();
	auto eit = sut.cend();

	// Assert
	EXPECT_EQ( bit, eit );
}

TEST( OffsetList_BeginEnd, CanCBeginCEnd2 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );

	// Act
	auto bit = sut.cbegin();
	auto eit = sut.cend();
	eit--;

	// Assert
	EXPECT_NE( bit, eit );
	EXPECT_EQ( *bit, 1 );
	EXPECT_EQ( *eit, 2 );
}

TEST( OffsetList_BeginEnd, CanCBeginCEnd3 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto bit = sut.cbegin();
	auto eit = sut.cend();
	eit--;

	// Assert
	EXPECT_NE( bit, eit );
	EXPECT_EQ( *bit, 1 );
	EXPECT_EQ( *eit, 3 );
}

TEST( OffsetList_BeginEnd, CanCRbeginCRend1 )
{
	// Arrange
	offset_list<int> sut;

	// Act
	auto bit = sut.crbegin();
	auto eit = sut.crend();

	// Assert
	EXPECT_EQ( bit, eit );
}

TEST( OffsetList_BeginEnd, CanCRbeginCRend2 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );

	// Act
	auto bit = sut.crbegin();
	auto eit = sut.crend();
	eit--;

	// Assert
	EXPECT_NE( bit, eit );
	EXPECT_EQ( *bit, 2 );
	EXPECT_EQ( *eit, 1 );
}

TEST( OffsetList_BeginEnd, CanCRbeginCRend3 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto bit = sut.crbegin();
	auto eit = sut.crend();
	eit--;

	// Assert
	EXPECT_NE( bit, eit );
	EXPECT_EQ( *bit, 3 );
	EXPECT_EQ( *eit, 1 );
}

TEST( OffsetList_Insert, CanInsertToEmptyList1 )
{
	// Arrange
	offset_list<int> sut;
	auto             it = sut.begin();

	// Act
	auto ret = sut.insert( it, 1 );

	// Assert
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_EQ( ret, sut.begin() );
	EXPECT_EQ( ret, sut.cbegin() );
	EXPECT_EQ( *ret, 1 );
}

TEST( OffsetList_Insert, CanInsertToEmptyList2 )
{
	// Arrange
	offset_list<int> sut;
	auto             it = sut.cbegin();

	// Act
	auto ret = sut.insert( it, 1 );

	// Assert
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_EQ( ret, sut.cbegin() );
	EXPECT_EQ( ret, sut.begin() );
	EXPECT_EQ( *ret, 1 );
}

TEST( OffsetList_Insert, CanInsertTo1ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 2 );
	auto bit = sut.begin();
	auto eit = sut.end();

	// Act
	auto ret1 = sut.insert( bit, 1 );
	auto ret2 = ret1;
	ret2++;
	auto ret3 = sut.insert( eit, 3 );

	// Assert
	EXPECT_EQ( sut.size(), 3 );
	EXPECT_EQ( ret1, sut.begin() );
	EXPECT_EQ( *ret1, 1 );
	EXPECT_EQ( *ret2, 2 );
	EXPECT_EQ( *ret3, 3 );
	eit--;
	EXPECT_EQ( ret3, eit );
}

TEST( OffsetList_Insert, CanInsertTo2ItemList1 )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 2 );
	sut.push_back( 4 );
	auto bit = sut.begin();
	auto eit = sut.end();

	// Act
	auto ret1 = sut.insert( bit, 1 );
	auto ret2 = ret1;
	ret2++;
	auto ret4 = ret2;
	ret4++;
	auto ret3 = sut.insert( ret4, 3 );
	auto ret5 = sut.insert( eit, 5 );

	// Assert
	EXPECT_EQ( sut.size(), 5 );
	EXPECT_EQ( ret1, sut.begin() );
	EXPECT_EQ( *ret1, 1 );
	EXPECT_EQ( *ret2, 2 );
	EXPECT_EQ( *ret3, 3 );
	EXPECT_EQ( *ret4, 4 );
	EXPECT_EQ( *ret5, 5 );
	eit--;
	EXPECT_EQ( ret5, eit );
}

TEST( OffsetList_Insert, CanInsertTo2ItemList2 )
{
	// Arrange
	auto                             td1 = EmplacementTestData { 1, 2.0 };
	auto                             td2 = EmplacementTestData { 3, 4.0 };
	auto                             td3 = EmplacementTestData { 5, 6.0 };
	auto                             td4 = EmplacementTestData { 7, 8.0 };
	auto                             td5 = EmplacementTestData { 9, 10.0 };
	offset_list<EmplacementTestData> sut;
	sut.push_back( EmplacementTestData { 3, 4.0 } );
	sut.push_back( EmplacementTestData { 7, 8.0 } );
	auto bit = sut.begin();
	auto eit = sut.end();

	// Act
	auto ret1 = sut.insert( bit, EmplacementTestData { 1, 2.0 } );
	auto ret2 = ret1;
	ret2++;
	auto ret4 = ret2;
	ret4++;
	auto ret3 = sut.insert( ret4, EmplacementTestData { 5, 6.0 } );
	auto ret5 = sut.insert( eit, EmplacementTestData { 9, 10.0 } );

	// Assert
	EXPECT_EQ( sut.size(), 5 );
	EXPECT_EQ( ret1, sut.begin() );
	EXPECT_EQ( *ret1, td1 );
	EXPECT_EQ( *ret2, td2 );
	EXPECT_EQ( *ret3, td3 );
	EXPECT_EQ( *ret4, td4 );
	EXPECT_EQ( *ret5, td5 );
	eit--;
	EXPECT_EQ( ret5, eit );
}

TEST( OffsetList_Insert, CanEmplaceTo2ItemList )
{
	// Arrange
	auto                             td1 = EmplacementTestData { 1, 2.0 };
	auto                             td2 = EmplacementTestData { 3, 4.0 };
	auto                             td3 = EmplacementTestData { 5, 6.0 };
	auto                             td4 = EmplacementTestData { 7, 8.0 };
	auto                             td5 = EmplacementTestData { 9, 10.0 };
	offset_list<EmplacementTestData> sut;
	sut.push_back( EmplacementTestData { 3, 4.0 } );
	sut.push_back( EmplacementTestData { 7, 8.0 } );
	auto bit = sut.begin();
	auto eit = sut.end();

	// Act
	auto ret1 = sut.emplace( bit, 1, 2.0 );
	auto ret2 = ret1;
	ret2++;
	auto ret4 = ret2;
	ret4++;
	auto ret3 = sut.emplace( ret4, 5, 6.0 );
	auto ret5 = sut.emplace( eit, 9, 10.0 );

	// Assert
	EXPECT_EQ( sut.size(), 5 );
	EXPECT_EQ( ret1, sut.begin() );
	EXPECT_EQ( *ret1, td1 );
	EXPECT_EQ( *ret2, td2 );
	EXPECT_EQ( *ret3, td3 );
	EXPECT_EQ( *ret4, td4 );
	EXPECT_EQ( *ret5, td5 );
	eit--;
	EXPECT_EQ( ret5, eit );
}

TEST( OffsetList_Pop, CanPopFrontFromEmpty )
{
	// Arrange
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut.pop_front() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Pop, CanPopBackFromEmpty )
{
	// Arrange
	offset_list<int> sut;

	// Act
	ASSERT_NO_THROW( sut.pop_back() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Pop, CanPopFrontFrom1ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );

	// Act
	ASSERT_NO_THROW( sut.pop_front() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Pop, CanPopBackFrom1ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );

	// Act
	ASSERT_NO_THROW( sut.pop_back() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Pop, CanPopFrontFrom2ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	sut.push_back( 2 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 2 );

	// Act
	ASSERT_NO_THROW( sut.pop_front() );

	// Assert
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_NE( sut.begin(), sut.end() );
	EXPECT_EQ( sut.front(), 2 );
	EXPECT_EQ( sut.back(), 2 );
}

TEST( OffsetList_Pop, CanPopBackFrom2ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	sut.push_back( 2 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 2 );

	// Act
	ASSERT_NO_THROW( sut.pop_back() );

	// Assert
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_NE( sut.begin(), sut.end() );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 1 );
}

TEST( OffsetList_Erase, CanEraseFromEmpty )
{
	// Arrange
	offset_list<int>           sut;
	offset_list<int>::iterator ret1, ret2;

	// Act
	ASSERT_NO_THROW( ret1 = sut.erase( sut.begin() ) );
	ASSERT_NO_THROW( ret2 = sut.erase( sut.end() ) );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
	EXPECT_EQ( ret1, sut.end() );
	EXPECT_EQ( ret2, sut.end() );
}

TEST( OffsetList_Erase, CanEraseFrom1ItemList1 )
{
	// Arrange
	offset_list<int>           sut;
	offset_list<int>::iterator ret;
	sut.push_back( 1 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );

	// Act
	ASSERT_NO_THROW( ret = sut.erase( sut.begin() ) );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
	EXPECT_EQ( ret, sut.end() );
}

TEST( OffsetList_Erase, CanEraseFrom1ItemList2 )
{
	// Arrange
	offset_list<int>           sut;
	offset_list<int>::iterator ret;
	sut.push_back( 1 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );
	auto eit = sut.end();
	eit--;

	// Act
	ASSERT_NO_THROW( ret = sut.erase( eit ) );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
	EXPECT_EQ( ret, sut.end() );
}

TEST( OffsetList_Erase, CanEraseFrom1ItemList3 )
{
	// Arrange
	offset_list<int>           sut;
	offset_list<int>::iterator ret;
	sut.push_back( 1 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );

	// Act
	ASSERT_NO_THROW( ret = sut.erase( sut.end() ) );

	// Assert
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_NE( sut.begin(), sut.end() );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( ret, sut.end() );
}

TEST( OffsetList_Erase, CanEraseFrom2ItemList1 )
{
	// Arrange
	offset_list<int>           sut;
	offset_list<int>::iterator ret;
	sut.push_back( 1 );
	sut.push_back( 2 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 2 );
	offset_list<int>::iterator td = ++( sut.begin() );

	// Act
	ASSERT_NO_THROW( ret = sut.erase( sut.begin() ) );

	// Assert
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_NE( sut.begin(), sut.end() );
	EXPECT_EQ( sut.front(), 2 );
	EXPECT_EQ( sut.back(), 2 );
	EXPECT_EQ( td, ret );
}

TEST( OffsetList_Erase, CanEraseFrom2ItemList2 )
{
	// Arrange
	offset_list<int>           sut;
	offset_list<int>::iterator ret;
	sut.push_back( 1 );
	sut.push_back( 2 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 2 );
	auto eit = sut.end();
	eit--;

	// Act
	ASSERT_NO_THROW( ret = sut.erase( eit ) );

	// Assert
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );
	EXPECT_NE( sut.begin(), sut.end() );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 1 );
	EXPECT_EQ( ret, sut.end() );
}

TEST( OffsetList_Erase, CanEraseFrom2ItemList3 )
{
	// Arrange
	offset_list<int>           sut;
	offset_list<int>::iterator ret;
	sut.push_back( 1 );
	sut.push_back( 2 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 2 );

	// Act
	ASSERT_NO_THROW( ret = sut.erase( sut.end() ) );

	// Assert
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 2 );
	EXPECT_NE( sut.begin(), sut.end() );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 2 );
	EXPECT_EQ( ret, sut.end() );
}

TEST( OffsetList_Erase, CanEraseFrom3ItemList )
{
	// Arrange
	offset_list<int>           sut;
	offset_list<int>::iterator ret;
	sut.push_back( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 3 );
	auto                       it_2 = ++( sut.begin() );
	offset_list<int>::iterator it_3 = it_2;
	it_3++;

	// Act
	ASSERT_NO_THROW( ret = sut.erase( it_2 ) );

	// Assert
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 2 );
	EXPECT_NE( sut.begin(), sut.end() );
	EXPECT_EQ( sut.front(), 1 );
	EXPECT_EQ( sut.back(), 3 );
	EXPECT_EQ( ret, it_3 );
}

TEST( OffsetList_Erase, CanClearFromEmpty )
{
	// Arrange
	offset_list<int> sut;
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );

	// Act
	ASSERT_NO_THROW( sut.clear() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Erase, CanClearFrom1ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 1 );

	// Act
	ASSERT_NO_THROW( sut.clear() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Erase, CanClearFrom2ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	sut.push_back( 2 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 2 );

	// Act
	ASSERT_NO_THROW( sut.clear() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Erase, CanClearFrom3ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 3 );

	// Act
	ASSERT_NO_THROW( sut.clear() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Swap, CanSwapBwEmptyAndEmpty )
{
	// Arrange
	offset_list<int> sut1;
	offset_list<int> sut2;

	// Act
	ASSERT_NO_THROW( sut1.swap( sut2 ) );

	// Assert
	EXPECT_TRUE( sut1.empty() );
	EXPECT_EQ( sut1.size(), 0 );
	EXPECT_EQ( sut1.begin(), sut1.end() );
	EXPECT_TRUE( sut2.empty() );
	EXPECT_EQ( sut2.size(), 0 );
	EXPECT_EQ( sut2.begin(), sut2.end() );
}

TEST( OffsetList_Swap, CanSwapBwEmptyAnd1Item )
{
	// Arrange
	offset_list<int> sut1;
	offset_list<int> sut2;
	sut2.push_back( 1 );
	EXPECT_FALSE( sut2.empty() );
	EXPECT_EQ( sut2.size(), 1 );

	// Act
	ASSERT_NO_THROW( sut1.swap( sut2 ) );

	// Assert
	EXPECT_FALSE( sut1.empty() );
	EXPECT_EQ( sut1.size(), 1 );
	EXPECT_NE( sut1.begin(), sut1.end() );
	EXPECT_EQ( sut1.front(), 1 );
	EXPECT_EQ( sut1.back(), 1 );
	EXPECT_TRUE( sut2.empty() );
	EXPECT_EQ( sut2.size(), 0 );
	EXPECT_EQ( sut2.begin(), sut2.end() );
}

TEST( OffsetList_Swap, CanSwapBwEmptyAnd2Item )
{
	// Arrange
	offset_list<int> sut1;
	offset_list<int> sut2 = { 1, 2 };
	EXPECT_FALSE( sut2.empty() );
	EXPECT_EQ( sut2.size(), 2 );

	// Act
	ASSERT_NO_THROW( sut1.swap( sut2 ) );

	// Assert
	EXPECT_FALSE( sut1.empty() );
	EXPECT_EQ( sut1.size(), 2 );
	EXPECT_NE( sut1.begin(), sut1.end() );
	EXPECT_EQ( sut1.front(), 1 );
	EXPECT_EQ( sut1.back(), 2 );
	EXPECT_TRUE( sut2.empty() );
	EXPECT_EQ( sut2.size(), 0 );
	EXPECT_EQ( sut2.begin(), sut2.end() );
}

TEST( OffsetList_Swap, CanSwapBw3ItemAnd2Item )
{
	// Arrange
	offset_list<int> sut1 = { 1, 2, 3 };
	offset_list<int> sut2 = { 4, 5 };
	EXPECT_FALSE( sut1.empty() );
	EXPECT_EQ( sut1.size(), 3 );
	EXPECT_FALSE( sut2.empty() );
	EXPECT_EQ( sut2.size(), 2 );

	// Act
	ASSERT_NO_THROW( sut1.swap( sut2 ) );

	// Assert
	EXPECT_FALSE( sut1.empty() );
	EXPECT_EQ( sut1.size(), 2 );
	EXPECT_NE( sut1.begin(), sut1.end() );
	EXPECT_EQ( sut1.front(), 4 );
	EXPECT_EQ( sut1.back(), 5 );
	EXPECT_FALSE( sut2.empty() );
	EXPECT_EQ( sut2.size(), 3 );
	EXPECT_NE( sut2.begin(), sut2.end() );
	EXPECT_EQ( sut2.front(), 1 );
	EXPECT_EQ( sut2.back(), 3 );
}

TEST( OffsetList_Swap, CanSwapFrom3ItemList )
{
	// Arrange
	offset_list<int> sut;
	sut.push_back( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );
	EXPECT_FALSE( sut.empty() );
	EXPECT_EQ( sut.size(), 3 );

	// Act
	ASSERT_NO_THROW( sut.clear() );

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
	EXPECT_EQ( sut.begin(), sut.end() );
}

TEST( OffsetList_Iterator, CanIteratePreIncrementPreDecrement )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto it1 = sut.begin();
	auto it2 = ++it1;   // 2
	auto it3 = ++it1;   // 3
	auto it4 = ++it1;   // end
	auto it5 = --it1;   // 3
	auto it6 = --it1;   // 2
	auto it7 = --it1;   // 1, begin

	// Assert
	EXPECT_EQ( it1, sut.begin() );
	EXPECT_EQ( *it1, 1 );
	EXPECT_EQ( *it2, 2 );
	EXPECT_EQ( *it3, 3 );
	EXPECT_EQ( it4, sut.end() );
	EXPECT_EQ( *it5, 3 );
	EXPECT_EQ( *it6, 2 );
	EXPECT_EQ( *it7, 1 );
	EXPECT_EQ( it7, sut.begin() );
}

TEST( OffsetList_Iterator, CanConstIteratePreIncrementPreDecrement )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto it1 = sut.cbegin();
	auto it2 = ++it1;   // 2
	auto it3 = ++it1;   // 3
	auto it4 = ++it1;   // end
	auto it5 = --it1;   // 3
	auto it6 = --it1;   // 2
	auto it7 = --it1;   // 1, begin

	// Assert
	EXPECT_EQ( it1, sut.cbegin() );
	EXPECT_EQ( it1, sut.begin() );
	EXPECT_EQ( *it1, 1 );
	EXPECT_EQ( *it2, 2 );
	EXPECT_EQ( *it3, 3 );
	EXPECT_EQ( it4, sut.cend() );
	EXPECT_EQ( it4, sut.end() );
	EXPECT_EQ( *it5, 3 );
	EXPECT_EQ( *it6, 2 );
	EXPECT_EQ( *it7, 1 );
	EXPECT_EQ( it7, sut.cbegin() );
	EXPECT_EQ( it7, sut.begin() );
}

TEST( OffsetList_Iterator, CanIteratePostIncrementPreDecrement )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto it1 = sut.begin();
	auto it2 = it1++;   // 1
	auto it3 = it1++;   // 2
	auto it4 = it1++;   // 3
	auto it5 = it1--;   // end
	auto it6 = it1--;   // 3
	auto it7 = it1--;   // 2

	// Assert
	EXPECT_EQ( it1, sut.begin() );
	EXPECT_EQ( *it1, 1 );
	EXPECT_EQ( *it2, 1 );
	EXPECT_EQ( *it3, 2 );
	EXPECT_EQ( *it4, 3 );
	EXPECT_EQ( it5, sut.end() );
	EXPECT_EQ( *it6, 3 );
	EXPECT_EQ( *it7, 2 );
}

TEST( OffsetList_Iterator, CanConstIteratePostIncrementPreDecrement )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto it1 = sut.cbegin();
	auto it2 = it1++;   // 1
	auto it3 = it1++;   // 2
	auto it4 = it1++;   // 3
	auto it5 = it1--;   // end
	auto it6 = it1--;   // 3
	auto it7 = it1--;   // 2

	// Assert
	EXPECT_EQ( it1, sut.cbegin() );
	EXPECT_EQ( it1, sut.begin() );
	EXPECT_EQ( *it1, 1 );
	EXPECT_EQ( *it2, 1 );
	EXPECT_EQ( *it3, 2 );
	EXPECT_EQ( *it4, 3 );
	EXPECT_EQ( it5, sut.cend() );
	EXPECT_EQ( it5, sut.end() );
	EXPECT_EQ( *it6, 3 );
	EXPECT_EQ( *it7, 2 );
}

TEST( OffsetList_ReverseIterator, CanReverseIteratePreIncrementPreDecrement )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto it1 = sut.rbegin();
	auto it2 = ++it1;   // 2
	auto it3 = ++it1;   // 1
	auto it4 = ++it1;   // rend
	auto it5 = --it1;   // 1
	auto it6 = --it1;   // 2
	auto it7 = --it1;   // 3, rbegin

	// Assert
	EXPECT_EQ( it1, sut.rbegin() );
	EXPECT_EQ( *it1, 3 );
	EXPECT_EQ( *it2, 2 );
	EXPECT_EQ( *it3, 1 );
	EXPECT_EQ( it4, sut.rend() );
	EXPECT_EQ( *it5, 1 );
	EXPECT_EQ( *it6, 2 );
	EXPECT_EQ( *it7, 3 );
	EXPECT_EQ( it7, sut.rbegin() );
}

TEST( OffsetList_ReverseIterator, CanConstReverseIteratePreIncrementPreDecrement )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto it1 = sut.crbegin();
	auto it2 = ++it1;   // 2
	auto it3 = ++it1;   // 1
	auto it4 = ++it1;   // crend
	auto it5 = --it1;   // 1
	auto it6 = --it1;   // 2
	auto it7 = --it1;   // 3, crbegin

	// Assert
	EXPECT_EQ( it1, sut.crbegin() );
	EXPECT_EQ( it1, sut.rbegin() );
	EXPECT_EQ( *it1, 3 );
	EXPECT_EQ( *it2, 2 );
	EXPECT_EQ( *it3, 1 );
	EXPECT_EQ( it4, sut.crend() );
	EXPECT_EQ( it4, sut.rend() );
	EXPECT_EQ( *it5, 1 );
	EXPECT_EQ( *it6, 2 );
	EXPECT_EQ( *it7, 3 );
	EXPECT_EQ( it7, sut.crbegin() );
	EXPECT_EQ( it7, sut.rbegin() );
}

TEST( OffsetList_ReverseIterator, CanReverseIteratePostIncrementPreDecrement )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto it1 = sut.rbegin();
	auto it2 = it1++;   // 3
	auto it3 = it1++;   // 2
	auto it4 = it1++;   // 1
	auto it5 = it1--;   // end
	auto it6 = it1--;   // 1
	auto it7 = it1--;   // 2

	// Assert
	EXPECT_EQ( it1, sut.rbegin() );
	EXPECT_EQ( *it1, 3 );
	EXPECT_EQ( *it2, 3 );
	EXPECT_EQ( *it3, 2 );
	EXPECT_EQ( *it4, 1 );
	EXPECT_EQ( it5, sut.rend() );
	EXPECT_EQ( *it6, 1 );
	EXPECT_EQ( *it7, 2 );
}

TEST( OffsetList_ReverseIterator, CanConstReverseIteratePostIncrementPreDecrement )
{
	// Arrange
	offset_list<int> sut;
	sut.push_front( 1 );
	sut.push_back( 2 );
	sut.push_back( 3 );

	// Act
	auto it1 = sut.crbegin();
	auto it2 = it1++;   // 3
	auto it3 = it1++;   // 2
	auto it4 = it1++;   // 1
	auto it5 = it1--;   // end
	auto it6 = it1--;   // 1
	auto it7 = it1--;   // 2

	// Assert
	EXPECT_EQ( it1, sut.crbegin() );
	EXPECT_EQ( it1, sut.rbegin() );
	EXPECT_EQ( *it1, 3 );
	EXPECT_EQ( *it2, 3 );
	EXPECT_EQ( *it3, 2 );
	EXPECT_EQ( *it4, 1 );
	EXPECT_EQ( it5, sut.crend() );
	EXPECT_EQ( it5, sut.rend() );
	EXPECT_EQ( *it6, 1 );
	EXPECT_EQ( *it7, 2 );
}

TEST( OffsetList, CanCallMaxSize )
{
	// Arrange
	offset_list<int> sut;

	// Act
	auto ret = sut.max_size();

	// Assert
	EXPECT_GT( ret, 0 );
}

TEST( OffsetList_Allocator, CanConstruct )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc         malloc_obj( p_mem, 1024 );
	offset_allocator<int> allocator_obj( malloc_obj );

	// Act
	offset_list<int, offset_allocator<int>> sut( allocator_obj );

	// Assert
}

TEST( OffsetList_Allocator, CanPush )
{
	// Arrange
	unsigned char                           test_buff[1024];
	void*                                   p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                           malloc_obj( p_mem, 1024 );
	offset_allocator<int>                   allocator_obj( malloc_obj );
	offset_list<int, offset_allocator<int>> sut( allocator_obj );

	// Act
	sut.push_back( 1 );

	// Assert
	auto it            = sut.begin();
	int* p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanCopyConstruct )
{
	// Arrange
	unsigned char                           test_buff[1024];
	void*                                   p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                           malloc_obj( p_mem, 1024 );
	offset_allocator<int>                   allocator_obj( malloc_obj );
	offset_list<int, offset_allocator<int>> src( allocator_obj );
	src.push_back( 1 );

	// Act
	offset_list<int, offset_allocator<int>> sut( src );

	// Assert
	auto it            = sut.begin();
	int* p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
	EXPECT_NE( it, src.begin() );
}

TEST( OffsetList_Allocator, CanMoveConstruct )
{
	// Arrange
	unsigned char                           test_buff[1024];
	void*                                   p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                           malloc_obj( p_mem, 1024 );
	offset_allocator<int>                   allocator_obj( malloc_obj );
	offset_list<int, offset_allocator<int>> src( allocator_obj );
	src.push_back( 1 );

	// Act
	offset_list<int, offset_allocator<int>> sut( std::move( src ) );

	// Assert
	auto it            = sut.begin();
	int* p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );

	sut.push_front( 2 );
	it            = sut.begin();
	p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanCopyAssingment )
{
	// Arrange
	unsigned char                           test_buff[1024];
	void*                                   p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                           malloc_obj( p_mem, 1024 );
	offset_allocator<int>                   allocator_obj( malloc_obj );
	offset_list<int, offset_allocator<int>> src( allocator_obj );
	src.push_back( 1 );

	unsigned char                           test_buff2[1024];
	void*                                   p_mem2 = reinterpret_cast<void*>( test_buff2 );
	offset_malloc                           malloc_obj2( p_mem2, 1024 );
	offset_allocator<int>                   allocator_obj2( malloc_obj2 );
	offset_list<int, offset_allocator<int>> sut( allocator_obj2 );

	// Act
	sut = src;

	// Assert
	auto it            = sut.begin();
	int* p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj2.is_belong_to( p_target_data ) );
	EXPECT_NE( it, src.begin() );
}

TEST( OffsetList_Allocator, CanMoveAssignment )
{
	// Arrange
	unsigned char                           test_buff[1024];
	void*                                   p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                           malloc_obj( p_mem, 1024 );
	offset_allocator<int>                   allocator_obj( malloc_obj );
	offset_list<int, offset_allocator<int>> src( allocator_obj );
	src.push_back( 1 );

	unsigned char                           test_buff2[1024];
	void*                                   p_mem2 = reinterpret_cast<void*>( test_buff2 );
	offset_malloc                           malloc_obj2( p_mem2, 1024 );
	offset_allocator<int>                   allocator_obj2( malloc_obj2 );
	offset_list<int, offset_allocator<int>> sut( allocator_obj2 );

	// Act
	sut = std::move( src );

	// Assert
	auto it            = sut.begin();
	int* p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj2.is_belong_to( p_target_data ) );

	sut.push_front( 2 );
	it            = sut.begin();
	p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj2.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanTypeSelf )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc         malloc_obj( p_mem, 1024 );
	offset_allocator<int> allocator_obj( malloc_obj );

	// Act
	offset_list<offset_list<int>, offset_allocator<offset_list<int>>> sut( allocator_obj );

	// Assert
}

TEST( OffsetList_Allocator, CanEmplaceFrontUsesAllocatableType )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );

	// Act
	sut.emplace_front();

	// Assert
	auto  it            = sut.begin();
	auto* p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanEmplaceBackUsesAllocatableType )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );

	// Act
	sut.emplace_back();

	// Assert
	auto  it            = sut.begin();
	auto* p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanInsertUsesAllocatableType )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );
	auto                                                            bit = sut.begin();

	// Act
	auto it = sut.insert( bit, TestElementType() );

	// Assert
	auto* p_target_data = &( *it );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanEmplaceToMovingPushedInnerElement1 )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );
	sut.push_back( TestElementType() );
	auto it = sut.begin();

	// Act
	it->emplace_back( 1.0 );

	// Assert
	auto  it2           = it->begin();
	auto* p_target_data = &( *it2 );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanEmplaceToMovingPushedInnerElement2 )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char            test_buff2[1024];
	void*                    p_mem2 = reinterpret_cast<void*>( test_buff2 );
	offset_malloc            malloc_obj2( p_mem2, 1024 );
	offset_allocator<double> allocator_obj2( malloc_obj2 );
	TestElementType          src( allocator_obj2 );

	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );

	sut.push_back( std::move( src ) );
	auto it = sut.begin();

	// Act
	it->emplace_back( 1.0 );

	// Assert
	auto it2 = it->begin();
	EXPECT_EQ( *it2, 1.0 );
	auto* p_target_data = &( *it2 );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanEmplaceToMovingPushedInnerElement3 )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char            test_buff2[1024];
	void*                    p_mem2 = reinterpret_cast<void*>( test_buff2 );
	offset_malloc            malloc_obj2( p_mem2, 1024 );
	offset_allocator<double> allocator_obj2( malloc_obj2 );
	TestElementType          src( allocator_obj2 );
	src.emplace_back( 1.0 );

	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );

	// Act
	sut.push_back( std::move( src ) );

	// Assert
	auto it  = sut.begin();
	auto it2 = it->begin();
	EXPECT_EQ( *it2, 1.0 );
	auto* p_target_data = &( *it2 );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );   // moveが行われた場合、move先であるコンテナのメモリリソースの管理下に置かれるべき。
	                                                           // よって、値管理を行うノードのアドレスは、sut側のメモリリソースに属することを確認する。
	                                                           // なお、値そのものは、moveが行われるべきである。
	EXPECT_FALSE( malloc_obj2.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanEmplaceToCopyingPushedInnerElement1 )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char test_buff[1024];
	void*         p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc malloc_obj( p_mem, 1024 );

	offset_allocator<double> allocator_obj2( malloc_obj );
	TestElementType          src( allocator_obj2 );

	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );

	sut.push_back( src );
	auto it = sut.begin();

	// Act
	it->emplace_back( 1.0 );

	// Assert
	auto it2 = it->begin();
	EXPECT_EQ( *it2, 1.0 );
	auto* p_target_data = &( *it2 );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanEmplaceToCopyingPushedInnerElement2 )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char            test_buff2[1024];
	void*                    p_mem2 = reinterpret_cast<void*>( test_buff2 );
	offset_malloc            malloc_obj2( p_mem2, 1024 );
	offset_allocator<double> allocator_obj2( malloc_obj2 );
	TestElementType          src( allocator_obj2 );

	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );

	sut.push_back( src );
	auto it = sut.begin();

	// Act
	it->emplace_back( 1.0 );

	// Assert
	auto it2 = it->begin();
	EXPECT_EQ( *it2, 1.0 );
	auto* p_target_data = &( *it2 );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanEmplaceToCopyingPushedInnerElement3 )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char            test_buff2[1024];
	void*                    p_mem2 = reinterpret_cast<void*>( test_buff2 );
	offset_malloc            malloc_obj2( p_mem2, 1024 );
	offset_allocator<double> allocator_obj2( malloc_obj2 );
	TestElementType          src( allocator_obj2 );
	src.emplace_back( 1.0 );

	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<TestElementType>                               allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );

	// Act
	sut.push_back( src );

	// Assert
	auto it  = sut.begin();
	auto it2 = it->begin();
	EXPECT_EQ( *it2, 1.0 );
	auto* p_target_data = &( *it2 );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}

TEST( OffsetList_Allocator, CanEmplaceToEmplacedInnerElement )
{
	// Arrange
	using TestElementType = offset_list<double, offset_allocator<double>>;
	unsigned char                                                   test_buff[1024];
	void*                                                           p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc                                                   malloc_obj( p_mem, 1024 );
	offset_allocator<int>                                           allocator_obj( malloc_obj );
	offset_list<TestElementType, offset_allocator<TestElementType>> sut( allocator_obj );
	sut.emplace_back();
	auto it = sut.begin();

	// Act
	it->emplace_back( 1.0 );

	// Assert
	auto  it2           = it->begin();
	auto* p_target_data = &( *it2 );
	EXPECT_TRUE( malloc_obj.is_belong_to( p_target_data ) );
}
