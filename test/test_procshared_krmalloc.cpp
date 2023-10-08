/**
 * @file test_procshared_krmalloc.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2023-10-07
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include "gtest/gtest.h"

#include "procshared_krmalloc.hpp"

TEST( ProcShared_KRmalloc_Cntr, CanConstruct )
{
	// Arrange
	void*                    p_mem       = malloc( 1024 );
	procshared_mem_krmalloc* p_mem_alloc = nullptr;

	// Act
	ASSERT_NO_THROW( p_mem_alloc = procshared_mem_krmalloc::make( p_mem, 1024 ) );

	// Assert
	EXPECT_NE( p_mem_alloc, nullptr );

	// Clean-up
	procshared_mem_krmalloc::teardown( p_mem_alloc );
	free( p_mem );
}

TEST( ProcShared_KRmalloc_Cntr, FailConstruct1 )
{
	// Arrange
	void* p_mem = malloc( sizeof( procshared_mem_krmalloc ) + 10 );

	// Act
	EXPECT_ANY_THROW( procshared_mem_krmalloc::make( p_mem, sizeof( procshared_mem_krmalloc ) + 10 ) );

	// Assert

	// Clean-up
	free( p_mem );
}

TEST( ProcShared_KRmalloc_Cntr, FailConstruct2 )
{
	// Arrange
	void* p_mem = malloc( 10 );

	// Act
	EXPECT_ANY_THROW( procshared_mem_krmalloc::make( p_mem, 20 ) );

	// Assert

	// Clean-up
	free( p_mem );
}

class ProcShared_KRmalloc : public testing::Test {
	// You can implement all the usual fixture class members here.
	// To access the test parameter, call GetParam() from class
	// TestWithParam<T>.
public:
	static constexpr size_t alloc_mem_size_ = 1024;

	void SetUp() override
	{
		p_mem_ = malloc( alloc_mem_size_ + 16 );

		uintptr_t addr = reinterpret_cast<uintptr_t>( p_mem_ );
		addr           = ( ( addr + 16 - 1 ) / 16 ) * 16;   // block::block_headerのサイズでアライメントを採る。

		p_sut = procshared_mem_krmalloc::make( reinterpret_cast<void*>( addr ), alloc_mem_size_ );
	}
	void TearDown() override
	{
		// Clean-up
		procshared_mem_krmalloc::teardown( p_sut );
		free( p_mem_ );
	}

	void*                    p_mem_;
	procshared_mem_krmalloc* p_sut;
};

TEST_F( ProcShared_KRmalloc, CanAllocateSmall )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut->allocate( 10 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
}

TEST_F( ProcShared_KRmalloc, CanAllocateOverSize )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut->allocate( 1024 );

	// Assert
	EXPECT_EQ( p_allc_mem, nullptr );
}

TEST_F( ProcShared_KRmalloc, CanDeallocate1 )
{
	// Arrange
	void* p_allc_mem = p_sut->allocate( 10 );

	// Act
	EXPECT_NO_THROW( p_sut->deallocate( p_allc_mem ) );

	// Assert
}

TEST_F( ProcShared_KRmalloc, CanDeallocate2 )
{
	// Arrange
	void* p_allc_mem = p_sut->allocate( alloc_mem_size_ - sizeof( procshared_mem_krmalloc ) - 48 /* procshared_mem_krmalloc::block::block_header */ );
	ASSERT_NE( p_allc_mem, nullptr );

	// Act
	EXPECT_NO_THROW( p_sut->deallocate( p_allc_mem ) );

	// Assert
}

TEST_F( ProcShared_KRmalloc, CanDeallocate3 )
{
	// Arrange
	void* p_allc_mem1 = p_sut->allocate( 10 );
	void* p_allc_mem2 = p_sut->allocate( 10 );
	void* p_allc_mem3 = p_sut->allocate( 10 );

	// Act
	EXPECT_NO_THROW( p_sut->deallocate( p_allc_mem2 ) );
	EXPECT_NO_THROW( p_sut->deallocate( p_allc_mem1 ) );
	EXPECT_NO_THROW( p_sut->deallocate( p_allc_mem3 ) );
	p_allc_mem1 = p_sut->allocate( 10 );
	EXPECT_NO_THROW( p_sut->deallocate( p_allc_mem1 ) );

	// Assert
}
