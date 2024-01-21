/**
 * @file lockfile_mutex.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2024-01-21
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef MUTEX_BY_LOCKFILE_HPP_
#define MUTEX_BY_LOCKFILE_HPP_

#include <string>

// interprocess shared memory
// namespace ipsm {

class lockfile_mutex {
public:
	lockfile_mutex( const char* p_lockfilename );
	~lockfile_mutex();
	lockfile_mutex( lockfile_mutex&& src );
	lockfile_mutex& operator=( lockfile_mutex&& src );

	void lock( void );
	bool try_lock( void );
	void unlock( void );

	void swap( lockfile_mutex& src );

private:
	lockfile_mutex( const lockfile_mutex& )            = delete;
	lockfile_mutex& operator=( const lockfile_mutex& ) = delete;

	bool try_create_lockfile( void );
	void discard_lockfile( void );

	std::string lockfilename_;
	int         lockfilefd_;
};

// }

#endif   // MUTEX_BY_LOCKFILE_HPP_
