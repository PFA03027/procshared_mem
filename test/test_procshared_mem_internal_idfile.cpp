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

#include "procshared_mem.hpp"
#include "procshared_mem_internal.hpp"
#include "test_procshared_common.hpp"

constexpr const char* const p_id_fname = "/tmp/my_test_procshared_mem_internal_id";

TEST( Test_id_file_resource_handler, CanDefaultConstruct_CanDestruct )
{
	// Arrange
	unlink( p_id_fname );

	// Act
	ASSERT_NO_THROW( {
		id_file_resource_handler sut;
	} );

	// Assert
}

TEST( Test_id_file_resource_handler, CanConstruct_CanDestruct )
{
	// Arrange
	unlink( p_id_fname );

	// Act
	ASSERT_NO_THROW( {
		id_file_resource_handler sut( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	} );

	// Assert
}

TEST( Test_id_file_resource_handler, CanMoveConstruct )
{
	// Arrange
	unlink( p_id_fname );
	int fd = open( p_id_fname, O_RDWR | O_CLOEXEC );
	close( fd );
	ASSERT_LT( fd, 0 );
	id_file_resource_handler sut_src( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	ASSERT_NO_THROW( {
		id_file_resource_handler sut = std::move( sut_src );
	} );

	// Assert
	fd = open( p_id_fname, O_CLOEXEC );
	close( fd );
	EXPECT_GE( fd, 0 );
}

TEST( Test_id_file_resource_handler, CanMoveAssignment )
{
	// Arrange
	unlink( p_id_fname );
	int fd = open( p_id_fname, O_RDWR | O_CLOEXEC );
	close( fd );
	ASSERT_LT( fd, 0 );
	id_file_resource_handler sut;

	// Act
	ASSERT_NO_THROW( {
		sut = id_file_resource_handler( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	} );

	// Assert
	fd = open( p_id_fname, O_CLOEXEC );
	close( fd );
	EXPECT_GE( fd, 0 );
}

TEST( Test_id_file_resource_handler, IsValid_inode_number )
{
	// Arrange
	unlink( p_id_fname );
	int fd = open( p_id_fname, O_RDWR | O_CLOEXEC );
	close( fd );
	EXPECT_LT( fd, 0 );
	id_file_resource_handler sut_src( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	ino_t sut = sut_src.get_inode_number();

	// Assert
	EXPECT_NE( sut, 0 );
}

TEST( Test_id_file_resource_handler, Is_Same_inode_number )
{
	// Arrange
	unlink( p_id_fname );
	id_file_resource_handler sut_src1( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	id_file_resource_handler sut_src2( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	ino_t sut1 = sut_src1.get_inode_number();
	ino_t sut2 = sut_src2.get_inode_number();

	// Assert
	EXPECT_EQ( sut1, sut2 );
}

TEST( Test_id_file_resource_handler, Is_Invalid )
{
	// Arrange
	id_file_resource_handler sut;

	// Act
	bool ret = sut.is_valid();

	// Assert
	EXPECT_FALSE( ret );
}

TEST( Test_id_file_resource_handler, Is_Valid )
{
	// Arrange
	unlink( p_id_fname );
	id_file_resource_handler sut( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	bool ret = sut.is_valid();

	// Assert
	EXPECT_TRUE( ret );
}

TEST( Test_id_file_resource_handler, CanCallName )
{
	// Arrange
	unlink( p_id_fname );
	id_file_resource_handler sut( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Act
	std::string ret = sut.name();

	// Assert
	EXPECT_STREQ( ret.c_str(), p_id_fname );
}

TEST( Test_id_file_resource_handler, CanSwap )
{
	// Arrange
	unlink( p_id_fname );
	id_file_resource_handler sut1;
	id_file_resource_handler sut2( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	EXPECT_FALSE( sut1.is_valid() );
	EXPECT_TRUE( sut2.is_valid() );
	ino_t valid_inode_num = sut2.get_inode_number();

	// Act
	sut1.swap( sut2 );

	// Assert
	EXPECT_TRUE( sut1.is_valid() );
	EXPECT_EQ( sut1.get_inode_number(), valid_inode_num );
	EXPECT_STREQ( sut1.name().c_str(), p_id_fname );

	EXPECT_FALSE( sut2.is_valid() );
	EXPECT_EQ( sut2.get_inode_number(), 0 );
	EXPECT_STREQ( sut2.name().c_str(), "" );
}

TEST( Test_id_file_resource_handler, CanCallReleaseResource )
{
	// Arrange
	unlink( p_id_fname );
	id_file_resource_handler sut1( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	EXPECT_TRUE( sut1.is_valid() );

	// Act
	sut1.release_resource();

	// Assert
	EXPECT_FALSE( sut1.is_valid() );
	EXPECT_EQ( sut1.get_inode_number(), 0 );
	EXPECT_STREQ( sut1.name().c_str(), "" );
}

TEST( Test_id_file_resource_handler, CanDoUnlink )
{
	// Arrange
	unlink( p_id_fname );
	id_file_resource_handler sut( p_id_fname, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	int                      fd = open( p_id_fname, O_RDWR | O_CLOEXEC );
	close( fd );
	ASSERT_GE( fd, 0 );

	// Act
	sut.do_unlink();

	// Assert
	fd = open( p_id_fname, O_RDWR | O_CLOEXEC );
	close( fd );
	ASSERT_LT( fd, 0 );
}
