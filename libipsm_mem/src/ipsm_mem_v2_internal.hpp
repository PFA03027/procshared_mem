/**
 * @file ipsm_mem_v2_internal.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-08
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef IPSM_MEM_V2_INTERNAL_HPP_
#define IPSM_MEM_V2_INTERNAL_HPP_

#include "ipsm_mem.hpp"

namespace ipsm_v2 {

class lock_file_guard {
public:
	~lock_file_guard();
	lock_file_guard( const std::string& fname, mode_t mode );

	bool try_exclusive_lock( void );
	bool try_shared_lock( void );
	void release_lock( void );

private:
	int  fd_;
	bool is_locked_;
};

class shm_guard {
public:
	~shm_guard();
	shm_guard( void );
	shm_guard( const shm_guard& )            = delete;
	shm_guard& operator=( const shm_guard& ) = delete;
	shm_guard( shm_guard&& src );
	shm_guard& operator=( shm_guard&& src );

	void swap( shm_guard& src );

	/**
	 * @brief open a shared memory object and map it to the process's address space.
	 *
	 * @param shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param length shared memory size
	 */
	bool open( const std::string& shm_name, size_t length, mode_t mode );

	/**
	 * @brief create a shared memory object and map it to the process's address space.
	 *
	 * if the shared memory object already exists, it will be truncated to zero size and recreated with the specified size.
	 *
	 * @param shm_name shared memory name. this string should start '/' and shorter than NAME_MAX-4
	 * @param length shared memory size
	 * @param mode access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
	 *
	 * @exception ipsm_mem_error if failed creation by any reason(in case of system call failure)
	 * @exception std::invalid_argument in case of invalid argument
	 */
	void create( const std::string& shm_name, size_t length, mode_t mode );

	void* get( void ) const
	{
		return p_addr_;
	}   //!< get top address of memory area
	size_t mmap_length( void ) const
	{
		return length_;
	}   //!< get mapped length of shared memory area

private:
	int    fd_;
	void*  p_addr_;
	size_t length_;
};

/**
 * @brief A shared memory management class that performs autonomous and distributed construction processing
 */
class ipsm_mem::impl {
public:
	~impl();
	impl(
		const char*                                    p_shm_name,                 //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                                    p_lifetime_ctrl_fname,      //!< [in] lifetime control file name.
		size_t                                         length,                     //!< [in] shared memory size
		mode_t                                         mode,                       //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<std::uintptr_t( void*, size_t )> creater_init_functor_arg,   //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length. return value is hint value for secondary process.
		int                                            timeout_msec,               //!< [in] timeout in milliseconds for waiting for shared memory initialization.
		int                                            retry_interval_msec         //!< [in] retry interval in milliseconds for waiting for shared memory initialization.
	);

	void*  get( void ) const;              //!< get top address of memory area
	size_t available_size( void ) const;   //!< larger than or equal to the size specified in constructor or allocate_shm_as_both.

	ipsm_mem::status get_status( void ) const;
	std::uintptr_t   get_hint_value( void ) const;

private:
	std::string shm_name_;              //!< shared memory name. this string should start '/' and shorter than NAME_MAX-4
	std::string lifetime_ctrl_fname_;   //!< lifetime control file name.
	size_t      req_length_;            //!< requested shared memory size
	mode_t      mode_;                  //!< access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP

	lock_file_guard shared_lock_guard_;   //<! guard for shared lock of lifetime control file
	shm_guard       shm_guard_;           //!< guard for shared memory object
	size_t          shm_length_;          //!< shared memory size. actual size of shared memory area.  req_length_ =< available_length_ < shm_length_
	size_t          available_length_;    //!< available size in shared memory. this size excludes the header area of the shared memory. req_length_ =< available_length_ < shm_length_
};

}   // namespace ipsm_v2

#endif   // IPSM_MEM_V2_INTERNAL_HPP_
