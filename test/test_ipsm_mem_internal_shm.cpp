/**
 * @file test_ipsm_mem.cpp
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

#include "ipsm_mem.hpp"
#include "ipsm_mem_internal.hpp"
#include "test_ipsm_common.hpp"

using namespace ipsm;

constexpr const char* const p_shm_name = "/my_test_ipsm_mem_internal_shm";
constexpr size_t            shm_size   = 1024;

TEST( Test_shm_resource_handler, CanDefaultConstruct_CanDestruct )
{
	// Arrange
	shm_unlink( p_shm_name );

	ASSERT_NO_THROW( {
		// Act
		shm_resource_handler sut;

		// Assert
		EXPECT_FALSE( sut.is_valid() );
	} );
}

TEST( Test_shm_resource_handler, CanConstruct_CanDestruct )
{
	// Arrange
	shm_unlink( p_shm_name );

	ASSERT_NO_THROW( {
		// Act
		shm_resource_handler sut( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

		// Assert
		EXPECT_TRUE( sut.is_valid() );
		EXPECT_GE( sut.allocated_size(), shm_size );
	} );
}

TEST( Test_shm_resource_handler, CanMoveConstruct )
{
	// Arrange
	shm_unlink( p_shm_name );
	shm_resource_handler sut_src( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	ASSERT_NO_THROW( {
		// Act
		shm_resource_handler sut = std::move( sut_src );

		// Assert
		EXPECT_TRUE( sut.is_valid() );
		EXPECT_GE( sut.allocated_size(), shm_size );
	} );

	// Assert
	EXPECT_FALSE( sut_src.is_valid() );   // NOLINT(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
}

TEST( Test_shm_resource_handler, CanMoveAssignment )
{
	// Arrange
	shm_unlink( p_shm_name );
	shm_resource_handler sut;

	// Act
	ASSERT_NO_THROW( {
		sut = shm_resource_handler( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	} );

	// Assert
	EXPECT_TRUE( sut.is_valid() );
	EXPECT_GE( sut.allocated_size(), shm_size );
}

TEST( Test_shm_resource_handler, GetNativeHandle )
{
	// Arrange
	shm_unlink( p_shm_name );
	shm_resource_handler sut( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	auto ret = sut.get_shm_pointer();

	// Assert
	EXPECT_NE( ret, nullptr );
}

TEST( Test_shm_resource_handler, Is_Invalid )
{
	// Arrange
	shm_resource_handler sut;

	// Act
	bool ret = sut.is_valid();

	// Assert
	EXPECT_FALSE( ret );
	EXPECT_EQ( sut.get_shm_pointer(), nullptr );
	EXPECT_EQ( sut.allocated_size(), 0 );
}

TEST( Test_shm_resource_handler, Is_Valid )
{
	// Arrange
	shm_unlink( p_shm_name );
	shm_resource_handler sut( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	bool ret = sut.is_valid();

	// Assert
	EXPECT_TRUE( ret );
	EXPECT_NE( sut.get_shm_pointer(), nullptr );
	EXPECT_GE( sut.allocated_size(), shm_size );
}

TEST( Test_shm_resource_handler, CanCallName )
{
	// Arrange
	shm_unlink( p_shm_name );
	shm_resource_handler sut( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	std::string ret = sut.name();

	// Assert
	EXPECT_STREQ( ret.c_str(), p_shm_name );
}

TEST( Test_shm_resource_handler, CanSwap )
{
	// Arrange
	shm_unlink( p_shm_name );
	shm_resource_handler sut1;
	shm_resource_handler sut2( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	EXPECT_FALSE( sut1.is_valid() );
	EXPECT_TRUE( sut2.is_valid() );
	auto p_shm_top  = sut2.get_shm_pointer();
	auto alloc_size = sut2.allocated_size();

	// Act
	sut1.swap( sut2 );

	// Assert
	EXPECT_TRUE( sut1.is_valid() );
	EXPECT_EQ( sut1.get_shm_pointer(), p_shm_top );
	EXPECT_EQ( sut1.allocated_size(), alloc_size );
	EXPECT_STREQ( sut1.name().c_str(), p_shm_name );

	EXPECT_FALSE( sut2.is_valid() );
	EXPECT_EQ( sut2.get_shm_pointer(), nullptr );
	EXPECT_EQ( sut2.allocated_size(), 0 );
	EXPECT_STREQ( sut2.name().c_str(), "" );
}

TEST( Test_shm_resource_handler, CanCallReleaseResource )
{
	// Arrange
	shm_unlink( p_shm_name );
	shm_resource_handler sut( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	EXPECT_TRUE( sut.is_valid() );

	// Act
	sut.release_resource();

	// Assert
	EXPECT_FALSE( sut.is_valid() );
	EXPECT_EQ( sut.get_shm_pointer(), nullptr );
	EXPECT_EQ( sut.allocated_size(), 0 );
	EXPECT_STREQ( sut.name().c_str(), "" );
}

TEST( Test_shm_resource_handler, CanDoUnlink )
{
	// Arrange
	shm_unlink( p_shm_name );
	shm_resource_handler sut( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	shm_resource_handler tmp_shm( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	ASSERT_FALSE( tmp_shm.is_valid() );

	// Act
	sut.do_unlink();

	// Assert
	shm_resource_handler sut2;
	ASSERT_NO_THROW( {
		sut2 = shm_resource_handler( shm_resource_handler::try_create_tag {}, p_shm_name, shm_size, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	} );
	ASSERT_TRUE( sut2.is_valid() );
	EXPECT_NE( sut.get_fd(), sut2.get_fd() );
}
