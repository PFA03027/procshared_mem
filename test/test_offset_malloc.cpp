/**
 * @file test_offset_malloc.cpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-10-07
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include "gtest/gtest.h"

#include "offset_malloc.hpp"

#include "test_procshared_common.hpp"

TEST( Offset_MemAllocator_Cntr, CanConstruct )
{
	// Arrange
	unsigned char  test_buff[1024];
	void*          p_mem       = reinterpret_cast<void*>( test_buff );
	offset_malloc* p_mem_alloc = nullptr;

	// Act
	ASSERT_NO_THROW( p_mem_alloc = new offset_malloc( p_mem, 1024 ) );

	// Assert
	EXPECT_NE( p_mem_alloc, nullptr );

	// Clean-up
	delete p_mem_alloc;
}

TEST( Offset_MemAllocator_Cntr, CanCopyConstruct )
{
	// Arrange
	unsigned char test_buff[1024];
	void*         p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc mem_alloc1( p_mem, 1024 );

	// Act
	ASSERT_NO_THROW( offset_malloc mem_alloc2 = mem_alloc1 );

	// Assert

	// Clean-up
}

TEST( Offset_MemAllocator_Cntr, CanMoveConstruct )
{
	// Arrange
	unsigned char test_buff[1024];
	void*         p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc mem_alloc1( p_mem, 1024 );

	// Act
	ASSERT_NO_THROW( offset_malloc mem_alloc2 = std::move( mem_alloc1 ) );

	// Assert

	// Clean-up
}

TEST( Offset_MemAllocator_Cntr, CanCopyAssignment1 )
{
	// Arrange
	unsigned char test_buff[1024];
	void*         p_mem = reinterpret_cast<void*>( test_buff );
	offset_malloc mem_alloc1( p_mem, 1024 );
	offset_malloc mem_alloc2;

	// Act
	ASSERT_NO_THROW( mem_alloc2 = mem_alloc1 );

	// Assert

	// Clean-up
}

TEST( Offset_MemAllocator_Cntr, CanCopyAssignment2 )
{
	// Arrange
	unsigned char test_buff1[1024];
	unsigned char test_buff2[1024];
	offset_malloc mem_alloc1( reinterpret_cast<void*>( test_buff1 ), 1024 );
	offset_malloc mem_alloc2( reinterpret_cast<void*>( test_buff2 ), 1024 );

	// Act
	ASSERT_NO_THROW( mem_alloc2 = mem_alloc1 );

	// Assert

	// Clean-up
}

TEST( Offset_MemAllocator_Cntr, CanMoveAssignment1 )
{
	// Arrange
	unsigned char test_buff[1024];
	offset_malloc mem_alloc1( reinterpret_cast<void*>( test_buff ), 1024 );
	offset_malloc mem_alloc2;

	// Act
	ASSERT_NO_THROW( mem_alloc2 = std::move( mem_alloc1 ) );

	// Assert

	// Clean-up
}

TEST( Offset_MemAllocator_Cntr, CanMoveAssignment2 )
{
	// Arrange
	unsigned char test_buff1[1024];
	unsigned char test_buff2[1024];
	offset_malloc mem_alloc1( reinterpret_cast<void*>( test_buff1 ), 1024 );
	offset_malloc mem_alloc2( reinterpret_cast<void*>( test_buff2 ), 1024 );

	// Act
	ASSERT_NO_THROW( mem_alloc2 = std::move( mem_alloc1 ) );

	// Assert

	// Clean-up
}

TEST( Offset_MemAllocator_Cntr, FailConstruct1 )
{
	// Arrange
	unsigned char test_buff[sizeof( offset_malloc ) + 1];
	void*         p_mem = reinterpret_cast<void*>( test_buff );

	// Act
	EXPECT_ANY_THROW( offset_malloc sut( p_mem, sizeof( offset_malloc ) + 1 ) );

	// Assert

	// Clean-up
}

TEST( Offset_MemAllocator_Cntr, FailConstruct2 )
{
	// Arrange
	unsigned char test_buff[1];
	void*         p_mem = reinterpret_cast<void*>( test_buff );

	// Act
	EXPECT_ANY_THROW( offset_malloc sut( p_mem, 1 ) );

	// Assert

	// Clean-up
}

class ProcShared_Malloc : public testing::Test {
	// You can implement all the usual fixture class members here.
	// To access the test parameter, call GetParam() from class
	// TestWithParam<T>.
public:
	static constexpr size_t alloc_mem_size = 1024;

	void SetUp() override
	{
		p_mem_ = malloc( alloc_mem_size + 16 );

		uintptr_t addr = reinterpret_cast<uintptr_t>( p_mem_ );
		addr           = ( ( addr + 16 - 1 ) / 16 ) * 16;   // block::block_headerのサイズでアライメントを採る。

		p_sut_ = new offset_malloc( reinterpret_cast<void*>( addr ), alloc_mem_size );
	}
	void TearDown() override
	{
		// Clean-up
		delete ( p_sut_ );
		free( p_mem_ );
	}

	void*          p_mem_;
	offset_malloc* p_sut_;
};

TEST_F( ProcShared_Malloc, CanAllocateSmall )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 10 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
}

TEST_F( ProcShared_Malloc, CanAllocateSmallwSmallAlignment )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 20, 8 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
}

TEST_F( ProcShared_Malloc, CanAllocateSmallwEQAlignment )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 20, 16 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
}

TEST_F( ProcShared_Malloc, CanAllocateSmallwBigAlignment )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 20, 128 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
}

TEST_F( ProcShared_Malloc, CanAllocateOverSize )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 1024 );

	// Assert
	EXPECT_EQ( p_allc_mem, nullptr );
}

TEST_F( ProcShared_Malloc, CanDeallocate1 )
{
	// Arrange
	void* p_allc_mem = p_sut_->allocate( 10 );

	// Act
	EXPECT_NO_THROW( p_sut_->deallocate( p_allc_mem ) );

	// Assert
}

TEST_F( ProcShared_Malloc, CanDeallocate2 )
{
	// Arrange
	void* p_allc_mem1 = p_sut_->allocate( 10 );
	void* p_allc_mem2 = p_sut_->allocate( 10 );
	void* p_allc_mem3 = p_sut_->allocate( 10 );

	// Act
	EXPECT_NO_THROW( p_sut_->deallocate( p_allc_mem2 ) );
	EXPECT_NO_THROW( p_sut_->deallocate( p_allc_mem1 ) );
	EXPECT_NO_THROW( p_sut_->deallocate( p_allc_mem3 ) );
	p_allc_mem1 = p_sut_->allocate( 10 );
	EXPECT_NO_THROW( p_sut_->deallocate( p_allc_mem1 ) );

	// Assert
}
