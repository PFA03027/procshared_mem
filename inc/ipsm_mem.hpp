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

#include <functional>
#include <stdexcept>
#include <string>

#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

namespace ipsm {

/**
 * @brief Async initialized shared memory
 *
 * @note
 * If you use this class for testing in one process, please intent an instance as primary explicitly.
 * sem_open return a same object of semaphore, even if sem_open is called by a different instance.
 * this will lead unexpected creation/destruction rece condition.
 */
class ipsm_mem {
public:
	/**
	 * @brief Construct a new procshared mem object that is empty
	 *
	 */
	ipsm_mem( void );
	~ipsm_mem();
	ipsm_mem( ipsm_mem&& src );
	ipsm_mem& operator=( ipsm_mem&& src );

	/**
	 * @brief Construct and allocate a new cooperative startup shared memory object
	 *
	 * This constructor allocates a shared memory during constructor.
	 * If an instance got as a primary role, it calls a functor primary_functor_arg() after finish setup of a shared memory
	 *
	 * @exception if failed creation by any reason, throw std::bad_alloc(in case of new operator throws) or std::run_time_error
	 *
	 * @note p_shm_name string AAA must follow POSIX semaphore name specifications. please refer sem_open or sem_overview
	 */
	ipsm_mem(
		const char*                           p_shm_name,             //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                           p_id_dirname,           //!< [in] directory name of id file. e.g. "/tmp"
		size_t                                length,                 //!< [in] shared memory size
		mode_t                                mode,                   //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<void*( void*, size_t )> primary_functor_arg,    //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length. return value is set to opt_info.
		std::function<void( void*, size_t )>  secondary_functor_arg   //!< [in] a functor as secondary role that is initialized by other ipsm_mem instance. first argument is the pointer to the top of memory. second argument is the assigned memory length
	);

	/**
	 * @brief allocate a new cooperative startup shared memory object
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception ipsm_mem_error
	 */
	void allocate_shm_as_both(
		const char*                           p_shm_name,             //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                           p_id_dirname,           //!< [in] directory name of id file. e.g. "/tmp"
		size_t                                length,                 //!< [in] shared memory size
		mode_t                                mode,                   //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<void*( void*, size_t )> primary_functor_arg,    //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length. return value is set to opt_info.
		std::function<void( void*, size_t )>  secondary_functor_arg   //!< [in] a functor as secondary role that is initialized by other ipsm_mem instance. first argument is the pointer to the top of memory. second argument is the assigned memory length
	);

	/**
	 * @brief allocate a new cooperative startup shared memory object as primary
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception ipsm_mem_error
	 */
	void allocate_shm_as_primary(
		const char*                           p_shm_name,           //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                           p_id_dirname,         //!< [in] directory name of id file. e.g. "/tmp"
		size_t                                length,               //!< [in] shared memory size
		mode_t                                mode,                 //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<void*( void*, size_t )> primary_functor_arg   //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length. return value is set to opt_info.
	);

	/**
	 * @brief allocate a new cooperative startup shared memory object as secondary
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception ipsm_mem_error
	 */
	void allocate_shm_as_secondary(
		const char*                          p_shm_name,             //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                          p_id_dirname,           //!< [in] directory name of id file. e.g. "/tmp"
		size_t                               length,                 //!< [in] shared memory size
		mode_t                               mode,                   //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<void( void*, size_t )> secondary_functor_arg   //!< [in] a functor as secondary role that is initialized by other ipsm_mem instance. first argument is the pointer to the top of memory. second argument is the assigned memory length
	);

	void*  get( void );   //!< get top address of memory area
	size_t available_size( void ) const;

	void* get_opt_info( void ) const;
	void  set_opt_info( void* p );

	void swap( ipsm_mem& src );

	void set_teardown(
		std::function<void( bool, void*, size_t )> teardown_functor_arg   //!< [in]  a functor that is called when final deletion. this functor is stored in this instance
	);

	int get_bind_count( void ) const;

	ino_t       debug_get_id_file_inode( void ) const;
	bool        debug_test_integrity( void ) const;
	std::string debug_dump_string( void ) const;
	static void debug_force_cleanup( const char* p_shm_name, const char* p_id_dirname );

private:
	ipsm_mem( const ipsm_mem& )            = delete;
	ipsm_mem& operator=( const ipsm_mem& ) = delete;

	class impl;
	impl* p_impl_;
};

}   // namespace ipsm

#endif   // IPSM_MEM_HPP_
