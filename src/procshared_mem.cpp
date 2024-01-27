/**
 * @file procshared_mem.cpp
 * @author PFA03027@nifty.com
 * @brief async open/close shared memory object
 * @version 0.1
 * @date 2023-09-03
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <stdexcept>
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

#include "lockfile_mutex.hpp"
#include "misc_utility.hpp"
#include "offset_ptr.hpp"
#include "procshared_logger.hpp"
#include "procshared_mem.hpp"
#include "procshared_mem_internal.hpp"
#include "semaphore_mutex.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////
class procshared_mem_retry : public std::runtime_error {
public:
	procshared_mem_retry( void )
	  : std::runtime_error( "retry procshared_mem_impl construction" )
	{
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Async initialized shared memory
 *
 * @note
 * If you use this class for testing in one process, please intent an instance as primary explicitly.
 * sem_open return a same object of semaphore, even if sem_open is called by a different instance.
 * this will lead unexpected creation/destruction rece condition.
 */
class procshared_mem::impl {
public:
	struct construct_as_primary_tag {};
	struct construct_as_secondary_tag {};

	/**
	 * @brief Construct a new procshared mem object that is empty
	 *
	 */
	impl( void );

	~impl();

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
	impl(
		const char*                           p_shm_name,             //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                           p_id_dirname,           //!< [in] directory name of id file. e.g. "/tmp"
		size_t                                length,                 //!< [in] shared memory size
		mode_t                                mode,                   //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<void*( void*, size_t )> primary_functor_arg,    //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length
		std::function<void( void*, size_t )>  secondary_functor_arg   //!< [in] a functor as secondary role that is initialized by other procshared_mem instance. first argument is the pointer to the top of memory. second argument is the assigned memory length
	);

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
	impl(
		const construct_as_primary_tag,
		const char*                           p_shm_name,           //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                           p_id_dirname,         //!< [in] directory name of id file. e.g. "/tmp"
		size_t                                length,               //!< [in] shared memory size
		mode_t                                mode,                 //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<void*( void*, size_t )> primary_functor_arg   //!< [in] a functor to initialize a shared memory area. first argument is the pointer to the top of memory. second argument is the assigned memory length
	);

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
	impl(
		const construct_as_secondary_tag,
		const char*                          p_shm_name,             //!< [in] shared memory name. this string should start '/' and shorter than NAME_MAX-4
		const char*                          p_id_dirname,           //!< [in] directory name of id file. e.g. "/tmp"
		size_t                               length,                 //!< [in] shared memory size
		mode_t                               mode,                   //!< [in] access mode. e.g. S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
		std::function<void( void*, size_t )> secondary_functor_arg   //!< [in] a functor as secondary role that is initialized by other procshared_mem instance. first argument is the pointer to the top of memory. second argument is the assigned memory length
	);

	void*  get( void ) const;   //!< get top address of memory area
	size_t available_size( void ) const;

	void* get_opt_info( void ) const;
	void  set_opt_info( void* p );

	/**
	 * @brief Set the teardown object
	 *
	 * @li 1st argument of teardown_functor_arg: true, if shared memory is unlinking. false, shared memory itself is still remaining
	 * @li 2nd argument of teardown_functor_arg: pointer to the top of accessible area in shared memory
	 * @li 3rd argument of teardown_functor_arg: size of accessible area in shared memory
	 */
	void set_teardown(
		std::function<void( bool, void*, size_t )> teardown_functor_arg   //!< [in]  a functor that is called when final deletion. this functor is stored in this instance
	);

	int get_bind_count( void ) const;

	ino_t       debug_get_id_file_inode( void ) const;
	bool        debug_test_integrity( void ) const;
	std::string debug_dump_string( void ) const;
	static void debug_force_cleanup( const char* p_shm_name, const char* p_id_dirname );

private:
	struct procshared_mem_mem_header;

	void check_path_name( const char* p_shm_name );
	void setup_as_both( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg,
	                    std::function<void*( void*, size_t )>& primary_functor_arg,
	                    std::function<void( void*, size_t )>&  secondary_functor_arg );
	bool try_setup_as_both(
		int role_type, const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg,
		std::function<void*( void*, size_t )>& primary_functor_arg,
		std::function<void( void*, size_t )>&  secondary_functor_arg );
	void setup_as_primary( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg,
	                       std::function<void*( void*, size_t )>& primary_functor_arg );
	void setup_as_secondary( const char* p_shm_name_arg, const char* p_id_dirname_arg, mode_t mode_arg, size_t length_arg,
	                         std::function<void( void*, size_t )>& secondary_functor_arg );

	static std::string get_id_filename( const char* p_path_name_arg, const char* p_id_dirname_arg );
	static std::string get_mutex_objname( const char* p_path_name_arg, const char* p_id_dirname_arg );
	static size_t      calc_total_neccesary_len( size_t requested_length );
	static size_t      calc_available_size( size_t allocated_shm_length );

	std::string                                mutex_obj_name_;
	id_file_resource_handler                   id_res_;
	shm_resource_handler                       shm_res_;
	std::function<void( bool, void*, size_t )> teardown_functor_;
	procshared_mem_mem_header*                 p_mem_;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct procshared_mem::impl::procshared_mem_mem_header {
	std::atomic<size_t> length_val_;
	std::atomic<int>    reference_count_;
	std::atomic<ino_t>  inode_val_;
	offset_ptr<char>    op_void_;
	unsigned char       shm_buff_[0];

	procshared_mem_mem_header( ino_t inode_v, size_t len_v )
	  : length_val_( len_v )    // constructor of atomic is not atomic operation
	  , reference_count_( 0 )   // constructor of atomic is not atomic operation
	  , inode_val_( inode_v )   // constructor of atomic is not atomic operation
	  , op_void_( nullptr )
	{
		length_val_.store( len_v );
		reference_count_.store( 0 );
		inode_val_.store( inode_v, std::memory_order_release );
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t procshared_mem::impl::calc_total_neccesary_len( size_t requested_length )
{
	size_t total_length = sizeof( procshared_mem_mem_header ) + requested_length;
	size_t page_len     = static_cast<size_t>( sysconf( _SC_PAGE_SIZE ) );
	size_t mx           = total_length / page_len;
	size_t rx           = total_length % page_len;

	return mx * page_len + ( ( rx == 0 ) ? 0 : page_len );
}

void procshared_mem::impl::check_path_name( const char* p_shm_name )
{
	if ( p_shm_name == nullptr ) {
		throw procshared_mem_error( "p_shm_name is nullptr" );
	}
	if ( p_shm_name[0] != '/' ) {
		std::string error_str = "first charactor of p_shm_name is /. actual=";
		error_str += std::string( p_shm_name );
		throw procshared_mem_error( error_str );
	}
	size_t namelen = strlen( p_shm_name );
	if ( namelen >= ( NAME_MAX - 4 ) ) {
		std::string error_str = "p_shm_name is too long. it should be shorter than NAME_MAX-4. actual=";
		error_str += std::to_string( namelen );
		throw procshared_mem_error( error_str );
	}
}

std::string procshared_mem::impl::get_id_filename( const char* p_path_name_arg, const char* p_id_dirname_arg )
{
	if ( p_path_name_arg == nullptr ) {
		throw procshared_mem_error( "p_path_name_arg is nullptr" );
	}
	std::string ans( ( p_id_dirname_arg != nullptr ) ? p_id_dirname_arg : TMP_DIR_FOR_ID_FILE );
	return ans + std::string( p_path_name_arg );
}

std::string procshared_mem::impl::get_mutex_objname( const char* p_path_name_arg, const char* p_id_dirname_arg )
{
	if ( p_path_name_arg == nullptr ) {
		throw procshared_mem_error( "p_path_name_arg is nullptr" );
	}
	std::string ans( ( p_id_dirname_arg != nullptr ) ? p_id_dirname_arg : TMP_DIR_FOR_ID_FILE );
	return ans + std::string( p_path_name_arg ) + std::string( ".lock" );
}

size_t procshared_mem::impl::calc_available_size( size_t allocated_shm_length )
{
	return allocated_shm_length - sizeof( procshared_mem_mem_header );
}

procshared_mem::impl::impl( void )
  : mutex_obj_name_()
  , id_res_()
  , shm_res_()
  , teardown_functor_()
  , p_mem_( nullptr )
{
}

procshared_mem::impl::~impl()
{
	if ( p_mem_ == nullptr ) return;

	auto orig_final_ref_c = p_mem_->reference_count_.load();
	try {
		lockfile_mutex                  lf_mtx( mutex_obj_name_.c_str() );   // inter-process mutex via filesystem
		std::lock_guard<lockfile_mutex> lk( lf_mtx );

		auto                       final_ref_c = p_mem_->reference_count_.fetch_sub( 1 ) - 1;
		procshared_mem_mem_header* p_cur_mem   = reinterpret_cast<procshared_mem_mem_header*>( shm_res_.get_shm_pointer() );
		if ( teardown_functor_ ) {
			// teardown_functor_が呼び出し可能な場合に呼び出しを行う。
			teardown_functor_( final_ref_c == 0, p_cur_mem->shm_buff_, calc_available_size( shm_res_.allocated_size() ) );
		}
		if ( final_ref_c == 0 ) {
			p_cur_mem = reinterpret_cast<procshared_mem_mem_header*>( shm_res_.get_shm_pointer() );
			p_cur_mem->inode_val_.store( 0, std::memory_order_release );   // 保存しているinode番号を無効値に書き換える。

			id_res_.do_unlink();
			shm_res_.do_unlink();
		}
	} catch ( std::runtime_error& e ) {
		// 握りつぶす
		psm_logoutput( psm_log_lv::kErr, "Error: procshared_mem::impl(ref count %d) destructor catch std::runtime_error exception: %s", orig_final_ref_c, e.what() );
	} catch ( ... ) {
		// 握りつぶす
		psm_logoutput( psm_log_lv::kErr, "Error: procshared_mem::impl destructor catch unknown exception" );
	}
}

procshared_mem::impl::impl(
	const char*                           p_shm_name,
	const char*                           p_id_dirname,
	size_t                                length,
	mode_t                                mode,
	std::function<void*( void*, size_t )> primary_functor_arg,
	std::function<void( void*, size_t )>  secondary_functor_arg )
  : mutex_obj_name_()
  , id_res_()
  , shm_res_()
  , teardown_functor_()
  , p_mem_( nullptr )
{
	check_path_name( p_shm_name );
	mutex_obj_name_       = get_mutex_objname( p_shm_name, p_id_dirname );
	size_t nessesary_size = calc_total_neccesary_len( length );
	setup_as_both( p_shm_name, p_id_dirname, mode, nessesary_size, primary_functor_arg, secondary_functor_arg );
}

procshared_mem::impl::impl(
	const construct_as_primary_tag,
	const char*                           p_shm_name,
	const char*                           p_id_dirname,
	size_t                                length,
	mode_t                                mode,
	std::function<void*( void*, size_t )> primary_functor_arg )
  : mutex_obj_name_()
  , id_res_()
  , shm_res_()
  , teardown_functor_()
  , p_mem_( nullptr )
{
	check_path_name( p_shm_name );
	mutex_obj_name_       = get_mutex_objname( p_shm_name, p_id_dirname );
	size_t nessesary_size = calc_total_neccesary_len( length );
	setup_as_primary( p_shm_name, p_id_dirname, mode, nessesary_size, primary_functor_arg );
}

procshared_mem::impl::impl(
	const construct_as_secondary_tag,
	const char*                          p_shm_name,
	const char*                          p_id_dirname,
	size_t                               length,
	mode_t                               mode,
	std::function<void( void*, size_t )> secondary_functor_arg )
  : mutex_obj_name_()
  , id_res_()
  , shm_res_()
  , teardown_functor_()
  , p_mem_( nullptr )
{
	check_path_name( p_shm_name );
	mutex_obj_name_       = get_mutex_objname( p_shm_name, p_id_dirname );
	size_t nessesary_size = calc_total_neccesary_len( length );
	setup_as_secondary( p_shm_name, p_id_dirname, mode, nessesary_size, secondary_functor_arg );
}

void procshared_mem::impl::setup_as_both(
	const char* p_shm_name_arg,
	const char* p_id_dirname_arg,
	mode_t mode_arg, size_t length_arg,
	std::function<void*( void*, size_t )>& primary_functor_arg,
	std::function<void( void*, size_t )>&  secondary_functor_arg )
{
	while ( !try_setup_as_both( 0, p_shm_name_arg, p_id_dirname_arg, mode_arg, length_arg, primary_functor_arg, secondary_functor_arg ) ) {
		psm_logoutput( psm_log_lv::kDebug, "Debug: retry shared memory setup" );
		std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
	}
}

void procshared_mem::impl::setup_as_primary(
	const char*                            p_shm_name_arg,
	const char*                            p_id_dirname_arg,
	mode_t                                 mode_arg,
	size_t                                 length_arg,
	std::function<void*( void*, size_t )>& primary_functor_arg )
{
	std::function<void( void*, size_t )> dummy_functor = []( void*, size_t ) {};
	try_setup_as_both( 1, p_shm_name_arg, p_id_dirname_arg, mode_arg, length_arg, primary_functor_arg, dummy_functor );
}

void procshared_mem::impl::setup_as_secondary(
	const char*                           p_shm_name_arg,
	const char*                           p_id_dirname_arg,
	mode_t                                mode_arg,
	size_t                                length_arg,
	std::function<void( void*, size_t )>& secondary_functor_arg )
{
	std::function<void*( void*, size_t )> dummy_functor = []( void*, size_t ) { return nullptr; };
	try_setup_as_both( 2, p_shm_name_arg, p_id_dirname_arg, mode_arg, length_arg, dummy_functor, secondary_functor_arg );
}

bool procshared_mem::impl::try_setup_as_both(
	int                                    role_type,
	const char*                            p_shm_name_arg,
	const char*                            p_id_dirname_arg,
	mode_t                                 mode_arg,
	size_t                                 length_arg,
	std::function<void*( void*, size_t )>& primary_functor_arg,
	std::function<void( void*, size_t )>&  secondary_functor_arg )
{
	std::string              id_fname = get_id_filename( p_shm_name_arg, p_id_dirname_arg );
	id_file_resource_handler cur_id_res( id_fname, mode_arg );
	if ( !cur_id_res.is_valid() ) {
		throw procshared_mem_error( "fail top open id file" );
	}

	lockfile_mutex                  lf_mtx( mutex_obj_name_.c_str() );   // inter-process mutex via filesystem
	std::lock_guard<lockfile_mutex> lk( lf_mtx );

	id_file_resource_handler tmp_id_file = id_file_resource_handler( id_fname );
	if ( !tmp_id_file.is_valid() ) {
		psm_logoutput( psm_log_lv::kDebug, "Debug: ID file fail to open, try again" );
		return false;
	}
	if ( cur_id_res.get_inode_number() != tmp_id_file.get_inode_number() ) {
		psm_logoutput( psm_log_lv::kDebug, "Debug: ID file inode is mismatch, try again" );
		return false;
	}
	// unlink済みであっても、ここを抜けてくる場合がある。

	shm_resource_handler       cur_shm_res;
	procshared_mem_mem_header* p_cur_mem;

	bool is_primary = true;
	switch ( role_type ) {
		case 0: /* both role */ {
			// try as primary
			cur_shm_res = shm_resource_handler( shm_resource_handler::try_create_tag(), p_shm_name_arg, length_arg, mode_arg );
			if ( cur_shm_res.is_valid() ) {
				is_primary = true;
				psm_logoutput( psm_log_lv::kDebug, "Debug: shared memory open as primary, role_type=%d", role_type );
			} else {
				// try as secondary
				cur_shm_res = shm_resource_handler( shm_resource_handler::try_open_tag(), p_shm_name_arg, length_arg, mode_arg );
				if ( !cur_shm_res.is_valid() ) {
					psm_logoutput( psm_log_lv::kInfo, "Info: shared memory open fail, role_type=%d", role_type );
					return false;
				}
				is_primary = false;
				psm_logoutput( psm_log_lv::kDebug, "Debug: shared memory open as secondary, role_type=%d", role_type );
			}
		} break;

		case 1: /* primary */ {
			cur_shm_res = shm_resource_handler( shm_resource_handler::try_create_tag(), p_shm_name_arg, length_arg, mode_arg );
			if ( !cur_shm_res.is_valid() ) {
				psm_logoutput( psm_log_lv::kWarn, "Warning: Fail shared memory open(%s) as primary, role_type=%d", p_shm_name_arg, role_type );
				return false;
			}
			is_primary = true;
		} break;

		case 2: /* secondary */ {
			cur_shm_res = shm_resource_handler( shm_resource_handler::try_open_tag(), p_shm_name_arg, length_arg, mode_arg );
			if ( !cur_shm_res.is_valid() ) {
				psm_logoutput( psm_log_lv::kWarn, "Warning: Fail shared memory open(%s) as secondary, role_type=%d", p_shm_name_arg, role_type );
				return false;
			}
			is_primary = false;
		} break;

		default: {
			// this is logic error
			throw procshared_mem_error( "role type error" );
		} break;
	}

	if ( is_primary ) {
		// primary
		p_cur_mem = new ( cur_shm_res.get_shm_pointer() ) procshared_mem_mem_header( cur_id_res.get_inode_number(), length_arg );

		void* p_tmp         = primary_functor_arg( p_cur_mem->shm_buff_, calc_available_size( cur_shm_res.allocated_size() ) );
		p_cur_mem->op_void_ = reinterpret_cast<char*>( p_tmp );
	} else {
		// secondary
		p_cur_mem                       = reinterpret_cast<procshared_mem_mem_header*>( cur_shm_res.get_shm_pointer() );
		auto cur_inode_value_in_cur_mem = p_cur_mem->inode_val_.load( std::memory_order_acquire );
		if ( cur_id_res.get_inode_number() != cur_inode_value_in_cur_mem ) {
			// 共有メモリ削除時にidファイルチェックを抜けてきた場合、最後は、ここで、inode番号がクリアされていることによる不一致で、不整合を検出する。
			psm_logoutput( psm_log_lv::kInfo, "Info: inode number mis-match, cur_id_res %lu, inode_val_ %lu", cur_id_res.get_inode_number(), cur_inode_value_in_cur_mem );
			return false;
		}
		if ( cur_inode_value_in_cur_mem < 1 ) {
			// 共有メモリ削除時にidファイルチェックを抜けてきた場合、最後は、ここで、inode番号がクリアされていることによる不一致で、不整合を検出する。
			psm_logoutput( psm_log_lv::kInfo, "Info: inode number is out of range, inode_val_ %lu", cur_inode_value_in_cur_mem );
			return false;
		}

		secondary_functor_arg( p_cur_mem->shm_buff_, calc_available_size( cur_shm_res.allocated_size() ) );
	}

	// 参照カウント+1
	p_cur_mem->reference_count_.fetch_add( 1 );

	// メンバ変数へ設定。
	id_res_  = std::move( cur_id_res );
	shm_res_ = std::move( cur_shm_res );
	p_mem_   = p_cur_mem;

	return true;   // セマフォポスト。そのあと、セマフォクローズ
}

void* procshared_mem::impl::get( void ) const
{
	return reinterpret_cast<void*>( p_mem_->shm_buff_ );
}

size_t procshared_mem::impl::available_size( void ) const
{
	if ( !shm_res_.is_valid() ) return 0;

	return calc_available_size( shm_res_.allocated_size() );
}

void* procshared_mem::impl::get_opt_info( void ) const
{
	return reinterpret_cast<void*>( p_mem_->op_void_.get() );
}

void procshared_mem::impl::set_opt_info( void* p )
{
	p_mem_->op_void_ = reinterpret_cast<char*>( p );
}

void procshared_mem::impl::set_teardown(
	std::function<void( bool, void*, size_t )> teardown_functor_arg   //!< [in]  a functor that is called when final deletion. this functor is stored in this instance
)
{
	teardown_functor_ = teardown_functor_arg;
}

int procshared_mem::impl::get_bind_count( void ) const
{
	return p_mem_->reference_count_.load( std::memory_order_acquire );
}

ino_t procshared_mem::impl::debug_get_id_file_inode( void ) const
{
	return reinterpret_cast<procshared_mem_mem_header*>( p_mem_ )->inode_val_.load( std::memory_order_acquire );
}

bool procshared_mem::impl::debug_test_integrity( void ) const
{
	bool ans = false;
	try {
		// printf( "inode_val: %ld, id_file: %ld\n", debug_get_id_file_inode(), id_res_.get_inode_number() );
		ans = ( debug_get_id_file_inode() == id_res_.get_inode_number() );

	} catch ( procshared_mem_error& e ) {
		ans = false;
	}

	return ans;
}

std::string procshared_mem::impl::debug_dump_string( void ) const
{
	char buff[2048];

	snprintf( buff, 2048,
	          "p_mem_      = %p\n",
	          p_mem_ );

	std::string ans( buff );
	return ans;
}

void procshared_mem::impl::debug_force_cleanup( const char* p_shm_name, const char* p_id_dirname )
{
	std::string id_fname = get_id_filename( p_shm_name, p_id_dirname );
	int         ret      = unlink( id_fname.c_str() );
	if ( ret != 0 ) {
		auto cur_errno = errno;
		auto es        = make_strerror( cur_errno );
		printf( "%s by unlink(%s)\n", es.c_str(), id_fname.c_str() );
	} else {
		printf( "success to unlink id file: %s\n", id_fname.c_str() );
	}

	ret = shm_unlink( p_shm_name );   // 共有メモリオブジェクトの削除
	if ( ret != 0 ) {
		auto cur_errno = errno;
		auto es        = make_strerror( cur_errno );
		printf( "%s by shm_unlink(%s)\n", es.c_str(), p_shm_name );
	} else {
		printf( "success to unlink shared memory: %s\n", p_shm_name );
	}

	std::string lock_fname = get_mutex_objname( p_shm_name, p_id_dirname );
	lockfile_mutex::debug_force_cleanup( lock_fname.c_str() );

#if 0
	ret = sem_unlink( p_shm_name );   // セマフォの削除
	if ( ret != 0 ) {
		auto cur_errno = errno;
		auto es        = make_strerror( cur_errno );
		printf( "%s by sem_unlink(%s)\n", es.c_str(), p_shm_name );
	} else {
		printf( "success to unlink semaphore: %s\n", p_shm_name );
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////
procshared_mem::procshared_mem( void )
  : p_impl_( nullptr )
{
}

procshared_mem::~procshared_mem()
{
	delete p_impl_;
}

procshared_mem::procshared_mem( procshared_mem&& src )
  : p_impl_( src.p_impl_ )
{
	src.p_impl_ = nullptr;
}

procshared_mem& procshared_mem::operator=( procshared_mem&& src )
{
	if ( this == &src ) return *this;

	delete p_impl_;
	p_impl_     = src.p_impl_;
	src.p_impl_ = nullptr;

	return *this;
}

procshared_mem::procshared_mem(
	const char*                           p_shm_name,
	const char*                           p_id_dirname,
	size_t                                length,
	mode_t                                mode,
	std::function<void*( void*, size_t )> primary_functor_arg,
	std::function<void( void*, size_t )>  secondary_functor_arg )
  : p_impl_( nullptr )
{
	p_impl_ = new impl( p_shm_name, p_id_dirname, length, mode, primary_functor_arg, secondary_functor_arg );
}

void procshared_mem::allocate_shm_as_both(
	const char*                           p_shm_name,
	const char*                           p_id_dirname,
	size_t                                length,
	mode_t                                mode,
	std::function<void*( void*, size_t )> primary_functor_arg,
	std::function<void( void*, size_t )>  secondary_functor_arg )
{
	delete p_impl_;
	p_impl_ = new impl( p_shm_name, p_id_dirname, length, mode, primary_functor_arg, secondary_functor_arg );
}

void procshared_mem::allocate_shm_as_primary(
	const char*                           p_shm_name,
	const char*                           p_id_dirname,
	size_t                                length,
	mode_t                                mode,
	std::function<void*( void*, size_t )> primary_functor_arg )
{
	delete p_impl_;
	p_impl_ = new impl( procshared_mem::impl::construct_as_primary_tag(), p_shm_name, p_id_dirname, length, mode, primary_functor_arg );
}

void procshared_mem::allocate_shm_as_secondary(
	const char*                          p_shm_name,
	const char*                          p_id_dirname,
	size_t                               length,
	mode_t                               mode,
	std::function<void( void*, size_t )> secondary_functor_arg )
{
	delete p_impl_;
	p_impl_ = new impl( procshared_mem::impl::construct_as_secondary_tag(), p_shm_name, p_id_dirname, length, mode, secondary_functor_arg );
}

size_t procshared_mem::available_size( void ) const
{
	if ( p_impl_ == nullptr ) return 0;

	return p_impl_->available_size();
}

void* procshared_mem::get( void )
{
	if ( p_impl_ == nullptr ) return nullptr;

	return p_impl_->get();
}

void* procshared_mem::get_opt_info( void ) const
{
	if ( p_impl_ == nullptr ) return nullptr;

	return p_impl_->get_opt_info();
}
void procshared_mem::set_opt_info( void* p )
{
	if ( p_impl_ == nullptr ) return;

	return p_impl_->set_opt_info( p );
}

void procshared_mem::swap( procshared_mem& src )
{
	auto p_tmp  = p_impl_;
	p_impl_     = src.p_impl_;
	src.p_impl_ = p_tmp;
}

void procshared_mem::set_teardown(
	std::function<void( bool, void*, size_t )> teardown_functor_arg   //!< [in]  a functor that is called when final deletion. this functor is stored in this instance
)
{
	if ( p_impl_ == nullptr ) return;

	p_impl_->set_teardown( teardown_functor_arg );
}

int procshared_mem::get_bind_count( void ) const
{
	if ( p_impl_ == nullptr ) return 0;

	return p_impl_->get_bind_count();
}

ino_t procshared_mem::debug_get_id_file_inode( void ) const
{
	if ( p_impl_ == nullptr ) return 0;

	return p_impl_->debug_get_id_file_inode();
}
bool procshared_mem::debug_test_integrity( void ) const
{
	if ( p_impl_ == nullptr ) return false;

	return p_impl_->debug_test_integrity();
}
std::string procshared_mem::debug_dump_string( void ) const
{
	if ( p_impl_ == nullptr ) return std::string();

	return p_impl_->debug_dump_string();
}
void procshared_mem::debug_force_cleanup( const char* p_shm_name, const char* p_id_dirname )
{
	procshared_mem::impl::debug_force_cleanup( p_shm_name, p_id_dirname );
}
