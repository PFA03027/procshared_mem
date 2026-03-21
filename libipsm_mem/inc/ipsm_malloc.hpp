/**
 * @file ipsm_malloc.hpp
 * @author PFA03027@nifty.com
 * @brief memory allocator from procshared memory
 * @version 0.1
 * @date 2023-12-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef IPSM_MALLOC_HPP_
#define IPSM_MALLOC_HPP_

#include <chrono>
#include <cstddef>
#include <optional>

#include "ipsm_mem.hpp"
#include "ipsm_mutex.hpp"
#include "ipsm_time_util.hpp"
#include "offset_allocator.hpp"
#include "offset_list.hpp"
#include "offset_malloc.hpp"

namespace ipsm {

struct msg_channels;

class ipsm_malloc {
public:
	~ipsm_malloc();
	ipsm_malloc( void );
	ipsm_malloc( ipsm_malloc&& src ) = default;
	ipsm_malloc& operator=( ipsm_malloc&& src );

	/**
	 * @brief Construct and allocate a new cooperative startup shared memory object
	 *
	 * This constructor allocates a shared memory during constructor.
	 * If an instance got as a primary role, it calls a functor initfunctor_arg() after finish setup of a shared memory
	 *
	 * @exception if failed creation by any reason, throw std::bad_alloc(in case of new operator throws) or std::run_time_error
	 *
	 * @note p_shm_name string AAA must follow POSIX semaphore name specifications. please refer sem_open or sem_overview
	 */
	ipsm_malloc(
		const char* p_shm_name,                   //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char* p_lifetime_ctrl_fname,        //!< [in] lifetime control file name.
		size_t      length,                       //!< [in] shared memory size
		mode_t      mode,                         //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		size_t      channel_size        = 2,      //!< [in] the number of channels for message passing. this value must be agreed upon in advance between communicating processes.
		int         timeout_msec        = 1000,   //!< [in] timeout in milliseconds for waiting for shared memory initialization.
		int         retry_interval_msec = 100     //!< [in] retry interval in milliseconds for waiting for shared memory initialization.
	);

	/**
	 * @brief Allocate memory from shared memory
	 *
	 * @param n the number of bytes to allocate
	 * @param alignment the alignment of allocated memory. default value is alignof(std::max_align_t). if alignment is 0, it is treated as 1.
	 *
	 * @return void* pointer to allocated memory as the address of a process. if allocation failed, return nullptr.
	 *
	 * @note
	 * This interface is analogy to std::malloc();
	 */
#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void* allocate( size_t n, size_t alignment = alignof( std::max_align_t ) );

	/**
	 * @brief Deallocate memory from shared memory
	 *
	 * @param p pointer to the memory to deallocate
	 * @param alignment the alignment of the memory to deallocate. default value is alignof(std::max_align_t). if alignment is 0, it is treated as 1.
	 *
	 * @note
	 * This interface is analogy to std::free();
	 */
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

	/**
	 * @brief allocate and construct T instance.
	 *
	 * @tparam T allocating type
	 * @tparam Args template parameter pack of argument types to construct T
	 * @param args paramter pack of arguments to construct T
	 * @return T* pointer to a constructed instance
	 *
	 * @note
	 * This interface is analogy to non-array type new expression
	 */
	template <typename T, typename... Args>
	T* new_instance( Args&&... args )
	{
		return shm_heap_.new_instance( std::forward<Args>( args )... );
	}

	/**
	 * @brief destroy and deallocate T instance.
	 *
	 * @tparam T allocating type
	 * @param p pointer to an allocated instance that constructed by new_instance()
	 * @return T* pointer to a constructed instance
	 *
	 * @note
	 * This interface is analogy to non-array type delete expression
	 */
	template <typename T>
	void delete_instance( T* p )
	{
		shm_heap_.delete_instance( p );
	}

	/**
	 * @brief allocate and construct T array
	 *
	 * @tparam T element type of array
	 * @param n number of elements
	 * @return T* pointer to the constructed array
	 *
	 * @note
	 * This interface is analogy to array type new expression
	 */
	template <typename T>
	T* new_array( size_t n )
	{
		return shm_heap_.new_array<T>( n );
	}

	/**
	 * @brief destroy and deallocate T array
	 *
	 * @tparam T element type of array
	 * @param p pointer to the array
	 *
	 * @note
	 * This interface is analogy to array type delete expression
	 */
	template <typename T>
	void delete_array( T* p )
	{
		shm_heap_.delete_array( p );
	}

	/**
	 * @brief Get the bind count object
	 *
	 * Indicates how many ipsm_malloc instances or their allocators are referencing this object. Mainly used for inspection in tests.
	 * Note: If any process referencing this shared memory terminates abnormally, this counter may not reflect the correct value.
	 *
	 * @return int
	 */
	int get_bind_count( void ) const;

	/**
	 * @brief Get the allocator object that is bound to this shared memory
	 *
	 * @tparam T
	 * @return offset_allocator<T>
	 */
	template <typename T>
	offset_allocator<T> get_allocator( void );

	/**
	 * @brief Get the offset malloc object reference
	 *
	 * @return const offset_malloc&
	 */
	const offset_malloc& get_offset_malloc( void ) const;

	/**
	 * @brief Send the value object
	 *
	 * Uses a pre-constructed message channel to send and receive values between processes.
	 *
	 * 事前に構築されているmessage channelを利用して、プロセス間で値を送受信する。
	 *
	 * @param ch Channel number. Specify a value in the range from 0 to channel_size() - 1. The channel number to use for communication must be agreed upon in advance between communicating processes.
	 * @param sending_value Offset pointer to the region obtained by allocate(). The type of value to send must be agreed upon in advance between communicating processes.
	 *
	 * @param ch チャンネル番号。0からchannel_size() - 1の範囲で指定する。どのチャンネル番号を使って通信するかは、通信するプロセス間で事前に合意しておく必要がある。
	 * @param sending_value allocate()で取得した領域へのオフセットポインタ。送信する値の型は、通信するプロセス間で事前に合意しておく必要がある。
	 */
	void send( unsigned int ch, offset_ptr<void> sending_value );

	/**
	 * @brief Receive a value object
	 *
	 * Uses a pre-constructed message channel to send and receive values between processes.
	 *
	 * 事前に構築されているmessage channelを利用して、プロセス間で値を送受信する。
	 *
	 * @param ch Channel number. Specify a value in the range from 0 to channel_size() - 1. The channel number to use for communication must be agreed upon in advance between communicating processes.
	 * @return offset_ptr<void> Offset pointer to the region obtained by allocate(). The type of value to receive must be agreed upon in advance between communicating processes.
	 *
	 * @param ch チャンネル番号。0からchannel_size() - 1の範囲で指定する。どのチャンネル番号を使って通信するかは、通信するプロセス間で事前に合意しておく必要がある。
	 * @return offset_ptr<void> allocate()で取得した領域へのオフセットポインタ。受信する値の型は、通信するプロセス間で事前に合意しておく必要がある。
	 */
	offset_ptr<void> receive( unsigned int ch );

	/**
	 * @brief Try to receive a value object
	 *
	 * please refer to receive() for details. This function does not block and returns immediately if no value is available.
	 */
	std::optional<offset_ptr<void>> try_receive( unsigned int ch );

	/**
	 * @brief Try to receive a value object until the specified absolute timeout time
	 *
	 * please refer to receive() for details. This function returns std::nullopt if no value is available until the specified absolute timeout time.
	 */
	std::optional<offset_ptr<void>> try_receive_until( unsigned int ch, const time_util::timespec_monotonic& abs_timeout_time );

	/**
	 * @brief Try to receive a value object until the specified relative timeout time
	 *
	 * please refer to receive() for details. This function returns std::nullopt if no value is available until the specified relative timeout time.
	 */
	template <class Rep, class Period>
	std::optional<offset_ptr<void>> try_receive_for( unsigned int ch, const std::chrono::duration<Rep, Period>& rel_time )
	{
		return try_receive_until( ch, time_util::timespec_monotonic::now() + rel_time );
	}

	/**
	 * @brief Get the number of channels
	 *
	 * @return size_t the number of channels
	 */
	size_t channel_size( void ) const;

private:
	ipsm_malloc( const ipsm_malloc& src )            = delete;
	ipsm_malloc& operator=( const ipsm_malloc& src ) = delete;

	void swap( ipsm_malloc& src );

	ipsm_mem      shm_obj_;    //!< shared memory object. this member variable declaration order required like ipsm_mem, then offset_malloc
	offset_malloc shm_heap_;   //!< offset base memory allocator on shared memory. this member variable declaration order required like ipsm_mem, then offset_malloc
	msg_channels* p_msgch_;
};

////////////////////////////////////////////////////////////////////
// Implement
template <typename T>
offset_allocator<T> ipsm_malloc::get_allocator( void )
{
	return offset_allocator<T>( shm_heap_ );   // NOLINT(clang-diagnostic-error)
}

}   // namespace ipsm

#endif   // IPSM_MALLOC_HPP_
