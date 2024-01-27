/**
 * @file procshared_mem_internal.hpp
 * @author PFA03027@nifty.com
 * @brief async open/close shared memory object
 * @version 0.1
 * @date 2023-12-17
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef PROCSHARED_MEM_INTERNAL_HPP_
#define PROCSHARED_MEM_INTERNAL_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>

#include <string.h>

#include <fcntl.h>  /* For O_* constants */
#include <limits.h> /* for NAME_MAX */
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

#include "misc_utility.hpp"
#include "procshared_logger.hpp"
#include "procshared_mem.hpp"

#define TMP_DIR_FOR_ID_FILE "/tmp"

constexpr const char* const p_id_file_base_dir = TMP_DIR_FOR_ID_FILE;

////////////////////////////////////////////////////////////////////////////////////////////////
class id_file_resource_handler {
public:
	id_file_resource_handler( void )
	  : fname_()
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
	}

	~id_file_resource_handler()
	{
		if ( is_valid() ) {
			if ( close( fd_ ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				psm_logoutput( psm_log_lv::kWarn, "Warning: fail to close(%d) of id file %s, %s", fd_, fname_.c_str(), es.c_str() );
			}
		}

		fname_.clear();
		fd_       = -1;
		inode_id_ = 0;
	}

	id_file_resource_handler( id_file_resource_handler&& src )
	  : fname_( std::move( src.fname_ ) )
	  , fd_( src.fd_ )
	  , inode_id_( src.inode_id_ )
	{
		src.fd_       = -1;
		src.inode_id_ = 0;
	}

	id_file_resource_handler& operator=( id_file_resource_handler&& src )
	{
		if ( this == &src ) return *this;

		id_file_resource_handler( std::move( src ) ).swap( *this );

		return *this;
	}

	id_file_resource_handler( const std::string& fname_str, mode_t mode_arg )
	  : fname_( fname_str )
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
		if ( fname_.empty() ) {
			// same as default constructor
			return;
		}

		try_create_or_open( mode_arg );
	}

	id_file_resource_handler( std::string&& fname_str, mode_t mode_arg )
	  : fname_( std::move( fname_str ) )
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
		if ( fname_.empty() ) {
			// same as default constructor
			return;
		}

		try_create_or_open( mode_arg );
	}

	explicit id_file_resource_handler( const std::string& fname_str )
	  : fname_( fname_str )
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
		if ( fname_.empty() ) {
			// same as default constructor
			return;
		}

		try_open();
	}

	explicit id_file_resource_handler( std::string&& fname_str )
	  : fname_( std::move( fname_str ) )
	  , fd_( -1 )
	  , inode_id_( 0 )
	{
		if ( fname_.empty() ) {
			// same as default constructor
			return;
		}

		try_open();
	}

	void do_unlink( void ) noexcept
	{
		if ( fname_.empty() ) {
			psm_logoutput( psm_log_lv::kWarn, "Warning: id file is requested to unlink(), but fname_ is empty" );
			return;
		}
		// psm_logoutput( psm_log_lv::kDebug, "Debug: id file deletion is requested, %s", fname_.c_str() );
		if ( unlink( fname_.c_str() ) != 0 ) {
			auto cur_errno = errno;
			if ( cur_errno == ENOENT ) {
				// psm_logoutput( psm_log_lv::kInfo, "Info: try to unlink(%s) for id file, but it has not existed already.", fname_.c_str() );
			} else {
				auto es = make_strerror( cur_errno );
				psm_logoutput( psm_log_lv::kErr, "Error: fail to unlink(%s), %s", fname_.c_str(), es.c_str() );
			}
		} else {
			// printf( "DEBUG: id file is unlink(%s)\n", fname_.c_str() );
		}
	}

	ino_t get_inode_number( void ) const noexcept
	{
		return inode_id_;
	}

	bool is_valid( void ) const noexcept
	{
		return ( fd_ >= 0 );
	}

	const std::string& name( void ) const noexcept
	{
		return fname_;
	}

	void swap( id_file_resource_handler& b )
	{
		fname_.swap( b.fname_ );
		std::swap( fd_, b.fd_ );
		std::swap( inode_id_, b.inode_id_ );
	}

	void release_resource( void )
	{
		id_file_resource_handler().swap( *this );
	}

private:
	id_file_resource_handler( const id_file_resource_handler& )            = delete;
	id_file_resource_handler& operator=( const id_file_resource_handler& ) = delete;

	void try_create_or_open( mode_t mode_arg )
	{
		fd_ = open( fname_.c_str(), O_CREAT | O_CLOEXEC, mode_arg );
		if ( fd_ < 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			// printf( "Debug: fail to open(%s, O_CREAT | O_CLOEXEC, %x): %s\n", fname_.c_str(), mode_arg, es.c_str() );
			return;
		}
		try {
			inode_id_ = get_inode_of_fd( fd_ );
		} catch ( ... ) {
			// ファイルの作り主なので、ファイルをunlinkする。
			if ( unlink( fname_.c_str() ) != 0 ) {
				type_of_errno cur_errno = errno;
				auto          err_str   = make_strerror( cur_errno );
				psm_logoutput( psm_log_lv::kErr, "Error: fail to unlink(%s): %s", fname_.c_str(), err_str.c_str() );
			}
			if ( close( fd_ ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				psm_logoutput( psm_log_lv::kWarn, "Warning: fail to close(%d) of id file %s, %s", fd_, fname_.c_str(), es.c_str() );
			}
			fd_ = -1;

			throw;   // inode番号が取れないのは致命傷なので、握りつぶさずに例外を再送する。
		}
	}

	void try_open( void )
	{
		fd_ = open( fname_.c_str(), O_CLOEXEC );
		if ( fd_ < 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			// printf( "Debug: fail to open(%s, O_CLOEXEC): %s\n", fname_.c_str(), es.c_str() );
			return;
		}
		try {
			inode_id_ = get_inode_of_fd( fd_ );
		} catch ( ... ) {
			if ( close( fd_ ) != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				psm_logoutput( psm_log_lv::kWarn, "Warning: fail to close(%d) of id file %s, %s", fd_, fname_.c_str(), es.c_str() );
			}
			fd_ = -1;

			throw;   // inode番号が取れないのは致命傷なので、握りつぶさずに例外を再送する。
		}
	}

	std::string fname_;      //!< 監視対象のIDファイルへのファイル名
	int         fd_;         //!< 監視対象のIDファイルへのfd
	ino_t       inode_id_;   //!< 監視対象のIDファイルのinode番号
};

////////////////////////////////////////////////////////////////////////////////////////////////
class shm_resource_handler {
public:
	struct try_create_tag {};
	struct try_open_tag {};

	shm_resource_handler( void )
	  : shm_name_()
	  , shm_fd_( -1 )
	  , length_( 0 )
	  , p_mem_( nullptr )
	{
	}

	~shm_resource_handler()
	{
		// if ( is_valid() ) {
		if ( p_mem_ != nullptr ) {
			int ret = munmap( p_mem_, length_ );
			if ( ret != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				psm_logoutput( psm_log_lv::kErr, "Error: %s by munmap(%p)", es.c_str(), p_mem_ );
			}
		}
		if ( shm_fd_ >= 0 ) {
			int ret = close( shm_fd_ );
			if ( ret != 0 ) {
				auto cur_errno = errno;
				auto es        = make_strerror( cur_errno );
				psm_logoutput( psm_log_lv::kErr, "Error: %s by close(%d)", es.c_str(), shm_fd_ );
			}
		}
		// }

		shm_name_.clear();
		shm_fd_ = -1;
		length_ = 0;
		p_mem_  = nullptr;
	}

	shm_resource_handler( shm_resource_handler&& src )
	  : shm_name_( std::move( src.shm_name_ ) )
	  , shm_fd_( src.shm_fd_ )
	  , length_( src.length_ )
	  , p_mem_( src.p_mem_ )
	{
		src.shm_fd_ = -1;
		src.length_ = 0;
		src.p_mem_  = nullptr;
	}

	shm_resource_handler& operator=( shm_resource_handler&& src )
	{
		if ( this == &src ) return *this;
		shm_resource_handler( std::move( src ) ).swap( *this );
		return *this;
	}

	shm_resource_handler( const try_create_tag, const std::string& shm_name_arg, size_t length_arg, mode_t mode_arg )
	  : shm_name_( shm_name_arg )
	  , shm_fd_( -1 )
	  , length_( 0 )
	  , p_mem_( nullptr )
	{
		try_create( length_arg, mode_arg );
	}

	shm_resource_handler( const try_open_tag, const std::string& shm_name_arg, size_t length_arg, mode_t mode_arg )
	  : shm_name_( shm_name_arg )
	  , shm_fd_( -1 )
	  , length_( 0 )
	  , p_mem_( nullptr )
	{
		try_open( length_arg, mode_arg );
	}

	void do_unlink( void ) noexcept
	{
		if ( shm_name_.empty() ) {
			psm_logoutput( psm_log_lv::kWarn, "Warning: Fail shm_unlink(), shm name is empty" );
			return;
		}
		// psm_logoutput( psm_log_lv::kDebug, "Debug: shared memory deletion is requested, %s", shm_name_.c_str() );
		if ( shm_unlink( shm_name_.c_str() ) != 0 ) {
			auto cur_errno = errno;
			auto es        = make_strerror( cur_errno );
			psm_logoutput( psm_log_lv::kErr, "Error: Fail shm_unlink(%s), %s", shm_name_.c_str(), es.c_str() );
		} else {
			// printf( "DEBUG: shared memory is shm_unlink(%s)\n", shm_name_.c_str() );
		}
	}

	bool is_valid( void ) const noexcept
	{
		return ( shm_fd_ >= 0 ) && ( p_mem_ != nullptr );
	}

	const std::string& name( void ) const noexcept
	{
		return shm_name_;
	}

	void* get_shm_pointer( void ) const noexcept
	{
		return p_mem_;
	}

	int get_fd( void ) const noexcept
	{
		return shm_fd_;
	}

	size_t allocated_size( void ) const noexcept
	{
		return length_;
	}

	void swap( shm_resource_handler& b )
	{
		shm_name_.swap( b.shm_name_ );
		std::swap( shm_fd_, b.shm_fd_ );
		std::swap( length_, b.length_ );
		std::swap( p_mem_, b.p_mem_ );
	}

	void release_resource( void )
	{
		shm_resource_handler().swap( *this );
	}

private:
	shm_resource_handler( const shm_resource_handler& )            = delete;
	shm_resource_handler& operator=( const shm_resource_handler& ) = delete;

	void try_common_create_or_open( int oflags_arg, size_t length_arg, mode_t mode_arg )
	{
		// 共有メモリの作成
		if ( std::numeric_limits<off_t>::max() < length_arg ) {
			psm_logoutput( psm_log_lv::kInfo, "Error: too big memory is required, length_arg=%zu", length_arg );
			return;
		}
		shm_fd_ = shm_open( shm_name_.c_str(), oflags_arg, mode_arg );
		if ( shm_fd_ < 0 ) {
			auto cur_errno = errno;
			auto es2       = make_strerror( cur_errno );
			// psm_logoutput( psm_log_lv::kInfo, "Info: Fail shm_open(%s, %x, %x), %s", shm_name_.c_str(), oflags_arg, mode_arg, es2.c_str() );
			return;
		}
		if ( ftruncate( shm_fd_, static_cast<off_t>( length_arg ) ) != 0 ) {   // TODO: fix this
			auto cur_errno = errno;
			char buff[1024];
			auto es = make_strerror( cur_errno );
			snprintf( buff, 1024, "Fail ftruncate(%d, %ld), %s", shm_fd_, length_arg, es.c_str() );
			std::string err_log = buff;

			if ( shm_unlink( shm_name_.c_str() ) != 0 ) {
				auto cur_errno = errno;
				char buff[1024];
				auto es2 = make_strerror( cur_errno );
				snprintf( buff, 1024, ", then fail shm_unlink(%s), %s", shm_name_.c_str(), es2.c_str() );
				err_log += buff;
			}
			if ( close( shm_fd_ ) != 0 ) {
				auto cur_errno2 = errno;
				char buff[1024];
				auto es2 = make_strerror( cur_errno2 );
				snprintf( buff, 1024, ", then fail close(%d), %s\n", shm_fd_, es2.c_str() );
				err_log += buff;
			}

			shm_fd_ = -1;
			throw procshared_mem_error( err_log.c_str() );
		}
		length_ = length_arg;

		p_mem_ = mmap( NULL, length_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0 );
		if ( p_mem_ == MAP_FAILED ) {
			auto cur_errno = errno;
			char buff[1024];
			auto es2 = make_strerror( cur_errno );
			snprintf( buff, 1024, "Fail mmap(NULL, %ld, PROT_READ | PROT_WRITE, MAP_SHARED, %d, 0), %s", length_, shm_fd_, es2.c_str() );
			std::string err_log = buff;

			if ( shm_unlink( shm_name_.c_str() ) != 0 ) {
				auto cur_errno = errno;
				char buff[1024];
				auto es2 = make_strerror( cur_errno );
				snprintf( buff, 1024, ", then fail shm_unlink(%s), %s", shm_name_.c_str(), es2.c_str() );
				err_log += buff;
			}
			if ( close( shm_fd_ ) != 0 ) {
				auto cur_errno2 = errno;
				char buff[1024];
				auto es2 = make_strerror( cur_errno2 );
				snprintf( buff, 1024, ", then fail close(%d), %s\n", shm_fd_, es2.c_str() );
				err_log += buff;
			}

			shm_fd_ = -1;
			length_ = 0;
			p_mem_  = nullptr;
			throw procshared_mem_error( err_log.c_str() );
		}

		return;
	}

	void try_create( size_t length_arg, mode_t mode_arg )
	{
		try_common_create_or_open( O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, length_arg, mode_arg );
		// psm_logoutput( psm_log_lv::kDebug, "Debug: shared memory creation is requested, %s", shm_name_.c_str() );
	}

	void try_open( size_t length_arg, mode_t mode_arg )
	{
		try_common_create_or_open( O_RDWR | O_CLOEXEC, length_arg, mode_arg );
	}

	std::string shm_name_;   //!< 共有メモリの名称
	int         shm_fd_;     //!< 共有メモリのfd
	size_t      length_;     //!< 共有メモリのサイズ
	void*       p_mem_;      //!< 共有メモリへのポインタ
};

#endif   // PROCSHARED_MEM_INTERNAL_HPP_
