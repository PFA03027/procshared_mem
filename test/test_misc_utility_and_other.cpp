/**
 * @file test_misc_utility_and_other.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2024-01-27
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include "gtest/gtest.h"

#include "lockfile_mutex.hpp"
#include "test_ipsm_common.hpp"

using namespace ipsm;

const char* p_lockfile_fname = "/tmp/my_test_misc_utility_and_other";

TEST( Test_lockfile_mutex, CanConstruct_CanDestruct )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data

	// Act
	{
		ASSERT_NO_THROW( lockfile_mutex sut( p_lockfile_fname ); );

		// Assert
		int ret = access( p_lockfile_fname, F_OK );
		EXPECT_NE( ret, 0 );
	}
	int ret = access( p_lockfile_fname, F_OK );
	EXPECT_NE( ret, 0 );
}

TEST( Test_lockfile_mutex, CanLock )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut( p_lockfile_fname );

	// Act
	sut.lock();

	// Assert
	int  ret       = access( p_lockfile_fname, F_OK );
	auto cur_errno = errno;
	EXPECT_EQ( ret, 0 ) << cur_errno << std::endl;
}

TEST( Test_lockfile_mutex, CanLock_CanDestruct )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	{
		lockfile_mutex sut( p_lockfile_fname );
		sut.lock();
		int  ret       = access( p_lockfile_fname, F_OK );
		auto cur_errno = errno;
		EXPECT_EQ( ret, 0 ) << cur_errno << std::endl;

		// Act
		// Destruct
	}

	// Assert
	int ret = access( p_lockfile_fname, F_OK );
	EXPECT_NE( ret, 0 );
}

TEST( Test_lockfile_mutex, CanUnlock )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut( p_lockfile_fname );
	sut.lock();

	// Act
	sut.unlock();

	// Assert
	int ret = access( p_lockfile_fname, F_OK );
	EXPECT_NE( ret, 0 );
}

TEST( Test_lockfile_mutex, CanTrylock1 )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut( p_lockfile_fname );

	// Act
	bool ret = sut.try_lock();

	// Assert
	EXPECT_TRUE( ret );
	int  ret2      = access( p_lockfile_fname, F_OK );
	auto cur_errno = errno;
	EXPECT_EQ( ret2, 0 ) << cur_errno << std::endl;
}

TEST( Test_lockfile_mutex, CanTrylock2 )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut( p_lockfile_fname );
	sut.lock();

	// Act
	bool ret = sut.try_lock();

	// Assert
	EXPECT_FALSE( ret );
	int  ret2      = access( p_lockfile_fname, F_OK );
	auto cur_errno = errno;
	EXPECT_EQ( ret2, 0 ) << cur_errno << std::endl;
}

TEST( Test_lockfile_mutex, CanTrylock3 )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut1( p_lockfile_fname );
	lockfile_mutex sut2( p_lockfile_fname );
	sut1.lock();

	// Act
	bool ret = sut2.try_lock();

	// Assert
	EXPECT_FALSE( ret );
	int  ret2      = access( p_lockfile_fname, F_OK );
	auto cur_errno = errno;
	EXPECT_EQ( ret2, 0 ) << cur_errno << std::endl;
}

TEST( Test_lockfile_mutex, CanTrylock4 )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut1( p_lockfile_fname );
	sut1.lock();

	// Act
	{
		lockfile_mutex sut2( p_lockfile_fname );
		sut2.try_lock();
	}

	// Assert
	int  ret2      = access( p_lockfile_fname, F_OK );
	auto cur_errno = errno;
	EXPECT_EQ( ret2, 0 ) << cur_errno << std::endl;
}

TEST( Test_lockfile_mutex, CanMove_ThenCanLock )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut_src( p_lockfile_fname );
	lockfile_mutex sut( std::move( sut_src ) );

	// Act
	bool ret = sut.try_lock();

	// Assert
	EXPECT_TRUE( ret );
	int  ret2      = access( p_lockfile_fname, F_OK );
	auto cur_errno = errno;
	EXPECT_EQ( ret2, 0 ) << cur_errno << std::endl;
}

TEST( Test_lockfile_mutex, CanMove_ThenCanLock2 )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut_src( p_lockfile_fname );
	bool           ret;
	{
		lockfile_mutex sut( std::move( sut_src ) );

		// Act
		ret = sut.try_lock();
	}

	// Assert
	EXPECT_TRUE( ret );
	int ret2 = access( p_lockfile_fname, F_OK );
	EXPECT_NE( ret2, 0 );
}

TEST( Test_lockfile_mutex, CanMove_ThenCanLock3 )
{
	// Arrange
	lockfile_mutex::debug_force_cleanup( p_lockfile_fname );   // to remove ghost data
	lockfile_mutex sut( p_lockfile_fname );
	bool           ret;
	{
		lockfile_mutex sut_src( p_lockfile_fname );
		ret = sut_src.try_lock();
		EXPECT_TRUE( ret );
		int  ret2      = access( p_lockfile_fname, F_OK );
		auto cur_errno = errno;
		EXPECT_EQ( ret2, 0 ) << cur_errno << std::endl;

		// Act
		sut = std::move( sut_src );
	}

	// Assert
	int  ret2      = access( p_lockfile_fname, F_OK );
	auto cur_errno = errno;
	EXPECT_EQ( ret2, 0 ) << cur_errno << std::endl;
}
