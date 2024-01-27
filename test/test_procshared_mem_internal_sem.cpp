/**
 * @file test_procshared_mem.cpp
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

#include "misc_utility.hpp"
#include "procshared_mem.hpp"
#include "procshared_mem_internal.hpp"
#include "semaphore_mutex.hpp"
#include "test_procshared_common.hpp"

constexpr const char* const p_shm_name = "/my_test_procshared_mem_internal_sem";

TEST( Test_semaphore_resource_handler, CanDefaultConstruct_CanDestruct )
{
	// Arrange
	sem_unlink( p_shm_name );

	ASSERT_NO_THROW( {
		// Act
		semaphore_mutex sut;

		// Assert
		EXPECT_FALSE( sut.is_valid() );
	} );
}

TEST( Test_semaphore_resource_handler, CanConstruct_CanDestruct )
{
	// Arrange
	sem_unlink( p_shm_name );

	ASSERT_NO_THROW( {
		// Act
		semaphore_mutex sut( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

		// Assert
		EXPECT_TRUE( sut.is_valid() );
	} );
}

TEST( Test_semaphore_resource_handler, CanMoveConstruct )
{
	// Arrange
	sem_unlink( p_shm_name );
	semaphore_mutex sut_src( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	ASSERT_NO_THROW( {
		// Act
		semaphore_mutex sut = std::move( sut_src );

		// Assert
		EXPECT_TRUE( sut.is_valid() );
	} );

	// Assert
	EXPECT_FALSE( sut_src.is_valid() );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
}

TEST( Test_semaphore_resource_handler, CanMoveAssignment )
{
	// Arrange
	sem_unlink( p_shm_name );
	semaphore_mutex sut;

	// Act
	ASSERT_NO_THROW( {
		sut = semaphore_mutex( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	} );

	// Assert
	EXPECT_TRUE( sut.is_valid() );
}

TEST( Test_semaphore_resource_handler, GetNativeHandle )
{
	// Arrange
	sem_unlink( p_shm_name );
	semaphore_mutex sut( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	auto ret = sut.native_handle();

	// Assert
	EXPECT_NE( ret, nullptr );
}

TEST( Test_semaphore_resource_handler, Is_Invalid )
{
	// Arrange
	semaphore_mutex sut;

	// Act
	bool ret = sut.is_valid();

	// Assert
	EXPECT_FALSE( ret );
}

TEST( Test_semaphore_resource_handler, Is_Valid )
{
	// Arrange
	sem_unlink( p_shm_name );
	semaphore_mutex sut( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	bool ret = sut.is_valid();

	// Assert
	EXPECT_TRUE( ret );
}

TEST( Test_semaphore_resource_handler, CanCallName )
{
	// Arrange
	sem_unlink( p_shm_name );
	semaphore_mutex sut( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	std::string ret = sut.name();

	// Assert
	EXPECT_STREQ( ret.c_str(), p_shm_name );
}

TEST( Test_semaphore_resource_handler, CanSwap )
{
	// Arrange
	sem_unlink( p_shm_name );
	semaphore_mutex sut1;
	semaphore_mutex sut2( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	EXPECT_FALSE( sut1.is_valid() );
	EXPECT_TRUE( sut2.is_valid() );
	auto nv_handle = sut2.native_handle();

	// Act
	sut1.swap( sut2 );

	// Assert
	EXPECT_TRUE( sut1.is_valid() );
	EXPECT_EQ( sut1.native_handle(), nv_handle );
	EXPECT_STREQ( sut1.name().c_str(), p_shm_name );

	EXPECT_FALSE( sut2.is_valid() );
	EXPECT_EQ( sut2.native_handle(), SEM_FAILED );
	EXPECT_STREQ( sut2.name().c_str(), "" );
}

#if 0
TEST( Test_semaphore_resource_handler, CanCallReleaseResource )
{
	// Arrange
	sem_unlink( p_shm_name );
	semaphore_mutex sut( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	EXPECT_TRUE( sut.is_valid() );

	// Act
	sut.release_resource();

	// Assert
	EXPECT_FALSE( sut.is_valid() );
	EXPECT_EQ( sut.native_handle(), SEM_FAILED );
	EXPECT_STREQ( sut.name().c_str(), "" );
}
#endif

TEST( Test_semaphore_resource_handler, CanDoUnlink )
{
	// Arrange
	sem_unlink( p_shm_name );
	semaphore_mutex sut( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	semaphore_mutex tmp_sem( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	ASSERT_FALSE( tmp_sem.is_valid() );

	// Act
	sut.do_unlink();

	// Assert
	semaphore_mutex sut2;
	ASSERT_NO_THROW( {
		sut2 = semaphore_mutex( p_shm_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	} );
	ASSERT_TRUE( sut2.is_valid() );

	semaphore_post_guard guard1( sut, semaphore_post_guard_defer_acquire );
	semaphore_post_guard guard2( sut2, semaphore_post_guard_defer_acquire );

	guard1.post();
	bool ret = guard2.try_acquire();   // sutとsut2は、異なるsemaphoreオブジェクトなので、post()の情報がsut2に伝わらないことを確かめる。
	EXPECT_FALSE( ret );
}
