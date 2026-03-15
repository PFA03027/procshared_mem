/**
 * @file test_ipsm_malloc.cpp
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

#include "ipsm_condition_variable.hpp"
#include "ipsm_malloc.hpp"
#include "test_ipsm_common.hpp"

TEST( Test_ipsm_malloc, CanDefaultConstruct_CanDestruct )
{
	// Arrange

	// Act
	ASSERT_NO_THROW(
		{
			ipsm::ipsm_malloc shm_obj;
		} );

	// Assert
}

TEST( Test_ipsm_malloc, CanDefaultConstruct_ThenAllocate )
{
	// Arrange
	ipsm::ipsm_malloc sut;

	// Act
	auto p = sut.allocate( 10 );

	// Assert
	EXPECT_EQ( p, nullptr );
}

TEST( Test_ipsm_malloc, CanConstruct_CanDestruct )
{
	// Arrange
	std::string shm_name            = "/test_ipsm_malloc_" + std::to_string( getpid() );
	std::string lifetime_ctrl_fname = "/tmp/test_ipsm_malloc_lifetime_ctrl_" + std::to_string( getpid() );

	// Act
	ASSERT_NO_THROW(
		{
			ipsm::ipsm_malloc shm_obj( shm_name.c_str(), lifetime_ctrl_fname.c_str(), 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
		} );

	// Assert
}

TEST( Test_ipsm_malloc, CanMoveConstruct )
{
	// Arrange
	std::string       shm_name            = "/test_ipsm_malloc_" + std::to_string( getpid() );
	std::string       lifetime_ctrl_fname = "/tmp/test_ipsm_malloc_lifetime_ctrl_" + std::to_string( getpid() );
	ipsm::ipsm_malloc sut;

	// Act
	ASSERT_NO_THROW(
		{
			sut = ipsm::ipsm_malloc( shm_name.c_str(), lifetime_ctrl_fname.c_str(), 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
		} );

	// Assert
}

// ===============================================================================
class TestIpsmMallocFixture : public ::testing::Test {
protected:
	std::string       shm_name_;
	std::string       lifetime_ctrl_fname;
	ipsm::ipsm_malloc sut_;

	void SetUp( void ) override
	{
		shm_name_           = "/test_ipsm_malloc_" + std::to_string( getpid() );
		lifetime_ctrl_fname = "/tmp/test_ipsm_malloc_lifetime_ctrl_" + std::to_string( getpid() );
		sut_                = ipsm::ipsm_malloc( shm_name_.c_str(), lifetime_ctrl_fname.c_str(), 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	}

	void TearDown( void ) override
	{
	}
};

TEST_F( TestIpsmMallocFixture, CanConstruct_ThenAllocate )
{
	// Arrange

	// Act
	auto p = sut_.allocate( 10 );

	// Assert
	EXPECT_NE( p, nullptr );
}

TEST_F( TestIpsmMallocFixture, CanConstruct_ThenAllocateDeallocate )
{
	// Arrange
	auto p = sut_.allocate( 10 );
	EXPECT_NE( p, nullptr );

	// Act
	ASSERT_NO_THROW( { sut_.deallocate( p ); } );

	// Assert
}

TEST_F( TestIpsmMallocFixture, CanMoveConstruct_ThenAllocate )
{
	// Arrange
	ipsm::ipsm_malloc sut2( std::move( sut_ ) );

	// Act
	auto p = sut2.allocate( 10 );

	// Assert
	EXPECT_NE( p, nullptr );
}

TEST_F( TestIpsmMallocFixture, CanMoveConstruct_ThenAllocateDeallocate )
{
	// Arrange
	ipsm::ipsm_malloc sut2( std::move( sut_ ) );
	auto              p = sut2.allocate( 10 );
	EXPECT_NE( p, nullptr );

	// Act
	ASSERT_NO_THROW( { sut2.deallocate( p ); } );

	// Assert
}

TEST_F( TestIpsmMallocFixture, CanMoveAssignment )
{
	// Arrange
	auto p1 = sut_.allocate( 10 );
	EXPECT_NE( p1, nullptr );

	std::string shm_name2            = "/test_ipsm_malloc2_" + std::to_string( getpid() );
	std::string lifetime_ctrl_fname2 = "/tmp/test_ipsm_malloc_lifetime_ctrl2_" + std::to_string( getpid() );

	// Act
	sut_ = ipsm::ipsm_malloc( shm_name2.c_str(), lifetime_ctrl_fname2.c_str(), 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

	// Assert
	auto p2 = sut_.allocate( 10 );
	EXPECT_NE( p2, nullptr );
	EXPECT_NE( p1, p2 );
}

TEST_F( TestIpsmMallocFixture, CanMoveAssignment_ThenDeallocate )
{
	// Arrange
	auto p1 = sut_.allocate( 10 );
	EXPECT_NE( p1, nullptr );

	std::string shm_name2            = "/test_ipsm_malloc2_" + std::to_string( getpid() );
	std::string lifetime_ctrl_fname2 = "/tmp/test_ipsm_malloc_lifetime_ctrl2_" + std::to_string( getpid() );
	sut_                             = ipsm::ipsm_malloc( shm_name2.c_str(), lifetime_ctrl_fname2.c_str(), 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	auto p2                          = sut_.allocate( 10 );
	EXPECT_NE( p2, nullptr );
	EXPECT_NE( p1, p2 );

	// Act
	ASSERT_NO_THROW( { sut_.deallocate( p2 ); } );

	// Assert
}

TEST_F( TestIpsmMallocFixture, TryOversizeAllocation )
{
	// Arrange

	// Act
	auto p = sut_.allocate( 4096 * 2 );   // 共有メモリのサイズを超えるサイズの割り当てを試みる

	// Assert
	EXPECT_EQ( p, nullptr );
}

TEST_F( TestIpsmMallocFixture, CanSend_ThenReceive )
{
	// Arrange

	int* p_send = ipsm::allocate_instance<int>( sut_.get_allocator<int>(), 12345 );

	// Act
	sut_.send( 0, p_send );   // 通常の生ポインタは、send()の引数型であるoffset_ptr<void>に暗黙的に変換され、送信される。

	// Assert
	auto p_recv = sut_.receive( 0 );
	ASSERT_NE( p_recv, nullptr );
	EXPECT_EQ( *reinterpret_cast<int*>( p_recv.get() ), 12345 );
}

TEST_F( TestIpsmMallocFixture, ChannelIsEmpty_CanTryReceive_ThenReturnNullopt )
{
	// Arrange

	// Act
	auto opt_recv = sut_.try_receive( 0 );

	// Assert
	EXPECT_EQ( opt_recv, std::nullopt );
}

TEST_F( TestIpsmMallocFixture, ChannelIsEmpty_CanTryReceiveUntilWithStdChronoSteadyClockNow_ThenReturnNullopt )
{
	// Arrange

	// Act
	auto opt_recv = sut_.try_receive_until( 0, std::chrono::steady_clock::now() );

	// Assert
	EXPECT_EQ( opt_recv, std::nullopt );
}

TEST_F( TestIpsmMallocFixture, ChannelIsEmpty_CanTryReceiveFor1msec_ThenReturnNullopt )
{
	// Arrange

	// Act
	auto opt_recv = sut_.try_receive_for( 0, std::chrono::milliseconds( 1 ) );

	// Assert
	EXPECT_EQ( opt_recv, std::nullopt );
}

TEST_F( TestIpsmMallocFixture, SendMsg_CanTryReceiveFor1sec_ThenReturnMsg )
{
	// Arrange
	std::thread sender( [this]() {
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		int* p_send = ipsm::allocate_instance<int>( sut_.get_allocator<int>(), 12345 );
		sut_.send( 0, p_send );
	} );

	// Act
	auto opt_recv = sut_.try_receive_for( 0, std::chrono::seconds( 1 ) );

	// Assert
	if ( sender.joinable() ) {
		sender.join();
	}
	ASSERT_NE( opt_recv, std::nullopt );
	EXPECT_NE( *opt_recv, nullptr );
	EXPECT_EQ( *( opt_recv->reinterpret_to_offset_ptr<int>() ), 12345 );

	// Clean up
}

#if defined( TEST_ENABLE_ADDRESSSANITIZER ) || defined( TEST_ENABLE_LEAKSANITIZER )
#else
TEST( Test_ipsm_malloc, CanAllocateBwProcess )
{
	// Arrange
	std::string       shm_name            = "/test_ipsm_malloc_" + std::to_string( getpid() );
	std::string       lifetime_ctrl_fname = "/tmp/test_ipsm_malloc_lifetime_ctrl_" + std::to_string( getpid() );
	ipsm::ipsm_malloc shm_malloc_obj( shm_name.c_str(), lifetime_ctrl_fname.c_str(), 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	EXPECT_EQ( shm_malloc_obj.get_bind_count(), 1 + shm_malloc_obj.channel_size() );

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t>                                f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), [shm_name, lifetime_ctrl_fname]() -> int {
		ipsm::ipsm_malloc sut_secondary( shm_name.c_str(), lifetime_ctrl_fname.c_str(), 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
		if ( sut_secondary.get_bind_count() < 0 ) {
			fprintf( stderr, "Error: bind count is %d\n", sut_secondary.get_bind_count() );
			return 4;
		}
		if ( static_cast<unsigned int>( sut_secondary.get_bind_count() ) != ( 2U + sut_secondary.channel_size() ) ) {
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
	std::thread                      t;
};

template <typename T>
class fifo_que {
public:
	fifo_que( const ipsm::offset_allocator<T>& a )
	  : mtx_()
	  , cond_()
	  , que_( a )
	{
	}

	void push( const T& v )
	{
		std::lock_guard<ipsm::ipsm_mutex> lk( mtx_ );
		que_.emplace_back( v );
	}
	void push( T&& v )
	{
		std::lock_guard<ipsm::ipsm_mutex> lk( mtx_ );
		que_.emplace_back( std::move( v ) );
	}
	T pop( void )
	{
		std::unique_lock<ipsm::ipsm_mutex> lk( mtx_ );
		cond_.wait( lk, [this]() -> bool {
			return !( que_.empty() );
		} );
		T ans = std::move( que_.front() );
		que_.pop_front();
		return ans;
	}

private:
	ipsm::ipsm_mutex                          mtx_;
	ipsm::ipsm_condition_variable_monotonic   cond_;
	ipsm::offset_list<T, ipsm::offset_allocator<T>> que_;
};

#define SHM_OBJ_NAME_STRING               "/my_test_shm_test_ipsm_malloc"
#define SHM_OBJ_NAME_LIFETIME_CTRL_STRING "/tmp/my_test_shm_test_ipsm_malloc.lifetime_ctrl"

TEST( Test_ipsm_malloc, CanMsgChannel )
{
	// Arrange
	constexpr int     num_of_threads = 100;
	constexpr int     num_of_loop    = 10000;
	ipsm::ipsm_malloc shm_malloc_obj( SHM_OBJ_NAME_STRING, SHM_OBJ_NAME_LIFETIME_CTRL_STRING, 4096UL * 100UL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	fifo_que<int>*    p_sut_list = ipsm::allocate_instance<fifo_que<int>>( shm_malloc_obj.get_allocator<fifo_que<int>>(), shm_malloc_obj.get_allocator<int>() );
	proc_task_data    pt_pack[num_of_threads];
	// Act
	for ( auto& e : pt_pack ) {
		shm_malloc_obj.send( 0, p_sut_list );
		std::packaged_task<child_proc_return_t( std::function<int()> )> task( call_pred_on_child_process );
		e.f = task.get_future();
		e.t = std::thread( std::move( task ), []() -> int {
			ipsm::ipsm_malloc sut_secondary( SHM_OBJ_NAME_STRING, SHM_OBJ_NAME_LIFETIME_CTRL_STRING, 4096UL * 100UL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
			void*             p_tmp      = sut_secondary.receive( 0 );
			fifo_que<int>*    p_sut_list = reinterpret_cast<fifo_que<int>*>( p_tmp );

			int v = 0;
			for ( int i = 0; i < num_of_loop; i++ ) {
				p_sut_list->push( v );
				int vv = p_sut_list->pop();
				v      = vv + 1;
			}

			int* p_ret = ipsm::allocate_instance<int>( sut_secondary.get_allocator<int>(), v );
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
		int*  p_ret = reinterpret_cast<int*>( p_tmp );
		ASSERT_NE( p_ret, nullptr );
		sum_value += *p_ret;
		destruct_obj_usee_allocator( shm_malloc_obj.get_allocator<int>(), p_ret );
	}
	EXPECT_EQ( sum_value, num_of_loop * num_of_threads );

	// Cleanup
	destruct_obj_usee_allocator( shm_malloc_obj.get_allocator<fifo_que<int>>(), p_sut_list );
}

#endif   // TEST_ENABLE_ADDRESSSANITIZER
