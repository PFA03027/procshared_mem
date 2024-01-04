/**
 * @file test_offset_allocator.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2024-01-04
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include "gtest/gtest.h"

#include "offset_allocator.hpp"

TEST( Offset_Allocator_Cntr, CanDefaultConstruct )
{
	// Arrange

	// Act
	offset_allocator<int> sut;

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 0 );
	EXPECT_EQ( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_Cntr, CanConstruct1 )
{
	// Arrange
	unsigned char test_buff[1024];
	void*         p_mem = reinterpret_cast<void*>( test_buff );

	// Act
	offset_allocator<int> sut( p_mem, 1024 );

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 1 );
	EXPECT_NE( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_Cntr, CanConstruct2 )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> tmp( p_mem, 1024 );

	// Act
	offset_allocator<int> sut( p_mem );

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 2 );
	EXPECT_NE( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_Cntr, CanConstruct3 )
{
	// Arrange
	unsigned char test_buff[1024];
	void*         p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc tmp( p_mem, 1024 );

	// Act
	offset_allocator<int> sut( tmp );

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 2 );
	EXPECT_NE( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_Cntr, CanConstruct4 )
{
	// Arrange
	unsigned char test_buff[1024];
	void*         p_mem = reinterpret_cast<void*>( test_buff );

	// Act
	offset_allocator<int> sut( offset_malloc( p_mem, 1024 ) );

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 1 );
	EXPECT_NE( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_Cntr, CanConstruct5 )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> tmp( p_mem, 1024 );

	// Act
	offset_allocator<double> sut( tmp );

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 2 );
	EXPECT_NE( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_CopyCntr, CanConstructFromInvalidAllocator )
{
	// Arrange
	offset_allocator<int> src;

	// Act
	offset_allocator<int> sut( src );

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 0 );
	EXPECT_EQ( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_CopyCntr, CanConstructFromValidAllocator )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> src( p_mem, 1024 );
	auto                  p_ret_from_src = src.allocate( 10 );
	EXPECT_NE( p_ret_from_src, nullptr );

	// Act
	offset_allocator<int> sut( src );
	auto                  p_ret_from_sut = sut.allocate( 10 );

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 2 );
	EXPECT_NE( p_ret_from_src, p_ret_from_sut );
	EXPECT_NE( p_ret_from_sut, nullptr );
}

TEST( Offset_Allocator_CopyAssingment, CanAssignFromInvalidAllocatorToInvalidAllocator )
{
	// Arrange
	offset_allocator<int> src;
	offset_allocator<int> sut;

	// Act

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 0 );
	EXPECT_EQ( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_CopyAssingment, CanAssignFromInvalidAllocatorToValidAllocator )
{
	// Arrange
	offset_allocator<int> src;
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> sut( p_mem, 1024 );

	// Act
	sut = src;

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 0 );
	EXPECT_EQ( sut.allocate( 10 ), nullptr );
}

TEST( Offset_Allocator_CopyAssingment, CanAssignFromValidAllocatorToInvalidAllocator )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> src( p_mem, 1024 );
	auto                  p_ret_from_src = src.allocate( 10 );
	EXPECT_NE( p_ret_from_src, nullptr );
	offset_allocator<int> sut;

	// Act
	sut                 = src;
	auto p_ret_from_sut = sut.allocate( 10 );

	// Assert
	EXPECT_EQ( sut.get_bind_count(), 2 );
	EXPECT_NE( p_ret_from_src, p_ret_from_sut );
	EXPECT_NE( p_ret_from_sut, nullptr );
}

TEST( Offset_Allocator_CopyAssingment, CanAssignFromValidAllocatorToValidAllocator )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> src( p_mem, 1024 );
	EXPECT_EQ( src.get_bind_count(), 1 );
	auto p_ret_from_src = src.allocate( 10 );
	EXPECT_NE( p_ret_from_src, nullptr );
	unsigned char         test_buff2[1024];
	void*                 p_mem2 = reinterpret_cast<void*>( test_buff2 );
	offset_allocator<int> sut( p_mem2, 1024 );
	EXPECT_EQ( sut.get_bind_count(), 1 );

	// Act
	sut                 = src;
	auto p_ret_from_sut = sut.allocate( 10 );

	// Assert
	EXPECT_EQ( src.get_bind_count(), 2 );
	EXPECT_EQ( sut.get_bind_count(), 2 );
	EXPECT_NE( p_ret_from_src, p_ret_from_sut );
	EXPECT_NE( p_ret_from_sut, nullptr );
}

TEST( Offset_Allocator, CanAllocate )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> sut( p_mem, 1024 );
	EXPECT_EQ( sut.get_bind_count(), 1 );

	// Act
	auto ret = sut.allocate( 10 );

	// Assert
	EXPECT_NE( ret, nullptr );
}

TEST( Offset_Allocator, CanDeallocate1 )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> sut( p_mem, 1024 );
	EXPECT_EQ( sut.get_bind_count(), 1 );
	auto ret = sut.allocate( 10 );

	// Act
	sut.deallocate( ret, 10 );

	// Assert
	EXPECT_NE( ret, nullptr );
}

TEST( Offset_Allocator, CanDeallocate2 )
{
	// Arrange
	unsigned char         test_buff[1024];
	void*                 p_mem = reinterpret_cast<void*>( test_buff );
	offset_allocator<int> sut( p_mem, 1024 );
	EXPECT_EQ( sut.get_bind_count(), 1 );
	int* p_pre = nullptr;
	int* p_cur = sut.allocate( 10 );
	ASSERT_NE( p_cur, nullptr );
	do {
		p_pre = p_cur;
		p_cur = sut.allocate( 10 );
	} while ( p_cur != nullptr );

	// Act
	sut.deallocate( p_pre, 10 );

	// Assert
	p_cur = sut.allocate( 10 );
	EXPECT_NE( p_cur, nullptr );
}
