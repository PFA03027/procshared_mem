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
#include "test_ipsm_common.hpp"

TEST( Test_ipsm_mutex, CanConstruct_CanDestruct )
{
	ASSERT_NO_THROW( ipsm::ipsm_mutex sut );
}

TEST( Test_ipsm_mutex, CanLock_CanTryLock_CanUnlock )
{
	// Arrange
	ipsm::ipsm_mutex sut;

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
	ipsm::ipsm_mutex sut;

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
	ipsm::ipsm_mutex sut;

	// Act
	auto ret = sut.native_handle();

	// Assert
	EXPECT_NE( ret, nullptr );

	// Cleanup
}

TEST( Test_ipsm_mutex, CanRecoverByRobustnessViaLock )
{
	// Arrange
	ipsm::ipsm_mutex sut;
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
	ipsm::ipsm_mutex sut;
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
	ipsm::ipsm_mutex sut;
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
	ASSERT_NO_THROW( ipsm::ipsm_recursive_mutex sut );
}

TEST( Test_ipsm_recursive_mutex, CanLock_CanTryLock_CanUnlock )
{
	// Arrange
	ipsm::ipsm_recursive_mutex sut;

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
	ipsm::ipsm_recursive_mutex sut;

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
	ipsm::ipsm_recursive_mutex sut;
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
	ipsm::ipsm_recursive_mutex sut;
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
	ipsm::ipsm_recursive_mutex sut;

	// Act
	auto ret = sut.native_handle();

	// Assert
	EXPECT_NE( ret, nullptr );

	// Cleanup
}

TEST( Test_ipsm_recursive_mutex, CanRecoverByRobustnessViaLock )
{
	// Arrange
	ipsm::ipsm_recursive_mutex sut;
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
	ipsm::ipsm_recursive_mutex sut;
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
#define SHM_OBJ_NAME_STRING               "/my_test_shm_test_ipsm_mutex"
#define SHM_OBJ_NAME_LIFETIME_CTRL_STRING "/tmp/my_test_shm_test_ipsm_mutex.lifetime_ctrl"

TEST( Test_ipsm_mutex_bw_proc, CanLock_CanTryLock_CanUnlock )
{
	// Arrange
	auto init_functor = []( void* p_mem, off_t len ) -> std::uintptr_t {
		if ( p_mem == nullptr ) {
			return ~( (std::uintptr_t)0 );
		}
		if ( len < 4096 ) {
			return ~( (std::uintptr_t)0 );
		}
		[[maybe_unused]] ipsm::ipsm_mutex* p_ps_mtx = new ( p_mem ) ipsm::ipsm_mutex();
		return 0;
	};
	ipsm::ipsm_mem    shm_obj( SHM_OBJ_NAME_STRING, SHM_OBJ_NAME_LIFETIME_CTRL_STRING, 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, init_functor );
	ipsm::ipsm_mutex* p_ps_mtx = reinterpret_cast<ipsm::ipsm_mutex*>( shm_obj.get() );
	p_ps_mtx->lock();

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t>                                f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), [init_functor]() -> int {
		ipsm::ipsm_mem          shm_obj_secondary( SHM_OBJ_NAME_STRING, SHM_OBJ_NAME_LIFETIME_CTRL_STRING, 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, init_functor );
		ipsm::ipsm_mutex* p_ps_mtx = reinterpret_cast<ipsm::ipsm_mutex*>( shm_obj_secondary.get() );

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
	auto init_functor = []( void* p_mem, off_t len ) -> std::uintptr_t {
		if ( p_mem == nullptr ) {
			return ~( (std::uintptr_t)0 );
		}
		if ( len < 4096 ) {
			return ~( (std::uintptr_t)0 );
		}
		[[maybe_unused]] ipsm::ipsm_recursive_mutex* p_ps_mtx = new ( p_mem ) ipsm::ipsm_recursive_mutex();
		return 0;
	};
	ipsm::ipsm_mem              shm_obj( SHM_OBJ_NAME_STRING, SHM_OBJ_NAME_LIFETIME_CTRL_STRING, 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, init_functor );
	ipsm::ipsm_recursive_mutex* p_ps_mtx = reinterpret_cast<ipsm::ipsm_recursive_mutex*>( shm_obj.get() );
	p_ps_mtx->lock();

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t>                                f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), [init_functor]() -> int {
		ipsm::ipsm_mem              shm_obj_secondary( SHM_OBJ_NAME_STRING, SHM_OBJ_NAME_LIFETIME_CTRL_STRING, 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, init_functor );
		ipsm::ipsm_recursive_mutex* p_ps_mtx = reinterpret_cast<ipsm::ipsm_recursive_mutex*>( shm_obj_secondary.get() );

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
