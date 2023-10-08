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

	void* allocate( size_t req_bytes );
	void  deallocate( void* p );

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

		void* get_body_ptr( void )
		{
			return reinterpret_cast<void*>( block_body_ );
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
	unsigned char     buff_[0];
};

static_assert( std::is_standard_layout<procshared_mem_krmalloc>::value, "procshared_mem_krmalloc should be standard layout" );

#endif   // PROCSHARED_KRMALLOC_HPP_