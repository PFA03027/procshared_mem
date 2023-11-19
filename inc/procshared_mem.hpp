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
#include <string>
#include <tuple>
#include <type_traits>

#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

class semaphore_post_guard;
class procshared_mem_defer_t {};                      // tag type
extern procshared_mem_defer_t procshared_mem_defer;   // tag object

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
	 * @brief Construct a new cooperative startup shared memory object
	 *
	 * This constructor allocates no shared memory during constructor.
	 * To allocate a shared memory, please call allocate_shm_as_both(), allocate_shm_as_primary() or allocate_shm_as_secondary().
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 *
	 * @exception if failed creation by any reason, throw std::bad_alloc(in case of new operator throws) or std::run_time_error
	 *
	 * @note p_shm_name string AAA must follow POSIX semaphore name specifications. please refer sem_open or sem_overview
	 */
	procshared_mem( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, const procshared_mem_defer_t& procshared_mem_defer_val );

	/**
	 * @brief Construct a new cooperative startup shared memory object
	 *
	 * This constructor allocates a shared memory during constructor.
	 *
	 * @param p_shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param p_id_dirname directory name of id file. e.g. "/tmp"
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 *
	 * @exception if failed creation by any reason, throw std::bad_alloc(in case of new operator throws) or std::run_time_error
	 *
	 * @note p_shm_name string AAA must follow POSIX semaphore name specifications. please refer sem_open or sem_overview
	 */
	procshared_mem( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode );

	/**
	 * @brief Construct a new cooperative startup shared memory object
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
	procshared_mem( const char* p_shm_name, const char* p_id_dirname, off_t length, mode_t mode, std::function<void( void*, off_t )> initfunctor_arg );

	~procshared_mem();

	off_t available_size( void ) const;

	void* get( void );

	void allocate_shm_as_both( std::function<void( void*, off_t )> initfunctor_arg );
	void allocate_shm_as_primary( std::function<void( void*, off_t )> initfunctor_arg );
	void allocate_shm_as_secondary( void );

	ino_t       debug_get_id_file_inode( void ) const;
	bool        debug_test_integrity( void ) const;
	void        debug_set_as_owner( void );
	static void debug_force_cleanup( const char* p_shm_name );

private:
	struct procshared_mem_mem_header;

	void check_path_name( const char* p_shm_name );
	void setup_as_both( void );
	void setup_as_primary( void );
	void setup_as_secondary( void );
	void teardown( void );
	void teardown_in_semaphore_lock( semaphore_post_guard&& semguard_arg );
	void close_resources( void );
	void remove_resources( void );

	static std::string get_id_filename( const char* p_path_name_arg, const char* p_id_dirname_arg );
	static ino_t       get_inode_of_fd( int id_f_fd );
	static off_t       calc_total_len( off_t requested_length );

	const std::string                   pathname_;
	const std::string                   id_dirname_;
	const std::string                   id_fname_;
	const mode_t                        mode_;
	const off_t                         length_;
	bool                                is_owner_;
	sem_t*                              p_sem_;
	int                                 id_fd_;
	int                                 shm_fd_;
	procshared_mem_mem_header*          p_mem_;
	std::function<void( void*, off_t )> initfunctor_;
};

#endif   // PROCSHARED_MEM_HPP_
