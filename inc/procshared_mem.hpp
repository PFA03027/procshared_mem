/**
 * @file procshared_mem.hpp
 * @author PFA03027@nifty.com
 * @brief async open/close shared memory object
 * @version 0.1
 * @date 2023-09-03
 *
 * @copyright Copyright (c) 2023, PFA03027@nifty.com
 *
 */

#ifndef PROCSHARED_MEM_HPP_
#define PROCSHARED_MEM_HPP_

#include <functional>
#include <stdexcept>
#include <string>

#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

using type_of_errno = typename std::remove_reference<decltype( errno )>::type;

class procshared_mem_error : public std::runtime_error {
public:
	procshared_mem_error( type_of_errno e_v );
	procshared_mem_error( type_of_errno e_v, const std::string& additional_error_str );
	procshared_mem_error( const char* p_error_str );
	procshared_mem_error( const std::string& error_str );
};

/**
 * @brief Async initialized shared memory
 *
 * @note
 * If you use this class for testing in one process, please intent an instance as primary explicitly.
 * sem_open return a same object of semaphore, even if sem_open is called by a different instance.
 * this will lead unexpected creation/destruction rece condition.
 */
class procshared_mem {
public:
	/**
	 * @brief Construct a new procshared mem object that is empty
	 *
	 */
	procshared_mem( void );

	~procshared_mem();

	procshared_mem( procshared_mem&& src );

	procshared_mem& operator=( procshared_mem&& src );

	/**
	 * @brief Construct and allocate a new cooperative startup shared memory object
	 *
	 * This constructor allocates a shared memory during constructor.
	 * If an instance got as a primary role, it calls a functor initfunctor_arg() after finish setup of a shared memory
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 * @param initfunctor_arg a functor to initialize a shared memory area that length is same to an argument 'length'
	 *
	 * @exception if failed creation by any reason, throw std::bad_alloc(in case of new operator throws) or std::run_time_error
	 *
	 * @note p_shm_name string AAA must follow POSIX semaphore name specifications. please refer sem_open or sem_overview
	 */
	procshared_mem( const char* p_shm_name, const char* p_id_dirname, size_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg );

	/**
	 * @brief allocate a new cooperative startup shared memory object
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 * @param initfunctor_arg a functor to initialize a shared memory area that length is same to an argument 'length'
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception procshared_mem_error
	 */
	void allocate_shm_as_both( const char* p_shm_name, const char* p_id_dirname, size_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg );

	/**
	 * @brief allocate a new cooperative startup shared memory object as primary
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 * @param initfunctor_arg a functor to initialize a shared memory area that length is same to an argument 'length'
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception procshared_mem_error
	 */
	void allocate_shm_as_primary( const char* p_shm_name, const char* p_id_dirname, size_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg );

	/**
	 * @brief allocate a new cooperative startup shared memory object as secondary
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 *
	 * @pre this instance is default constructed instance
	 *
	 * @exception procshared_mem_error
	 */
	void allocate_shm_as_secondary( const char* p_shm_name, const char* p_id_dirname, size_t length, mode_t mode );

	size_t available_size( void ) const;

	void* get( void );

	ino_t       debug_get_id_file_inode( void ) const;
	bool        debug_test_integrity( void ) const;
	std::string debug_dump_string( void ) const;
	static void debug_force_cleanup( const char* p_shm_name, const char* p_id_dirname );

private:
	procshared_mem( const procshared_mem& )            = delete;
	procshared_mem& operator=( const procshared_mem& ) = delete;

	class impl;
	impl* p_impl_;
};

#endif   // PROCSHARED_MEM_HPP_
