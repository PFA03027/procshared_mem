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
	procshared_malloc shm_malloc_obj( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t> f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), []() -> int {
		procshared_malloc sut_secondary( p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
		if ( sut_secondary.get_bind_count() != 2 ) {
			fprintf( stderr, "Error: bind count is %d\n", sut_secondary.get_bind_count() );
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

struct proc_task_data {
	std::future<child_proc_return_t> f;
	std::thread t;
};

template <typename T>
class fifo_que {
public:
	fifo_que( const offset_allocator<T>& a )
	  : mtx_()
	  , cond_()
	  , que_( a )
	{
	}

	void push( const T& v )
	{
		std::lock_guard<procshared_mutex> lk( mtx_ );
		que_.emplace_back( v );
	}
	void push( T&& v )
	{
		std::lock_guard<procshared_mutex> lk( mtx_ );
		que_.emplace_back( std::move( v ) );
	}
	T pop( void )
	{
		std::unique_lock<procshared_mutex> lk( mtx_ );
		cond_.wait( lk, [this]() -> bool {
			return !( que_.empty() );
		} );
		T ans = std::move( que_.front() );
		que_.pop_front();
		return ans;
	}

private:
	procshared_mutex mtx_;
	procshared_condition_variable cond_;
	offset_list<T, offset_allocator<T>> que_;
};

TEST( Test_procshared_malloc, CanMsgChannel )
{
	// Arrange
	constexpr int num_of_threads = 100;
	constexpr int num_of_loop = 10000;
	procshared_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	procshared_malloc shm_malloc_obj( p_shm_obj_name, "/tmp", 4096UL * 100UL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	fifo_que<int>* p_sut_list = make_obj_construct_using_allocator<fifo_que<int>>( shm_malloc_obj.get_allocator<fifo_que<int>>(), shm_malloc_obj.get_allocator<int>() );
	proc_task_data pt_pack[num_of_threads];
	// Act
	for ( auto& e : pt_pack ) {
		shm_malloc_obj.send( 0, p_sut_list );
		std::packaged_task<child_proc_return_t( std::function<int()> )> task( call_pred_on_child_process );
		e.f = task.get_future();
		e.t = std::thread( std::move( task ), []() -> int {
			procshared_malloc sut_secondary( p_shm_obj_name, "/tmp", 4096UL * 100UL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
			void* p_tmp = sut_secondary.receive( 0 );
			fifo_que<int>* p_sut_list = reinterpret_cast<fifo_que<int>*>( p_tmp );

			int v = 0;
			for ( int i = 0; i < num_of_loop; i++ ) {
				p_sut_list->push( v );
				int vv = p_sut_list->pop();
				v = vv + 1;
			}

			int* p_ret = make_obj_construct_using_allocator<int>( sut_secondary.get_allocator<int>(), v );
			sut_secondary.send( 1, p_ret );
			return EXIT_SUCCESS;
		} );
	}

	// Assert
	int sum_value = 0;
	for ( auto& e : pt_pack ) {
		child_proc_return_t ret = { 0 };
		ASSERT_NO_THROW( ret = e.f.get() );
		EXPECT_TRUE( ret.is_exit_normaly_ );
		EXPECT_EQ( ret.exit_code_, EXIT_SUCCESS );
		if ( e.t.joinable() ) {
			e.t.join();
		}

		void* p_tmp = shm_malloc_obj.receive( 1 );
		int* p_ret = reinterpret_cast<int*>( p_tmp );
		ASSERT_NE( p_ret, nullptr );
		sum_value += *p_ret;
		destruct_obj_usee_allocator( shm_malloc_obj.get_allocator<int>(), p_ret );
	}
	EXPECT_EQ( sum_value, num_of_loop * num_of_threads );

	// Cleanup
	destruct_obj_usee_allocator( shm_malloc_obj.get_allocator<fifo_que<int>>(), p_sut_list );
}

#endif   // TEST_ENABLE_ADDRESSSANITIZER
