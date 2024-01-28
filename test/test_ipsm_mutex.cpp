/**
 * @file test_ipsm_mutex.cpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <future>
#include <thread>

#include "gtest/gtest.h"

#include "ipsm_mem.hpp"
#include "ipsm_mutex.hpp"
#include "ipsm_recursive_mutex.hpp"
#include "test_ipsm_common.hpp"

using namespace ipsm;

TEST( Test_ipsm_mutex, CanConstruct_CanDestruct )
{
	ASSERT_NO_THROW( ipsm_mutex sut );
}

TEST( Test_ipsm_mutex, CanLock_CanTryLock_CanUnlock )
{
	// Arrange
	ipsm_mutex sut;

	// Act
	sut.lock();

	// Assert
	EXPECT_FALSE( sut.try_lock() );

	// Cleanup
	sut.unlock();
}

TEST( Test_ipsm_mutex, CanTryLock_CanTryLock_CanUnlock )
{
	// Arrange
	ipsm_mutex sut;

	// Act
	bool ret = sut.try_lock();

	// Assert
	EXPECT_TRUE( ret );
	EXPECT_FALSE( sut.try_lock() );

	// Cleanup
	sut.unlock();
}

TEST( Test_ipsm_mutex, CanNativeHandle )
{
	// Arrange
	ipsm_mutex sut;

	// Act
	auto ret = sut.native_handle();

	// Assert
	EXPECT_NE( ret, nullptr );

	// Cleanup
}

TEST( Test_ipsm_mutex, CanRecoverByRobustnessViaLock )
{
	// Arrange
	ipsm_mutex sut;
	std::thread      lock_owner_terminating( [&sut]( void ) {
        sut.lock();
    } );
	lock_owner_terminating.join();

	// Act
	EXPECT_NO_THROW( sut.lock() );

	// Assert

	// Cleanup
	sut.unlock();
}

TEST( Test_ipsm_mutex, CanRecoverByRobustnessViaTryLock )
{
	// Arrange
	ipsm_mutex sut;
	std::thread      lock_owner_terminating( [&sut]( void ) {
        sut.lock();
    } );
	lock_owner_terminating.join();
	bool ret = false;

	// Act
	EXPECT_NO_THROW( ret = sut.try_lock() );

	// Assert
	EXPECT_TRUE( ret );

	// Cleanup
	sut.unlock();
}

#ifdef ENABLE_PTHREAD_MUTEX_ERRORTYPE
TEST( Test_ipsm_mutex, CanDetectDeadLock )
{
	// Arrange
	ipsm_mutex sut;
	sut.lock();

	// Act
	EXPECT_THROW( sut.lock(), std::system_error );

	// Assert

	// Cleanup
	sut.unlock();
}
#else
#endif

TEST( Test_ipsm_recursive_mutex, CanConstruct_CanDestruct )
{
	ASSERT_NO_THROW( ipsm_recursive_mutex sut );
}

TEST( Test_ipsm_recursive_mutex, CanLock_CanTryLock_CanUnlock )
{
	// Arrange
	ipsm_recursive_mutex sut;

	// Act
	sut.lock();

	// Assert
	EXPECT_TRUE( sut.try_lock() );   // because same thread calls

	// Cleanup
	sut.unlock();
	sut.unlock();
}

TEST( Test_ipsm_recursive_mutex, CanTryLock_CanTryLock_CanUnlock )
{
	// Arrange
	ipsm_recursive_mutex sut;

	// Act
	bool ret = sut.try_lock();

	// Assert
	EXPECT_TRUE( ret );
	EXPECT_TRUE( sut.try_lock() );   // because same thread calls

	// Cleanup
	sut.unlock();
	sut.unlock();
}

TEST( Test_ipsm_recursive_mutex, CanLock_OtherThreadCanTryLock_CanUnlock )
{
	// Arrange
	ipsm_recursive_mutex sut;
	sut.lock();
	std::packaged_task<bool()> task( [&sut]() { return sut.try_lock(); } );   // 非同期実行する関数を登録する
	std::future<bool>          f = task.get_future();

	// Act
	std::thread t( std::move( task ) );

	// Assert
	EXPECT_FALSE( f.get() );   // because same thread calls

	// Cleanup
	if ( t.joinable() ) {
		t.join();
	}
	sut.unlock();
}

TEST( Test_ipsm_recursive_mutex, CanTryLock_OtherThreadCanTryLock_CanUnlock )
{
	// Arrange
	ipsm_recursive_mutex sut;
	sut.try_lock();
	std::packaged_task<bool()> task( [&sut]() { return sut.try_lock(); } );   // 非同期実行する関数を登録する
	std::future<bool>          f = task.get_future();

	// Act
	std::thread t( std::move( task ) );

	// Assert
	EXPECT_FALSE( f.get() );   // because same thread calls

	// Cleanup
	if ( t.joinable() ) {
		t.join();
	}
	sut.unlock();
}

TEST( Test_ipsm_recursive_mutex, CanNativeHandle )
{
	// Arrange
	ipsm_recursive_mutex sut;

	// Act
	auto ret = sut.native_handle();

	// Assert
	EXPECT_NE( ret, nullptr );

	// Cleanup
}

TEST( Test_ipsm_recursive_mutex, CanRecoverByRobustnessViaLock )
{
	// Arrange
	ipsm_recursive_mutex sut;
	std::thread                lock_owner_terminating( [&sut]( void ) {
        sut.lock();
    } );
	lock_owner_terminating.join();

	// Act
	EXPECT_NO_THROW( sut.lock() );

	// Assert

	// Cleanup
	sut.unlock();
}

TEST( Test_ipsm_recursive_mutex, CanRecoverByRobustnessViaTryLock )
{
	// Arrange
	ipsm_recursive_mutex sut;
	std::thread                lock_owner_terminating( [&sut]( void ) {
        sut.lock();
    } );
	lock_owner_terminating.join();
	bool ret = false;

	// Act
	EXPECT_NO_THROW( ret = sut.try_lock() );

	// Assert
	EXPECT_TRUE( ret );

	// Cleanup
	sut.unlock();
}

#if defined( TEST_ENABLE_ADDRESSSANITIZER ) || defined( TEST_ENABLE_LEAKSANITIZER )
#else
//===========================================
const char* p_shm_obj_name = "/my_test_shm_test_ipsm_mutex";

TEST( Test_ipsm_mutex_bw_proc, CanLock_CanTryLock_CanUnlock )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	ipsm_mem shm_obj(
		p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		[]( void* p_mem, off_t len ) -> void* {
			if ( p_mem == nullptr ) {
				return nullptr;
			}
			if ( len < 4096 ) {
				return nullptr;
			}
			[[maybe_unused]] ipsm_mutex* p_ps_mtx = new ( p_mem ) ipsm_mutex();
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );
	ipsm_mutex* p_ps_mtx = reinterpret_cast<ipsm_mutex*>( shm_obj.get() );
	p_ps_mtx->lock();

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t>                                f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), []() -> int {
		ipsm_mem shm_obj_secondary;
		shm_obj_secondary.allocate_shm_as_secondary(
			p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			[]( void*, size_t ) { /* 何もしない */ } );
		if ( not shm_obj_secondary.debug_test_integrity() ) {
			return 1;
		}
		ipsm_mutex* p_ps_mtx = reinterpret_cast<ipsm_mutex*>( shm_obj_secondary.get() );

		return ( p_ps_mtx->try_lock() ) ? 2 : 3;
	} );

	// Assert
	child_proc_return_t ret = { 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 3 ) << ( ret.is_exit_normaly_ ? "true " : "false " ) << ret.signal_code_ << std::endl;   // try_lock()に失敗することを確認する。

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}

	p_ps_mtx->unlock();
}

TEST( Test_ipsm_recursive_mutex_bw_proc, CanLock_CanTryLock_CanUnlock )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	ipsm_mem shm_obj(
		p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		[]( void* p_mem, off_t len ) -> void* {
			if ( p_mem == nullptr ) {
				return nullptr;
			}
			if ( len < 4096 ) {
				return nullptr;
			}
			[[maybe_unused]] ipsm_recursive_mutex* p_ps_mtx = new ( p_mem ) ipsm_recursive_mutex();
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );
	ipsm_recursive_mutex* p_ps_mtx = reinterpret_cast<ipsm_recursive_mutex*>( shm_obj.get() );
	p_ps_mtx->lock();

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t>                                f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), []() -> int {
		ipsm_mem shm_obj_secondary;
		shm_obj_secondary.allocate_shm_as_secondary(
			p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			[]( void*, size_t ) { /* 何もしない */ } );
		if ( not shm_obj_secondary.debug_test_integrity() ) {
			return 1;
		}
		ipsm_recursive_mutex* p_ps_mtx = reinterpret_cast<ipsm_recursive_mutex*>( shm_obj_secondary.get() );

		return ( p_ps_mtx->try_lock() ) ? 2 : 3;
	} );

	// Assert

	// Assert
	child_proc_return_t ret = { 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 3 ) << ( ret.is_exit_normaly_ ? "true " : "false " ) << ret.signal_code_ << std::endl;   // try_lock()に失敗することを確認する。

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}

	p_ps_mtx->unlock();
}
#endif   // TEST_ENABLE_ADDRESSSANITIZER
