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

#include <future>
#include <mutex>
#include <thread>

#include "gtest/gtest.h"

#include "offset_malloc.hpp"

#include "test_procshared_common.hpp"

TEST( Offset_Malloc_Cntr, CanConstruct )
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

TEST( Offset_Malloc_Cntr, CanCopyConstruct )
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

TEST( Offset_Malloc_Cntr, CanMoveConstruct )
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

TEST( Offset_Malloc_Cntr, CanCopyAssignment1 )
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

TEST( Offset_Malloc_Cntr, CanCopyAssignment2 )
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

TEST( Offset_Malloc_Cntr, CanMoveAssignment1 )
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

TEST( Offset_Malloc_Cntr, CanMoveAssignment2 )
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

TEST( Offset_Malloc_Cntr, FailConstruct1 )
{
	// Arrange
	unsigned char test_buff[sizeof( offset_malloc ) + 1];
	void*         p_mem = reinterpret_cast<void*>( test_buff );

	// Act
	EXPECT_ANY_THROW( offset_malloc sut( p_mem, sizeof( offset_malloc ) + 1 ) );

	// Assert

	// Clean-up
}

TEST( Offset_Malloc_Cntr, FailConstruct2 )
{
	// Arrange
	unsigned char test_buff[1];
	void*         p_mem = reinterpret_cast<void*>( test_buff );

	// Act
	EXPECT_ANY_THROW( offset_malloc sut( p_mem, 1 ) );

	// Assert

	// Clean-up
}

class Offset_Malloc : public testing::Test {
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

TEST_F( Offset_Malloc, CanAllocateSmall )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 10 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
	EXPECT_TRUE( p_sut_->is_belong_to( p_allc_mem ) );
}

TEST_F( Offset_Malloc, CanAllocateSmallwSmallAlignment )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 20, 8 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
	EXPECT_TRUE( p_sut_->is_belong_to( p_allc_mem ) );
}

TEST_F( Offset_Malloc, CanAllocateSmallwEQAlignment )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 20, 16 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
	EXPECT_TRUE( p_sut_->is_belong_to( p_allc_mem ) );
}

TEST_F( Offset_Malloc, CanAllocateSmallwBigAlignment )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 20, 128 );

	// Assert
	EXPECT_NE( p_allc_mem, nullptr );
	EXPECT_TRUE( p_sut_->is_belong_to( p_allc_mem ) );
}

TEST_F( Offset_Malloc, CanAllocateOverSize )
{
	// Arrange

	// Act
	void* p_allc_mem = p_sut_->allocate( 1024 );

	// Assert
	EXPECT_EQ( p_allc_mem, nullptr );
	EXPECT_FALSE( p_sut_->is_belong_to( p_allc_mem ) );
}

TEST_F( Offset_Malloc, CanDeallocate1 )
{
	// Arrange
	void* p_allc_mem = p_sut_->allocate( 10 );

	// Act
	EXPECT_NO_THROW( p_sut_->deallocate( p_allc_mem ) );

	// Assert
}

TEST_F( Offset_Malloc, CanDeallocate2 )
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

TEST( Offset_Malloc_Highload, CanMulti_Thread_Calling )
{
	// Arrange
	const int        loopcount       = 100000;
	const size_t     alloc_size      = 11;
	constexpr int    test_thread_num = 100;
	constexpr size_t buff_size       = alloc_size * 100 * test_thread_num;

	std::unique_ptr<unsigned char[]> up_buff( new unsigned char[buff_size] );   // 4MBytes
	void*                            p_mem = reinterpret_cast<void*>( up_buff.get() );
	offset_malloc                    sut( p_mem, buff_size );

	std::packaged_task<int( offset_malloc )> fail_count_tasks[test_thread_num];
	std::future<int>                         fail_count_task_results[test_thread_num];
	std::thread                              thread_array[test_thread_num];

	// Act
	for ( int i = 0; i < test_thread_num; i++ ) {
		fail_count_tasks[i]        = std::packaged_task<int( offset_malloc )>( []( offset_malloc ttsut ) -> int {
            int fail_count_ret = 0;
            for ( int i = 0; i < loopcount; i++ ) {
                auto p_ret = ttsut.allocate( 11 );
                if ( p_ret == nullptr ) {
                    fail_count_ret++;
                }
                ttsut.deallocate( p_ret );
            }

            return fail_count_ret;
        } );
		fail_count_task_results[i] = fail_count_tasks[i].get_future();
		thread_array[i]            = std::thread( std::move( fail_count_tasks[i] ), sut );
	}

	// Assert
	int final_fail_count_result = 0;
	for ( auto& e : fail_count_task_results ) {
		final_fail_count_result += e.get();
	}
	EXPECT_EQ( final_fail_count_result, 0 );

	// Clean-up
	for ( auto& e : thread_array ) {
		if ( e.joinable() ) {
			e.join();
		}
	}
}
