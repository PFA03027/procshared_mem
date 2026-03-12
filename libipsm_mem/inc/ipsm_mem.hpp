/**
 * @file ipsm_mem.hpp
 * @author PFA03027@nifty.com
 * @brief async open/close shared memory object
 * @version 0.1
 * @date 2023-09-03
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef IPSM_MEM_HPP_
#define IPSM_MEM_HPP_

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>

#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

namespace ipsm {

/**
 * @brief A shared memory management class that performs autonomous and distributed construction processing
 */
class ipsm_mem {
public:
	enum class status : std::uintptr_t {
		initializing = 0x1111'1111'1111'1111UL,   //!< initializing
		ready        = 0x2222'2222'2222'2222UL,   //!< ready to use
	};

	/**
	 * @brief Construct a new procshared mem object that is empty
	 *
	 */
	ipsm_mem( void );
	~ipsm_mem();
	ipsm_mem( ipsm_mem&& src );
	ipsm_mem& operator=( ipsm_mem&& src );

	void swap( ipsm_mem& src );

	/**
	 * @brief allocate a new cooperative startup shared memory object
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception ipsm_mem_error
	 */
	bool setup(
		const char*                            p_shm_name,                   //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                            p_lifetime_ctrl_fname,        //!< [in] lifetime control file name.
		size_t                                 length,                       //!< [in] shared memory size
		mode_t                                 mode,                         //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<size_t( void*, size_t )> init_functor_arg,             //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length. return value is consumed memory size. this functor is called multiple by internal retry caused by shared mamory creation collision or shared memory initialization failure of other process.
		int                                    timeout_msec        = 1000,   //!< [in] timeout in milliseconds for waiting for shared memory initialization.
		int                                    retry_interval_msec = 100     //!< [in] retry interval in milliseconds for waiting for shared memory initialization.
	);

	void*  get( void ) const;              //!< get top address of memory area
	size_t available_size( void ) const;   //!< larger than or equal to the size specified in constructor or allocate_shm_as_both.

	status         get_status( void ) const;
	std::uintptr_t get_hint_value( void ) const;

private:
	ipsm_mem( const ipsm_mem& )            = delete;
	ipsm_mem& operator=( const ipsm_mem& ) = delete;

	class impl;
	impl* p_impl_;
};

}   // namespace ipsm

#endif   // IPSM_MEM_HPP_
