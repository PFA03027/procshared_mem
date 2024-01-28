/**
 * @file test_offset_malloc_impl.cpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-10-07
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include "gtest/gtest.h"

#include "offset_mallloc_impl.hpp"

#include "test_procshared_common.hpp"

using namespace ipsm;

TEST( ProcShared_KRmalloc_Cntr, CanConstruct )
{
	// Arrange
	unsigned char                      test_buff[1024];
	void*                              p_mem       = reinterpret_cast<void*>( test_buff );
	offset_malloc::offset_malloc_impl* p_mem_alloc = nullptr;

	// Act
	ASSERT_NO_THROW( p_mem_alloc = offset_malloc::offset_malloc_impl::placement_new( p_mem, reinterpret_cast<void*>( reinterpret_cast<uintptr_t>( p_mem ) + 1024 ) ) );

	// Assert
	ASSERT_NE( p_mem_alloc, nullptr );
	EXPECT_EQ( p_mem_alloc->get_bind_count(), 1 );

	// Clean-up
	offset_malloc::offset_malloc_impl::teardown( p_mem_alloc );
}

TEST( ProcShared_KRmalloc_Cntr, CanBind )
{
	// Arrange
	unsigned char                      test_buff[1024];
	void*                              p_mem        = reinterpret_cast<void*>( test_buff );
	offset_malloc::offset_malloc_impl* p_mem_alloc  = nullptr;
	offset_malloc::offset_malloc_impl* p_mem_alloc2 = nullptr;
	ASSERT_NO_THROW( p_mem_alloc = offset_malloc::offset_malloc_impl::placement_new( p_mem, reinterpret_cast<void*>( reinterpret_cast<uintptr_t>( p_mem ) + 1024 ) ) );

	// Act
	ASSERT_NO_THROW( p_mem_alloc2 = offset_malloc::offset_malloc_impl::bind( reinterpret_cast<offset_malloc::offset_malloc_impl*>( p_mem ) ) );

	// Assert
	ASSERT_NE( p_mem_alloc2, nullptr );
	EXPECT_EQ( p_mem_alloc2->get_bind_count(), 2 );

	// Clean-up
	offset_malloc::offset_malloc_impl::teardown( p_mem_alloc );
	EXPECT_EQ( p_mem_alloc2->get_bind_count(), 1 );
	offset_malloc::offset_malloc_impl::teardown( p_mem_alloc2 );
}

TEST( ProcShared_KRmalloc_Cntr, FailConstruct1 )
{
	// Arrange
	unsigned char test_buff[sizeof( offset_malloc::offset_malloc_impl ) + 10];
	void*         p_mem = reinterpret_cast<void*>( test_buff );

	// Act
	EXPECT_ANY_THROW( offset_malloc::offset_malloc_impl::placement_new( p_mem, reinterpret_cast<void*>( reinterpret_cast<uintptr_t>( p_mem ) + sizeof( offset_malloc::offset_malloc_impl ) + 10 ) ) );

	// Assert

	// Clean-up
}

TEST( ProcShared_KRmalloc_Cntr, FailConstruct2 )
{
	// Arrange
	unsigned char test_buff[10];
	void*         p_mem = reinterpret_cast<void*>( test_buff );

	// Act
	EXPECT_ANY_THROW( offset_malloc::offset_malloc_impl::placement_new( p_mem, reinterpret_cast<void*>( reinterpret_cast<uintptr_t>( p_mem ) + 10 ) ) );

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

		p_sut_ = offset_malloc::offset_malloc_impl::placement_new( reinterpret_cast<void*>( addr ), reinterpret_cast<void*>( reinterpret_cast<uintptr_t>( p_mem_ ) + alloc_mem_size + 16 ) );
	}
	void TearDown() override
	{
		// Clean-up
		offset_malloc::offset_malloc_impl::teardown( p_sut_ );
		free( p_mem_ );
	}

	void*                              p_mem_;
	offset_malloc::offset_malloc_impl* p_sut_;
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
	void* p_allc_mem = p_sut_->allocate( alloc_mem_size - sizeof( offset_malloc::offset_malloc_impl ) - offset_malloc::offset_malloc_impl::test_block_header_size() * 10 /* offset_malloc::offset_malloc_impl::block::block_header */ );
	ASSERT_NE( p_allc_mem, nullptr );

	// Act
	EXPECT_NO_THROW( p_sut_->deallocate( p_allc_mem ) );

	// Assert
}

TEST_F( ProcShared_Malloc, CanDeallocate3 )
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
