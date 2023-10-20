/**
 * @file test_procshared_mutex.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include <future>
#include <mutex>
#include <thread>

#include "gtest/gtest.h"

#include "procshared_condition_variable.hpp"
#include "procshared_mem.hpp"
#include "procshared_mutex.hpp"
#include "procshared_recursive_mutex.hpp"
#include "test_procshared_common.hpp"

TEST( Test_procshared_condition_variable, CanConstruct_CanDestruct )
{
	ASSERT_NO_THROW( procshared_condition_variable sut );
}

TEST( Test_procshared_condition_variable, CanWait_CanNotifyAll )
{
	// Arrange
	bool                          shared_state_flag = false;
	procshared_mutex              mtx;
	procshared_condition_variable sut1;
	procshared_condition_variable sut2;

	std::packaged_task<bool()> task1( [&sut1, &mtx, &shared_state_flag]() {
		std::unique_lock<procshared_mutex> lk( mtx );
		sut1.wait( lk, [&shared_state_flag]() { return shared_state_flag; } );
		return true;
	} );   // 非同期実行する関数を登録する
	std::future<bool>          f1 = task1.get_future();
	std::packaged_task<bool()> task2( [&sut2, &mtx, &shared_state_flag]() {
		std::unique_lock<procshared_mutex> lk( mtx );
		sut2.wait( lk, [&shared_state_flag]() { return shared_state_flag; } );
		return true;
	} );   // 非同期実行する関数を登録する
	std::future<bool>          f2 = task2.get_future();

	std::thread t1( std::move( task1 ) );
	std::thread t2( std::move( task2 ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

	// Act
	{
		std::unique_lock<procshared_mutex> lk( mtx );
		shared_state_flag = true;
		sut1.notify_all();
		sut2.notify_all();
	}

	// Assert
	bool ret = false;
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret );
	ASSERT_NO_THROW( ret = f2.get() );
	EXPECT_TRUE( ret );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
	if ( t2.joinable() ) {
		t2.join();
	}
}

TEST( Test_procshared_condition_variable, CanWaitFor_Timeout )
{
	// Arrange
	bool                          shared_state_flag = false;
	procshared_mutex              mtx;
	procshared_condition_variable sut1;

	std::packaged_task<std::cv_status()> task1( [&sut1, &mtx, &shared_state_flag]() {
		std::unique_lock<procshared_mutex> lk( mtx );
		return sut1.wait_for( lk, std::chrono::milliseconds( 10 ) );
	} );   // 非同期実行する関数を登録する
	std::future<std::cv_status>          f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
	// {
	// 	std::unique_lock<procshared_mutex> lk( mtx );
	// 	shared_state_flag = true;
	// 	sut1.notify_all();
	// 	sut2.notify_all();
	// }

	// Assert
	std::cv_status ret = std::cv_status::no_timeout;
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_EQ( ret, std::cv_status::timeout );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

TEST( Test_procshared_condition_variable, CanWaitFor_NoTimeout )
{
	// Arrange
	bool                          shared_state_flag = false;
	procshared_mutex              mtx;
	procshared_condition_variable sut1;

	std::packaged_task<std::cv_status()> task1( [&sut1, &mtx, &shared_state_flag]() {
		std::unique_lock<procshared_mutex> lk( mtx );
		return sut1.wait_for( lk, std::chrono::milliseconds( 20 ) );
	} );   // 非同期実行する関数を登録する
	std::future<std::cv_status>          f1 = task1.get_future();
	std::thread                          t1( std::move( task1 ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

	// Act
	{
		std::unique_lock<procshared_mutex> lk( mtx );
		shared_state_flag = true;
		sut1.notify_all();
	}

	// Assert
	std::cv_status ret = std::cv_status::timeout;
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_EQ( ret, std::cv_status::no_timeout );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

TEST( Test_procshared_condition_variable, CanWaitForPred_Timeout )
{
	// Arrange
	bool                          shared_state_flag = false;
	procshared_mutex              mtx;
	procshared_condition_variable sut1;

	std::packaged_task<bool()> task1( [&sut1, &mtx, &shared_state_flag]() {
		std::unique_lock<procshared_mutex> lk( mtx );
		return sut1.wait_for(
			lk,
			std::chrono::milliseconds( 10 ),
			[&shared_state_flag]() { return shared_state_flag; } );
	} );   // 非同期実行する関数を登録する
	std::future<bool>          f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );

	// Assert
	bool ret = true;
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_FALSE( ret );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

TEST( Test_procshared_condition_variable, CanWaitForPred_NoTimeout )
{
	// Arrange
	bool                          shared_state_flag = false;
	procshared_mutex              mtx;
	procshared_condition_variable sut1;

	std::packaged_task<bool()> task1( [&sut1, &mtx, &shared_state_flag]() {
		std::unique_lock<procshared_mutex> lk( mtx );
		return sut1.wait_for(
			lk,
			std::chrono::milliseconds( 20 ),
			[&shared_state_flag]() { return shared_state_flag; } );
	} );   // 非同期実行する関数を登録する
	std::future<bool>          f1 = task1.get_future();
	std::thread                t1( std::move( task1 ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

	// Act
	{
		std::unique_lock<procshared_mutex> lk( mtx );
		shared_state_flag = true;
		sut1.notify_one();
	}

	// Assert
	bool ret = false;
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

#if defined( TEST_ENABLE_ADDRESSSANITIZER ) || defined( TEST_ENABLE_LEAKSANITIZER )
#else
//===========================================
const char* p_shm_obj_name = "/my_test_shm_test_procshared_condition_variable";

struct test_shared_data {
	bool shared_state_flag_;
	procshared_mutex mtx_;
	procshared_condition_variable cond_;

	test_shared_data( void )
	  : shared_state_flag_( false )
	  , mtx_()
	  , cond_()
	{
	}
};

TEST( Test_procshared_condition_variable_bw_proc, CanWaitForPred_Timeout )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name );   // to remove ghost data
	procshared_mem shm_obj( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, []( void* p_mem, off_t len ) {
		if ( p_mem == nullptr ) {
			return;
		}
		if ( len < 4096 ) {
			return;
		}
		[[maybe_unused]] test_shared_data* p_sut = new ( p_mem ) test_shared_data();
	} );
	[[maybe_unused]] test_shared_data* p_sut = reinterpret_cast<test_shared_data*>( shm_obj.get() );

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t> f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), []() -> int {
		procshared_mem shm_obj_secondary( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, procshared_mem_defer );
		shm_obj_secondary.allocate_shm_as_secondary();
		if ( not shm_obj_secondary.debug_test_integrity() ) {
			return 1;
		}
		test_shared_data* p_sut = reinterpret_cast<test_shared_data*>( shm_obj_secondary.get() );

		std::unique_lock<procshared_mutex> lk( p_sut->mtx_ );
		bool ret = p_sut->cond_.wait_for(
			lk,
			std::chrono::milliseconds( 10 ),
			[p_sut]() { return p_sut->shared_state_flag_; } );
		return ( ret ) ? 2 : 3;
	} );
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );

	// Assert
	child_proc_return_t ret { false, 0, 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 3 );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

TEST( Test_procshared_condition_variable_bw_proc, CanWaitForPred_NoTimeout )
{
	// Arrange
	procshared_mem::debug_force_cleanup( p_shm_obj_name );   // to remove ghost data
	procshared_mem shm_obj( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, []( void* p_mem, off_t len ) {
		if ( p_mem == nullptr ) {
			return;
		}
		if ( len < 4096 ) {
			return;
		}
		[[maybe_unused]] test_shared_data* p_sut = new ( p_mem ) test_shared_data();
	} );
	test_shared_data* p_sut = reinterpret_cast<test_shared_data*>( shm_obj.get() );

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t> f1 = task1.get_future();

	std::thread t1( std::move( task1 ), []() -> int {
		procshared_mem shm_obj_secondary( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, procshared_mem_defer );
		shm_obj_secondary.allocate_shm_as_secondary();
		if ( not shm_obj_secondary.debug_test_integrity() ) {
			return 1;
		}
		test_shared_data* p_sut = reinterpret_cast<test_shared_data*>( shm_obj_secondary.get() );

		std::unique_lock<procshared_mutex> lk( p_sut->mtx_ );
		bool ret = p_sut->cond_.wait_for(
			lk,
			std::chrono::milliseconds( 20 ),
			[p_sut]() { return p_sut->shared_state_flag_; } );
		return ( ret ) ? 2 : 3;
	} );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

	// Act
	{
		std::unique_lock<procshared_mutex> lk( p_sut->mtx_ );
		p_sut->shared_state_flag_ = true;
		p_sut->cond_.notify_one();
	}

	// Assert
	child_proc_return_t ret { false, 0, 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 2 );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}
#endif   // TEST_ENABLE_ADDRESSSANITIZER
