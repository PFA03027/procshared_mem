/**
 * @file procshared_krmalloc.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief process sharable malloc based on K&R malloc algorithm
 * @version 0.1
 * @date 2023-10-07
 *
 * @copyright Copyright (c) 2023, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef PROCSHARED_KRMALLOC_HPP_
#define PROCSHARED_KRMALLOC_HPP_

#include <cstddef>

#include "offset_ptr.hpp"
#include "procshared_mutex.hpp"

class procshared_mem_krmalloc {
public:
	static procshared_mem_krmalloc* make( void* p_mem, size_t mem_bytes );
	static void                     teardown( procshared_mem_krmalloc* p_mem );
	static procshared_mem_krmalloc* bind( void* p_mem );

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	void*
		 allocate( size_t req_bytes, size_t alignment = alignof( std::max_align_t ) );
	void deallocate( void* p, size_t alignment = alignof( std::max_align_t ) );

	static constexpr inline size_t size_of_block_header( void )
	{
		return sizeof( block::block_header );
	}

private:
	procshared_mem_krmalloc( size_t mem_bytes );

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
			// 	fprintf( stderr, "BBBBBBBBBBBooooooooooooooo active_header_.size_of_this_block_= %zd, ans=%zd\n",
			// 	         active_header_.size_of_this_block_,
			// 	         ans );
			// 	ans = 0;
			// }
			// if ( ans != 0 ) {
			// 	fprintf( stderr, "xxxxxxxxxxxxxxx ans=%zd\n", ans );
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

	inline static size_t bytes2blocksize( size_t bytes )
	{
		return ( bytes + sizeof( block::block_header ) - 1 ) / sizeof( block::block_header );
	}

	const size_t      mem_size_;
	procshared_mutex  mtx_;
	offset_ptr<block> op_freep_;
	block             base_blk_;
};

static_assert( std::is_standard_layout<procshared_mem_krmalloc>::value, "procshared_mem_krmalloc should be standard layout" );

#endif   // PROCSHARED_KRMALLOC_HPP_