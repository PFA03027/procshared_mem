/**
 * @file offset_krmalloc.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief process sharable malloc based on K&R malloc algorithm
 * @version 0.1
 * @date 2023-10-07
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef OFFSET_KRMALLOC_HPP_
#define OFFSET_KRMALLOC_HPP_

#include <cstddef>

#include "offset_ptr.hpp"
#include "procshared_logger.hpp"
#include "procshared_mutex.hpp"

/**
 * @brief memory allocator that implemented K&R memory allocation algorithm
 *
 * このクラスは、メモリ領域を所有しない設計としているため、placement_new()で指定された領域にクラス構造を構築する。
 */
class offset_mem_krmalloc {
public:
	static offset_mem_krmalloc* placement_new( void* begin_pointer, void* end_pointer );
	static offset_mem_krmalloc* bind( void* p_mem );
	static void                 teardown( offset_mem_krmalloc* p_mem );

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
	offset_mem_krmalloc( void* end_pointer );
	~offset_mem_krmalloc() = default;

private:
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

	offset_mem_krmalloc( const offset_mem_krmalloc& )            = delete;
	offset_mem_krmalloc( offset_mem_krmalloc&& )                 = delete;
	offset_mem_krmalloc& operator=( const offset_mem_krmalloc& ) = delete;
	offset_mem_krmalloc& operator=( offset_mem_krmalloc&& )      = delete;

	int bind( void );
	int unbind( void );

	static constexpr size_t size_of_block_header( void );
	static constexpr size_t bytes2blocksize( size_t bytes );

	const uintptr_t          addr_end_;
	mutable procshared_mutex mtx_;
	int                      bind_cnt_;
	offset_ptr<block>        op_freep_;
	block                    base_blk_;   //!< bigger address of this member variable is allocation memory area
};

static_assert( std::is_standard_layout<offset_mem_krmalloc>::value, "offset_mem_krmalloc should be standard layout" );

#endif   // OFFSET_KRMALLOC_HPP_