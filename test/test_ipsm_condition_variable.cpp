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
#include <mutex>
#include <thread>

#include <pthread.h>

#include "gtest/gtest.h"

#include "ipsm_condition_variable.hpp"
#include "ipsm_mem.hpp"
#include "ipsm_mutex.hpp"
#include "ipsm_recursive_mutex.hpp"
#include "test_ipsm_common.hpp"

using namespace ipsm;

TEST( Test_ipsm_condition_variable, CanConstruct_CanDestruct )
{
	ASSERT_NO_THROW( ipsm_condition_variable_monotonic sut );
}

TEST( Test_ipsm_condition_variable, CanWait_CanNotifyAll )
{
	// Arrange
	bool                                    shared_state_flag = false;
	ipsm_mutex                        mtx;
	ipsm_condition_variable_monotonic sut1;
	ipsm_condition_variable_monotonic sut2;

	std::packaged_task<bool()> task1( [&sut1, &mtx, &shared_state_flag]() {
		std::unique_lock<ipsm_mutex> lk( mtx );
		sut1.wait( lk, [&shared_state_flag]() { return shared_state_flag; } );
		return true;
	} );   // 非同期実行する関数を登録する
	std::future<bool>          f1 = task1.get_future();
	std::packaged_task<bool()> task2( [&sut2, &mtx, &shared_state_flag]() {
		std::unique_lock<ipsm_mutex> lk( mtx );
		sut2.wait( lk, [&shared_state_flag]() { return shared_state_flag; } );
		return true;
	} );   // 非同期実行する関数を登録する
	std::future<bool>          f2 = task2.get_future();

	std::thread t1( std::move( task1 ) );
	std::thread t2( std::move( task2 ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

	// Act
	{
		std::unique_lock<ipsm_mutex> lk( mtx );
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

TEST( Test_ipsm_condition_variable, CanWaitFor_Timeout )
{
	// Arrange
	// bool                          shared_state_flag = false;
	ipsm_mutex                        mtx;
	ipsm_condition_variable_monotonic sut1;

	std::packaged_task<std::cv_status()> task1( [&sut1, &mtx]() {
		std::unique_lock<ipsm_mutex> lk( mtx );
		return sut1.wait_for( lk, std::chrono::milliseconds( 10 ) );
	} );   // 非同期実行する関数を登録する
	std::future<std::cv_status>          f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
	// {
	// 	std::unique_lock<ipsm_mutex> lk( mtx );
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

TEST( Test_ipsm_condition_variable, CanWaitFor_NoTimeout )
{
	// Arrange
	pthread_barrier_t br_obj;
	ASSERT_EQ( pthread_barrier_init( &br_obj, nullptr, 2 ), 0 );

	bool                                    shared_state_flag = false;
	ipsm_mutex                        mtx;
	ipsm_condition_variable_monotonic sut1;

	std::packaged_task<std::cv_status()> task1( [&sut1, &mtx, &br_obj]() {
		pthread_barrier_wait( &br_obj );
		std::unique_lock<ipsm_mutex> lk( mtx );
		return sut1.wait_for( lk, std::chrono::milliseconds( 2000 ) );
	} );   // 非同期実行する関数を登録する
	std::future<std::cv_status>          f1 = task1.get_future();
	std::thread                          t1( std::move( task1 ) );

	pthread_barrier_wait( &br_obj );
	std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

	// Act
	{
		std::unique_lock<ipsm_mutex> lk( mtx );
		shared_state_flag = true;
		sut1.notify_all();   // これが実行されるまでに、sut1.wait_for()で待機列に入っている必要がある。wait_for()はその保証が難しいため、notify_all()の呼び出しをなるべく遅くするようにチューニングするしかない。
	}

	// Assert
	std::cv_status ret = std::cv_status::timeout;
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_EQ( ret, std::cv_status::no_timeout );
	EXPECT_TRUE( shared_state_flag );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
	pthread_barrier_destroy( &br_obj );
}

TEST( Test_ipsm_condition_variable, CanWaitForPred_Timeout )
{
	// Arrange
	bool                                    shared_state_flag = false;
	ipsm_mutex                        mtx;
	ipsm_condition_variable_monotonic sut1;

	std::packaged_task<bool()> task1( [&sut1, &mtx, &shared_state_flag]() {
		std::unique_lock<ipsm_mutex> lk( mtx );
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

TEST( Test_ipsm_condition_variable, CanWaitForPred_NoTimeout )
{
	// Arrange
	bool                                    shared_state_flag = false;
	ipsm_mutex                        mtx;
	ipsm_condition_variable_monotonic sut1;

	std::packaged_task<bool()> task1( [&sut1, &mtx, &shared_state_flag]() {
		std::unique_lock<ipsm_mutex> lk( mtx );
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
		std::unique_lock<ipsm_mutex> lk( mtx );
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
const char* p_shm_obj_name = "/my_test_shm_test_ipsm_condition_variable";

struct test_shared_data {
	bool shared_state_flag_;
	ipsm_mutex mtx_;
	ipsm_condition_variable_monotonic cond_;

	test_shared_data( void )
	  : shared_state_flag_( false )
	  , mtx_()
	  , cond_()
	{
	}
};

TEST( Test_ipsm_condition_variable_bw_proc, CanWaitForPred_Timeout )
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
			[[maybe_unused]] test_shared_data* p_sut = new ( p_mem ) test_shared_data();
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );
	[[maybe_unused]] test_shared_data* p_sut = reinterpret_cast<test_shared_data*>( shm_obj.get() );

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t> f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), []() -> int {
		ipsm_mem shm_obj_secondary;
		shm_obj_secondary.allocate_shm_as_secondary(
			p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			[]( void*, size_t ) { /* 何もしない */ } );
		if ( not shm_obj_secondary.debug_test_integrity() ) {
			return 1;
		}
		test_shared_data* p_sut = reinterpret_cast<test_shared_data*>( shm_obj_secondary.get() );

		std::unique_lock<ipsm_mutex> lk( p_sut->mtx_ );
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

TEST( Test_ipsm_condition_variable_bw_proc, CanWaitForPred_NoTimeout )
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
			[[maybe_unused]] test_shared_data* p_sut = new ( p_mem ) test_shared_data();
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );
	test_shared_data* p_sut = reinterpret_cast<test_shared_data*>( shm_obj.get() );

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t> f1 = task1.get_future();

	std::thread t1( std::move( task1 ), []() -> int {
		ipsm_mem shm_obj_secondary;
		shm_obj_secondary.allocate_shm_as_secondary(
			p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			[]( void*, size_t ) { /* 何もしない */ } );
		if ( not shm_obj_secondary.debug_test_integrity() ) {
			return 1;
		}
		test_shared_data* p_sut = reinterpret_cast<test_shared_data*>( shm_obj_secondary.get() );

		std::unique_lock<ipsm_mutex> lk( p_sut->mtx_ );
		bool ret = p_sut->cond_.wait_for(
			lk,
			std::chrono::milliseconds( 20 ),
			[p_sut]() { return p_sut->shared_state_flag_; } );
		return ( ret ) ? 2 : 3;
	} );
	std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

	// Act
	{
		std::unique_lock<ipsm_mutex> lk( p_sut->mtx_ );
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
