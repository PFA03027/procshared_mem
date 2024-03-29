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
#include "test_ipsm_common.hpp"

using namespace ipsm;

const char* p_shm_obj_name = "/my_test_shm_test_ipsm_mem";

TEST( Test_ipsm_mem, CanConstruct_CanDestruct )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	bool test_flag = false;

	// Act
	ASSERT_NO_THROW(
		ipsm_mem shm_obj(
			p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			[&test_flag]( void* p_mem, off_t len ) -> void* {
				if ( p_mem == nullptr ) {
					return nullptr;
				}
				if ( len < 4096 ) {
					return nullptr;
				}
				test_flag = true;
				return nullptr;
			},
			[]( void*, size_t ) { /* 何もしない */ } ) );

	// Assert
	EXPECT_TRUE( test_flag );
}

TEST( Test_ipsm_mem, CanConstructDefer_CanDestruct )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	bool           test_flag = false;
	ipsm_mem shm_obj;

	// Act
	shm_obj.allocate_shm_as_both(
		p_shm_obj_name, "/tmp", 4096 - 16, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		[&test_flag]( void* p_mem, off_t len ) -> void* {
			if ( p_mem == nullptr ) {
				return nullptr;
			}
			if ( len < ( 4096 - 16 ) ) {
				return nullptr;
			}
			test_flag = true;
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );

	// Assert
	EXPECT_TRUE( test_flag );
	EXPECT_GE( shm_obj.available_size(), 4096 - 16 );
	EXPECT_NE( shm_obj.get(), nullptr );
}

TEST( Test_ipsm_mem, CanConstructDefer_Primary )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	bool           test_flag = false;
	ipsm_mem shm_obj;

	// Act
	shm_obj.allocate_shm_as_primary(
		p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		[&test_flag]( void* p_mem, off_t len ) -> void* {
			if ( p_mem == nullptr ) {
				return nullptr;
			}
			if ( len < 4096 ) {
				return nullptr;
			}
			test_flag = true;
			return nullptr;
		} );

	// Assert
	EXPECT_TRUE( test_flag );
}

#if defined( TEST_ENABLE_ADDRESSSANITIZER ) || defined( TEST_ENABLE_LEAKSANITIZER )
#else
TEST( Test_ipsm_mem, CanConstruct_Secondary )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	bool           test_flag = false;
	ipsm_mem shm_obj(
		p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		[&test_flag]( void* p_mem, off_t len ) -> void* {
			if ( p_mem == nullptr ) {
				return nullptr;
			}
			if ( len < 4096 ) {
				return nullptr;
			}
			test_flag = true;
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );
	EXPECT_TRUE( test_flag );
	*( reinterpret_cast<int*>( shm_obj.get() ) ) = 12;

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
		return *( reinterpret_cast<int*>( shm_obj_secondary.get() ) );
	} );

	// Assert
	child_proc_return_t ret = { 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 12 );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

TEST( Test_ipsm_mem, CanConstructDefer_Secondary )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	bool           test_flag = false;
	ipsm_mem shm_obj(
		p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		[&test_flag]( void* p_mem, off_t len ) -> void* {
			if ( p_mem == nullptr ) {
				return nullptr;
			}
			if ( len < 4096 ) {
				return nullptr;
			}
			test_flag = true;
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );
	EXPECT_TRUE( test_flag );
	*( reinterpret_cast<int*>( shm_obj.get() ) ) = 12;

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
		return *( reinterpret_cast<int*>( shm_obj_secondary.get() ) );
	} );

	// Assert
	child_proc_return_t ret = { 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 12 );

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

TEST( Test_ipsm_mem, CanConstruct_Secondary_by_both )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	bool           test_flag = false;
	ipsm_mem shm_obj(
		p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		[&test_flag]( void* p_mem, off_t len ) -> void* {
			if ( p_mem == nullptr ) {
				return nullptr;
			}
			if ( len < 4096 ) {
				return nullptr;
			}
			test_flag = true;
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );
	EXPECT_TRUE( test_flag );
	*( reinterpret_cast<int*>( shm_obj.get() ) ) = 12;

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t>                                f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), []() -> int {
		ipsm_mem shm_obj_secondary_by_both(
			p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			[]( void* p_mem, off_t len ) -> void* {
				*( reinterpret_cast<int*>( p_mem ) ) = 22;   // nullptrチェックも行わない
				return nullptr;
			},
			[]( void*, size_t ) { /* 何もしない */ } );
		if ( not shm_obj_secondary_by_both.debug_test_integrity() ) {
			return 1;
		}
		return *( reinterpret_cast<int*>( shm_obj_secondary_by_both.get() ) );
	} );

	// Assert
	child_proc_return_t ret = { 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 12 );   // secondaryとして起動した場合、初期化処理を行うファンクタは実行されない。実行されていないことを確認する。

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}

TEST( Test_ipsm_mem, CanConstructDefer_Secondary_by_both )
{
	// Arrange
	ipsm_mem::debug_force_cleanup( p_shm_obj_name, "/tmp" );   // to remove ghost data
	bool           test_flag = false;
	ipsm_mem shm_obj(
		p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		[&test_flag]( void* p_mem, off_t len ) -> void* {
			if ( p_mem == nullptr ) {
				return nullptr;
			}
			if ( len < 4096 ) {
				return nullptr;
			}
			test_flag = true;
			return nullptr;
		},
		[]( void*, size_t ) { /* 何もしない */ } );
	EXPECT_TRUE( test_flag );
	*( reinterpret_cast<int*>( shm_obj.get() ) ) = 12;

	std::packaged_task<child_proc_return_t( std::function<int()> )> task1( call_pred_on_child_process );   // 非同期実行する関数を登録する
	std::future<child_proc_return_t>                                f1 = task1.get_future();

	// Act
	std::thread t1( std::move( task1 ), []() -> int {
		ipsm_mem shm_obj_secondary;
		shm_obj_secondary.allocate_shm_as_both(
			p_shm_obj_name, "/tmp", 4096, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			[]( void* p_mem, size_t len ) -> void* {
				*( reinterpret_cast<int*>( p_mem ) ) = 22;   // nullptrチェックも行わない
				return nullptr;
			},
			[]( void*, size_t ) { /* 何もしない */ } );
		if ( not shm_obj_secondary.debug_test_integrity() ) {
			return 1;
		}
		return *( reinterpret_cast<int*>( shm_obj_secondary.get() ) );
	} );

	// Assert
	child_proc_return_t ret = { 0 };
	ASSERT_NO_THROW( ret = f1.get() );
	EXPECT_TRUE( ret.is_exit_normaly_ );
	EXPECT_EQ( ret.exit_code_, 12 );   // secondaryとして起動した場合、初期化処理を行うファンクタは実行されない。実行されていないことを確認する。

	// Cleanup
	if ( t1.joinable() ) {
		t1.join();
	}
}
#endif   // TEST_ENABLE_ADDRESSSANITIZER
