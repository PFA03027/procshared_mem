/**
 * @file offset_krmalloc.hpp
 * @author PFA03027@nifty.com
 * @brief process sharable malloc based on K&R malloc algorithm
 * @version 0.1
 * @date 2023-10-07
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef OFFSET_MALLOC_IMPL_HPP_
#define OFFSET_MALLOC_IMPL_HPP_

#include <cstddef>

#include "offset_malloc.hpp"
#include "offset_ptr.hpp"
#include "procshared_logger.hpp"
#include "procshared_mutex.hpp"

/**
 * @brief offset_malloc::offset_malloc_impl implementation
 *
 * this memory allocator implemented K&R memory allocation algorithm.
 *
 * this class instance does not become resource owner. caller side of placement_new() should release memory resource.
 *
 * このクラスは、メモリ領域を所有しない設計としているため、placement_new()で指定された領域にクラス構造を構築する。
 */
class offset_malloc::offset_malloc_impl {
public:
	static offset_malloc_impl* placement_new( void* begin_pointer, void* end_pointer );
	static offset_malloc_impl* bind( offset_malloc_impl* p_mem );
	static bool                teardown( offset_malloc_impl* p_mem );

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void*
		 allocate( size_t req_bytes, size_t alignment = alignof( std::max_align_t ) );
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

	int get_bind_count( void ) const;

	inline static constexpr size_t test_block_header_size( void )
	{
		return sizeof( block::block_header );
	}

protected:
private:
	offset_malloc_impl( void* end_pointer );
	~offset_malloc_impl() = default;

	int bind( void );
	int unbind( void );

	struct block {
		struct block_header {
			offset_ptr<block> op_next_block_;        // 次のブロックへのオフセットポインタ
			std::size_t       size_of_this_block_;   // ブロックヘッダとブロック本体を含むこのヘッダが属するブロックのサイズ。1単位は、block_headerのバイトサイズ

			block_header( block* p_next_arg, std::size_t this_block_size );
		};

		block( block* p_next_arg, std::size_t this_block_size );

		block* get_next_ptr( void )
		{
			return active_header_.op_next_block_.get();
		}
		void set_next_ptr( block* p_nxt )
		{
			active_header_.op_next_block_ = p_nxt;
		}

		size_t get_blk_size( void )
		{
			return active_header_.size_of_this_block_;
		}
		void set_blk_size( size_t ns )
		{
			active_header_.size_of_this_block_ = ns;
		}

		void* get_body_ptr( size_t alignment )
		{
			uintptr_t addr_base = reinterpret_cast<uintptr_t>( block_body_ );
			uintptr_t mx        = ( addr_base + alignment - 1 ) / alignment;
			addr_base           = mx * alignment;
			return reinterpret_cast<void*>( addr_base );
		}

		size_t header_slot_optimize( size_t alignment )
		{
			// この最適化は、alignment値が大きくて、allocateサイズが小さい場合に発生しうる。

			size_t ans = 0;

			uintptr_t addr_base = reinterpret_cast<uintptr_t>( block_body_ );
			uintptr_t mx        = ( addr_base + alignment - 1 ) / alignment;
			addr_base           = mx * alignment;

			mx        = ( addr_base + sizeof( block_header ) - 1 ) / sizeof( block_header );
			addr_base = mx * sizeof( block_header );

			uintptr_t diff = addr_base - reinterpret_cast<uintptr_t>( block_body_ );
			ans            = diff / sizeof( block_header );

			// if ( active_header_.size_of_this_block_ <= ( 1 + ans ) ) {
			// 	// 計算結果とsize_of_this_block_との間で整合性が取れていない。
			// 	// 不具合のが発生している可能性があるので、サイズの最適化を行わないような戻り値にする。
			// 	psm_logoutput( psm_log_lv::kErr, "Error: BBBBBBBBBBBooooooooooooooo active_header_.size_of_this_block_= %zd, ans=%zd",
			// 	         active_header_.size_of_this_block_,
			// 	         ans );
			// 	ans = 0;
			// }
			// if ( ans != 0 ) {
			// 	psm_logoutput( psm_log_lv::kErr, "Error: xxxxxxxxxxxxxxx ans=%zd", ans );
			// }

			return ans;
		}

		block* get_end_ptr( void )
		{
			return reinterpret_cast<block*>( &( block_body_[get_blk_size() - 1] ) );
		}

		block_header active_header_;   // ブロックヘッダ
		block_header block_body_[0];   // ブロック本体。ブロックをブロックヘッダー単位で分割管理するので、block_header型の配列としてアクセスできるように定義
	};

	offset_malloc_impl( const offset_malloc_impl& )            = delete;
	offset_malloc_impl( offset_malloc_impl&& )                 = delete;
	offset_malloc_impl& operator=( const offset_malloc_impl& ) = delete;
	offset_malloc_impl& operator=( offset_malloc_impl&& )      = delete;

	static constexpr size_t size_of_block_header( void );
	static constexpr size_t bytes2blocksize( size_t bytes );

	const offset_ptr<unsigned char> op_end_;
	mutable procshared_mutex        mtx_;
	int                             bind_cnt_;
	offset_ptr<block>               op_freep_;
	block                           base_blk_;   //!< bigger address of this member variable is allocation memory area
};

static_assert( std::is_standard_layout<offset_malloc::offset_malloc_impl>::value, "offset_malloc_impl should be standard layout" );

#endif   // OFFSET_MALLOC_IMPL_HPP_