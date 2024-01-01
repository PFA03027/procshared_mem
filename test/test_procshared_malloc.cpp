/**
 * @file test_procshared_malloc.cpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "procshared_malloc.hpp"
#include "test_procshared_common.hpp"

const char* p_shm_obj_name = "/my_test_shm_test_procshared_malloc";

TEST( Test_procshared_malloc, CanDefaultConstruct_CanDestruct )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data

	// Act
	ASSERT_NO_THROW(
		{
			procshared_malloc shm_obj;
		} );

	// Assert
}

TEST( Test_procshared_malloc, CanDefaultConstruct_ThenAllocate )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc sut;

	// Act
	auto p = sut.allocate( 10 );

	// Assert
	EXPECT_EQ( p, nullptr );
}

TEST( Test_procshared_malloc, CanConstruct_CanDestruct )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data

	// Act
	ASSERT_NO_THROW(
		{
			procshared_malloc shm_obj( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
		} );

	// Assert
}

TEST( Test_procshared_malloc, CanConstruct_ThenAllocate )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc sut( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	auto p = sut.allocate( 10 );

	// Assert
	EXPECT_NE( p, nullptr );
}

TEST( Test_procshared_malloc, CanConstruct_ThenAllocateDeallocate )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc sut( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	auto              p = sut.allocate( 10 );
	EXPECT_NE( p, nullptr );

	// Act
	ASSERT_NO_THROW( { sut.deallocate( p ); } );

	// Assert
}

TEST( Test_procshared_malloc, CanMoveConstruct )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc sut;

	// Act
	ASSERT_NO_THROW(
		{
			sut = procshared_malloc( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
		} );

	// Assert
}

TEST( Test_procshared_malloc, CanMoveConstruct_ThenAllocate )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc sut;
	sut = procshared_malloc( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	auto p = sut.allocate( 10 );

	// Assert
	EXPECT_NE( p, nullptr );
}

TEST( Test_procshared_malloc, CanMoveConstruct_ThenAllocateDeallocate )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc sut;
	sut    = procshared_malloc( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	auto p = sut.allocate( 10 );
	EXPECT_NE( p, nullptr );

	// Act
	ASSERT_NO_THROW( { sut.deallocate( p ); } );

	// Assert
}

TEST( Test_procshared_malloc, CanMoveAssignment )
{
	// Arrange
	const char* p_shm_obj_name2 = "/my_test_shm_test_procshared_malloc2";
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );    // to remove ghost data
	procshared_mem::debug_force_cleanup( p_shm_obj_name2, "/tmp" );   // to remove ghost data
	procshared_malloc sut( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	auto              p1 = sut.allocate( 10 );
	EXPECT_NE( p1, nullptr );

	// Act
	sut     = procshared_malloc( p_shm_obj_name2, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	auto p2 = sut.allocate( 10 );

	// Assert
	EXPECT_NE( p2, nullptr );
	EXPECT_NE( p1, p2 );
}

TEST( Test_procshared_malloc, CanMoveAssignment_ThenDeallocate )
{
	// Arrange
	const char* p_shm_obj_name2 = "/my_test_shm_test_procshared_malloc2";
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );    // to remove ghost data
	procshared_mem::debug_force_cleanup( p_shm_obj_name2, "/tmp" );   // to remove ghost data
	procshared_malloc sut( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	auto              p1 = sut.allocate( 10 );
	EXPECT_NE( p1, nullptr );
	sut     = procshared_malloc( p_shm_obj_name2, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	auto p2 = sut.allocate( 10 );
	EXPECT_NE( p2, nullptr );
	EXPECT_NE( p1, p2 );

	// Act
	ASSERT_NO_THROW( { sut.deallocate( p2 ); } );

	// Assert
}

TEST( Test_procshared_malloc, TryOversizeAllocation )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc sut( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	auto p = sut.allocate( 4096 );

	// Assert
	EXPECT_NE( p, nullptr );
}

#if defined( TEST_ENABLE_ADDRESSSANITIZER ) || defined( TEST_ENABLE_LEAKSANITIZER )
#else
TEST( Test_procshared_malloc, CanAllocateBwProcess )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc shm_obj( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t> f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), []() -> int {
		procshared_malloc sut_secondary( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
		if ( sut_secondary.get_bind_count() != 2 ) {
			return 3;
		}
		auto p = sut_secondary.allocate( 10 );
		if ( p == nullptr ) {
			return 2;
		}
		return 1;
	} );

	// Assert
	child_proc_return_t ret = { 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 1 );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

#endif   // TEST_ENABLE_ADDRESSSANITIZER
