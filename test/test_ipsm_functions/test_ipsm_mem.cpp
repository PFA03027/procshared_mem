/**
 * @file test_ipsm_mem.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-08
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <cstdlib>
#include <iostream>
#include <string>

#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */

#include "ipsm_mem_internal.hpp"

#include "gtest/gtest.h"

// ==============================================================================

class TestLockFileGuard : public testing::Test {
protected:
	std::string fname_;
	mode_t      mode_;

	void SetUp() override
	{
		fname_ = "/tmp/test_lock_file_guard_" + std::to_string( getpid() );
		mode_  = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	}
	void TearDown() override
	{
		unlink( fname_.c_str() );
	}
};

TEST_F( TestLockFileGuard, CanConstruct )
{
	// Arrange

	// Act & Assert
	EXPECT_NO_THROW( ipsm::lock_file_guard sut( fname_, mode_ ) );

	//
}

TEST_F( TestLockFileGuard, Solo_CanTryExclusiveLock_ThenReturnTrue )
{
	// Arrange
	ipsm::lock_file_guard sut( fname_, mode_ );

	// Act
	bool ret = sut.try_exclusive_lock();

	// Assert
	EXPECT_TRUE( ret );
}

TEST_F( TestLockFileGuard, Solo_CanTrySharedLock_ThenReturnTrue )
{
	// Arrange
	ipsm::lock_file_guard sut( fname_, mode_ );

	// Act
	bool ret = sut.try_shared_lock();

	// Assert
	EXPECT_TRUE( ret );
}

TEST_F( TestLockFileGuard, AlreadyExclusiveLocked_CanTryExclusiveLock_ThenReturnFalse )
{
	// Arrange
	ipsm::lock_file_guard sut1( fname_, mode_ );
	ipsm::lock_file_guard sut2( fname_, mode_ );

	sut1.try_exclusive_lock();

	// Act
	bool ret = sut2.try_exclusive_lock();

	// Assert
	EXPECT_FALSE( ret );
}

TEST_F( TestLockFileGuard, AlreadyExclusiveLocked_CanTrySharedLock_ThenReturnFalse )
{
	// Arrange
	ipsm::lock_file_guard sut1( fname_, mode_ );
	ipsm::lock_file_guard sut2( fname_, mode_ );

	sut1.try_exclusive_lock();

	// Act
	bool ret = sut2.try_shared_lock();

	// Assert
	EXPECT_FALSE( ret );
}

TEST_F( TestLockFileGuard, AlreadySharedLocked_CanTryExclusiveLock_ThenReturnFalse )
{
	// Arrange
	ipsm::lock_file_guard sut1( fname_, mode_ );
	ipsm::lock_file_guard sut2( fname_, mode_ );

	sut1.try_shared_lock();

	// Act
	bool ret = sut2.try_exclusive_lock();

	// Assert
	EXPECT_FALSE( ret );
}

TEST_F( TestLockFileGuard, AlreadySharedLocked_CanTrySharedLock_ThenReturnTrue )
{
	// Arrange
	ipsm::lock_file_guard sut1( fname_, mode_ );
	ipsm::lock_file_guard sut2( fname_, mode_ );

	sut1.try_shared_lock();

	// Act
	bool ret = sut2.try_shared_lock();

	// Assert
	EXPECT_TRUE( ret );
}

// ==============================================================================

class TestSHMeGuard : public testing::Test {
protected:
	std::string fname_;
	mode_t      mode_;
	size_t      length_;

	void SetUp() override
	{
		fname_  = "/test_shm_guard_" + std::to_string( getpid() );
		mode_   = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
		length_ = 1024;
	}
	void TearDown() override
	{
		shm_unlink( fname_.c_str() );
	}
};

TEST_F( TestSHMeGuard, CanConstruct )
{
	// Arrange

	// Act & Assert
	EXPECT_NO_THROW( ipsm::shm_guard sut );

	//
}

TEST_F( TestSHMeGuard, NotExist_CanCreateSharedMemory )
{
	// Arrange
	ipsm::shm_guard sut;

	// Act & Assert
	EXPECT_NO_THROW( sut.create( fname_, length_, mode_ ) );

	// Assert
	EXPECT_NE( sut.get(), nullptr );
	EXPECT_GE( sut.mmap_length(), length_ );
}

TEST_F( TestSHMeGuard, Exist_CanCreateSharedMemory )
{
	// Arrange
	ipsm::shm_guard sut1;
	ipsm::shm_guard sut2;

	sut1.create( fname_, length_, mode_ );

	// Act & Assert
	EXPECT_NO_THROW( sut2.create( fname_, length_, mode_ ) );

	// Assert
	EXPECT_NE( sut2.get(), nullptr );
	EXPECT_GE( sut2.mmap_length(), length_ );
}

TEST_F( TestSHMeGuard, NotExist_CanOpenSharedMemory_ThenReturnFalse )
{
	// Arrange
	ipsm::shm_guard sut;

	// Act
	bool ret = sut.open( fname_, length_, mode_ );

	// Assert
	EXPECT_FALSE( ret );
	EXPECT_EQ( sut.get(), nullptr );
	EXPECT_EQ( sut.mmap_length(), 0 );
}

TEST_F( TestSHMeGuard, Exist_CanOpenSharedMemory_ThenReturnTrue )
{
	// Arrange
	ipsm::shm_guard sut1;
	ipsm::shm_guard sut2;

	sut1.create( fname_, length_, mode_ );

	// Act
	bool ret = sut2.open( fname_, length_, mode_ );

	// Assert
	EXPECT_TRUE( ret );
	EXPECT_NE( sut2.get(), nullptr );
	EXPECT_GE( sut2.mmap_length(), length_ );
}

// ==============================================================================

class TestIPSMem : public testing::Test {
protected:
	std::string shm_name_;
	std::string lifetime_ctrl_fname_;
	mode_t      mode_;
	size_t      length_;

	void SetUp() override
	{
		shm_name_            = "/test_ipsm_mem_impl_" + std::to_string( getpid() );
		lifetime_ctrl_fname_ = "/tmp/test_ipsm_mem_impl_lifetime_ctrl_" + std::to_string( getpid() );
		mode_                = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
		length_              = 1024;
	}
	void TearDown() override
	{
		shm_unlink( shm_name_.c_str() );
		unlink( lifetime_ctrl_fname_.c_str() );
	}
};

TEST_F( TestIPSMem, CanConstruct )
{
	// Arrange

	// Act & Assert
	EXPECT_NO_THROW( ipsm::ipsm_mem sut );

	// Assert
}

TEST_F( TestIPSMem, NotExist_CanSetup )
{
	// Arrange
	ipsm::ipsm_mem sut;

	// Act
	EXPECT_NO_THROW( sut.setup( shm_name_.c_str(), lifetime_ctrl_fname_.c_str(), length_, mode_, []( void* p, size_t s ) -> size_t { return 0; } ) );

	// Assert
	EXPECT_NE( sut.get(), nullptr );
	EXPECT_GE( sut.available_size(), length_ );
	EXPECT_EQ( sut.get_status(), ipsm::ipsm_mem::status::ready );
}

TEST_F( TestIPSMem, Exist_CanSetup )
{
	// Arrange
	ipsm::ipsm_mem sut1;
	ipsm::ipsm_mem sut2;

	sut1.setup( shm_name_.c_str(), lifetime_ctrl_fname_.c_str(), length_, mode_, []( void* p, size_t s ) -> size_t { return 0; } );
	int* p = static_cast<int*>( sut1.get() );
	EXPECT_NE( *p, 12345 );
	*p = 12345;

	// Act
	EXPECT_NO_THROW( sut2.setup( shm_name_.c_str(), lifetime_ctrl_fname_.c_str(), length_, mode_, []( void* p, size_t s ) -> size_t { return 0; } ) );

	// Assert
	EXPECT_NE( sut2.get(), nullptr );
	EXPECT_GE( sut2.available_size(), length_ );
	EXPECT_EQ( sut2.get_status(), ipsm::ipsm_mem::status::ready );
	EXPECT_EQ( *static_cast<int*>( sut2.get() ), 12345 );
}

void TestIPSMem_SetupThen_ProcessAbort(
	const char* p_shm_name,              //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
	const char* p_lifetime_ctrl_fname,   //!< [in] lifetime control file name.
	size_t      length,                  //!< [in] shared memory size
	mode_t      mode                     //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
)
{
	ipsm::ipsm_mem sut1;

	sut1.setup( p_shm_name, p_lifetime_ctrl_fname, length, mode, []( void* p, size_t s ) -> size_t { return 0; } );
	int* p = static_cast<int*>( sut1.get() );
	*p     = 12345;

	std::cerr << "Sending myself unblockable signal" << std::endl;
	std::abort();   // no call destructor, simulate last process abort
}

using TestIPSMemDeathTest = TestIPSMem;

TEST_F( TestIPSMemDeathTest, LastProcessAbortThen_CanSetup_ThenRecreated )
{
	// Arrange
	ASSERT_EXIT( TestIPSMem_SetupThen_ProcessAbort( shm_name_.c_str(), lifetime_ctrl_fname_.c_str(), length_, mode_ ),
	             testing::KilledBySignal( SIGABRT ),
	             "Sending myself unblockable signal" );

	ipsm::ipsm_mem sut2;

	// Act
	EXPECT_NO_THROW( sut2.setup( shm_name_.c_str(), lifetime_ctrl_fname_.c_str(), length_, mode_, []( void* p, size_t s ) -> size_t { return 0; } ) );

	// Assert
	EXPECT_NE( sut2.get(), nullptr );
	EXPECT_GE( sut2.available_size(), length_ );
	EXPECT_EQ( sut2.get_status(), ipsm::ipsm_mem::status::ready );
	EXPECT_NE( *static_cast<int*>( sut2.get() ), 12345 );
}

TEST_F( TestIPSMemDeathTest, AnotherProcessAbortThen_CanDestructSharedMemory )
{
	// Arrange
	{
		ipsm::ipsm_mem sut2;
		EXPECT_NO_THROW( sut2.setup( shm_name_.c_str(), lifetime_ctrl_fname_.c_str(), length_, mode_, []( void* p, size_t s ) -> size_t { return 0; } ) );

		ASSERT_EXIT( TestIPSMem_SetupThen_ProcessAbort( shm_name_.c_str(), lifetime_ctrl_fname_.c_str(), length_, mode_ ),
		             testing::KilledBySignal( SIGABRT ),
		             "Sending myself unblockable signal" );

		// Act
	}

	// Assert
}
