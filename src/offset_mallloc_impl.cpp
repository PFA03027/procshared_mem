/**
 * @file ipsm_krmalloc.cpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-10-07
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <cstddef>
#include <mutex>

#include "offset_mallloc_impl.hpp"
#include "offset_malloc.hpp"
#include "offset_ptr.hpp"
#include "ipsm_logger.hpp"

namespace ipsm {

offset_malloc::offset_malloc_impl::block::block_header::block_header( offset_malloc::offset_malloc_impl::block* p_next_arg, std::size_t this_block_size )
  : op_next_block_( p_next_arg )
  , size_of_this_block_( this_block_size )
{
}

offset_malloc::offset_malloc_impl::block::block( offset_malloc::offset_malloc_impl::block* p_next_arg, std::size_t this_block_size )
  : active_header_( p_next_arg, this_block_size )
{
}

constexpr size_t offset_malloc::offset_malloc_impl::size_of_block_header( void )
{
	return sizeof( block::block_header );
}

constexpr size_t offset_malloc::offset_malloc_impl::bytes2blocksize( size_t bytes )
{
	return ( bytes + sizeof( block::block_header ) - 1 ) / sizeof( block::block_header );
}

offset_malloc::offset_malloc_impl::offset_malloc_impl( void* end_pointer )
  : op_end_( reinterpret_cast<unsigned char*>( end_pointer ) )
  , mtx_()
  , bind_cnt_( 0 )
  , op_freep_( nullptr )
  , base_blk_( nullptr, 0 )
{
	uintptr_t addr_end        = reinterpret_cast<uintptr_t>( op_end_.get() );
	uintptr_t addr_buff       = reinterpret_cast<uintptr_t>( base_blk_.block_body_ );
	uintptr_t addr_top        = ( ( addr_buff + size_of_block_header() - 1 ) / size_of_block_header() ) * size_of_block_header();
	uintptr_t addr_buff_start = addr_top + reinterpret_cast<uintptr_t>( sizeof( offset_malloc::offset_malloc_impl ) );

	if ( addr_end <= addr_buff_start ) {
		throw std::bad_alloc();
	}
	uintptr_t buff_lenght = addr_end - addr_buff_start;

	size_t num_of_blocks = buff_lenght / size_of_block_header();
	if ( num_of_blocks < 2 ) {
		throw std::bad_alloc();
	}

	block* p_1st_blk = new ( reinterpret_cast<void*>( addr_top ) ) block( &base_blk_, num_of_blocks - 1 );
	base_blk_.set_next_ptr( p_1st_blk );

	op_freep_ = &base_blk_;

	bind_cnt_ = 1;
}

void* offset_malloc::offset_malloc_impl::allocate( size_t req_bytes, size_t alignment )
{
	const size_t real_alignment  = ( alignment == 0 ) ? 1 : alignment;
	const size_t additional_size = ( alignment <= size_of_block_header() ) ? 0 : ( alignment - size_of_block_header() );

	size_t req_num_of_blocks_w_header = bytes2blocksize( req_bytes + additional_size ) + 1;

	std::lock_guard<ipsm_mutex> lk( mtx_ );

	block* p_end_blk = op_freep_.get();
	block* p_cur_blk = p_end_blk;
	block* p_pre_blk = nullptr;
	block* p_nxt_blk = nullptr;
	do {
		p_nxt_blk = p_cur_blk->get_next_ptr();

		if ( p_cur_blk->active_header_.size_of_this_block_ > ( req_num_of_blocks_w_header + 1 ) ) {
			// 要求ブロック数より大きい空きブロック本体を持つブロックを見つけたので、後ろから切り出す。
			size_t new_block_size = p_cur_blk->active_header_.size_of_this_block_ - req_num_of_blocks_w_header;
			block* p_ans          = reinterpret_cast<block*>( &( p_cur_blk->block_body_[new_block_size - 1] ) );
			// 候補として値を設定し、最適化の余地を確認する。
			// なお、この確認は、deallocate動作で一意にblockの開始位置を決定できることを保証するためでもある。
			p_ans->set_next_ptr( nullptr );
			p_ans->set_blk_size( req_num_of_blocks_w_header );
			size_t opt_val = p_ans->header_slot_optimize( real_alignment );   // 補正量を算出
			req_num_of_blocks_w_header -= opt_val;
			new_block_size = p_cur_blk->active_header_.size_of_this_block_ - req_num_of_blocks_w_header;
			p_ans          = reinterpret_cast<block*>( &( p_cur_blk->block_body_[new_block_size - 1] ) );

			p_cur_blk->set_blk_size( new_block_size );
			op_freep_ = p_cur_blk;

			p_ans->set_next_ptr( nullptr );
			p_ans->set_blk_size( req_num_of_blocks_w_header );
			return p_ans->get_body_ptr( real_alignment );

		} else if ( ( p_pre_blk != nullptr ) && ( p_cur_blk->active_header_.size_of_this_block_ >= req_num_of_blocks_w_header ) ) {
			// 要求ブロック数と同じ大きさの空きブロック本体を持つブロックを見つけたので、このブロックを返す。また、ブロックリストから外す。
			// 要求ブロック数+ 1の空きブロック本体を持つブロックを見つけたので、このブロックを返す。また、ブロックリストから外す。
			// また、候補として値を設定し、最適化の余地を確認する。
			// なお、この確認は、deallocate動作で一意にblockの開始位置を決定できることを保証するためでもある。
			size_t opt_val = p_cur_blk->header_slot_optimize( real_alignment );   // 補正量を算出
			block* p_ans   = nullptr;
			if ( opt_val == 0 ) {
				// 補正は必要ないので、そのままブロックリストから外す
				p_pre_blk->set_next_ptr( p_nxt_blk );

				p_ans = p_cur_blk;
				p_ans->set_next_ptr( nullptr );
			} else {
				// 補正分だけ、返す位置を変える
				p_ans = reinterpret_cast<block*>( &( p_cur_blk->block_body_[opt_val - 1] ) );
				req_num_of_blocks_w_header -= opt_val;
				p_ans->set_next_ptr( nullptr );
				p_ans->set_blk_size( req_num_of_blocks_w_header );

				// 補正分だけ、preのサイズを補正する
				p_pre_blk->set_next_ptr( p_nxt_blk );
				size_t pre_blk_new_size = p_pre_blk->get_blk_size() + opt_val;
				p_pre_blk->set_blk_size( pre_blk_new_size );
			}
			op_freep_ = p_pre_blk;
			return p_ans->get_body_ptr( real_alignment );
		}

		// 次のブロックを探す
		p_pre_blk = p_cur_blk;
		p_cur_blk = p_nxt_blk;
	} while ( p_cur_blk != p_end_blk );

	return nullptr;
}

void offset_malloc::offset_malloc_impl::deallocate( void* p, size_t alignment )
{
	uintptr_t addr_p   = reinterpret_cast<uintptr_t>( p );
	uintptr_t addr_top = reinterpret_cast<uintptr_t>( base_blk_.block_body_ );
	uintptr_t addr_end = reinterpret_cast<uintptr_t>( op_end_.get() );
	if ( addr_p < addr_top ) {
		// out of range
		psm_logoutput( psm_log_lv::kErr, "Error: incorrect deallocation is requested. it is out of range, p_mem=%p, addr_top=%p",
		               p, reinterpret_cast<void*>( addr_top ) );
		return;
	}
	if ( addr_end <= addr_p ) {
		// out of range
		psm_logoutput( psm_log_lv::kErr, "Error: incorrect deallocation is requested. it is out of range, p_mem=%p, addr_end=%p",
		               p, reinterpret_cast<void*>( addr_end ) );
		return;
	}

	uintptr_t    addr_target_blk = ( addr_p / size_of_block_header() - 1 ) * size_of_block_header();
	block* const p_target_blk    = reinterpret_cast<block*>( addr_target_blk );

	std::lock_guard<ipsm_mutex> lk( mtx_ );

	block* p_end_blk = op_freep_.get();
	block* p_pre_blk = p_end_blk;
	block* p_nxt_blk = p_pre_blk->get_next_ptr();
	do {
		if ( ( ( p_pre_blk < p_target_blk ) && ( p_target_blk < p_nxt_blk ) ) || ( ( p_pre_blk < p_target_blk ) && ( p_nxt_blk < p_pre_blk ) ) ) {
			// found
			if ( ( p_pre_blk->get_end_ptr() == p_target_blk ) && ( p_target_blk->get_end_ptr() == p_nxt_blk ) ) {
				// 前後のブロックがともに隣接している場合、前後のブロック含めて結合する。
				p_pre_blk->set_next_ptr( p_nxt_blk->get_next_ptr() );
				size_t pre_new_blk_num = p_pre_blk->get_blk_size() + p_target_blk->get_blk_size() + p_nxt_blk->get_blk_size();
				p_pre_blk->set_blk_size( pre_new_blk_num );
			} else if ( p_pre_blk->get_end_ptr() == p_target_blk ) {
				// 前側だけ隣接している場合
				size_t pre_new_blk_num = p_pre_blk->get_blk_size() + p_target_blk->get_blk_size();
				p_pre_blk->set_blk_size( pre_new_blk_num );
			} else if ( p_target_blk->get_end_ptr() == p_nxt_blk ) {
				// 後側だけ隣接している場合
				p_target_blk->set_next_ptr( p_nxt_blk->get_next_ptr() );
				p_pre_blk->set_next_ptr( p_target_blk );
				size_t pre_new_blk_num = p_target_blk->get_blk_size() + p_nxt_blk->get_blk_size();
				p_target_blk->set_blk_size( pre_new_blk_num );
			} else {
				// 前後、ともに隣接していない場合
				p_target_blk->set_next_ptr( p_nxt_blk );
				p_pre_blk->set_next_ptr( p_target_blk );
			}
			op_freep_ = p_pre_blk;
			return;
		}
		p_pre_blk = p_nxt_blk;
		p_nxt_blk = p_nxt_blk->get_next_ptr();
	} while ( p_pre_blk != p_end_blk );

	if ( ( p_pre_blk == &base_blk_ ) && ( p_nxt_blk == &base_blk_ ) ) {
		// すべてを使い切っていて、かつ、どこかを返却されたパターン
		p_pre_blk->set_next_ptr( p_target_blk );
		p_target_blk->set_next_ptr( p_pre_blk );
		return;
	}

	throw std::logic_error( "fail to free" );
}

int offset_malloc::offset_malloc_impl::bind( void )
{
	std::lock_guard<ipsm_mutex> lk( mtx_ );

	if ( bind_cnt_ <= 0 ) {
		// 0の場合、すでに解放済みの領域を指している。rece conditionと考えられるため、あえて例外を投げる。
		throw std::bad_alloc();
	}

	bind_cnt_++;
	return bind_cnt_;
}
int offset_malloc::offset_malloc_impl::unbind( void )
{
	std::lock_guard<ipsm_mutex> lk( mtx_ );
	if ( bind_cnt_ >= 0 ) {
		bind_cnt_--;
	}
	return bind_cnt_;
}

int offset_malloc::offset_malloc_impl::get_bind_count( void ) const
{
	std::lock_guard<ipsm_mutex> lk( mtx_ );
	return bind_cnt_;
}

bool offset_malloc::offset_malloc_impl::is_belong_to( void* p_mem ) const
{
	uintptr_t addr_p   = reinterpret_cast<uintptr_t>( p_mem );
	uintptr_t addr_top = reinterpret_cast<uintptr_t>( base_blk_.block_body_ );
	uintptr_t addr_end = reinterpret_cast<uintptr_t>( op_end_.get() );
	if ( addr_p < addr_top ) {
		// out of range
		return false;
	}
	if ( addr_end <= addr_p ) {
		// out of range
		return false;
	}

	return true;
}

offset_malloc::offset_malloc_impl* offset_malloc::offset_malloc_impl::placement_new( void* begin_pointer, void* end_pointer )
{
	if ( begin_pointer == nullptr ) {
		throw std::bad_alloc();
	}
	if ( end_pointer == nullptr ) {
		throw std::bad_alloc();
	}
	if ( begin_pointer >= end_pointer ) {
		throw std::bad_alloc();
	}
	if ( ( reinterpret_cast<uintptr_t>( begin_pointer ) + sizeof( offset_malloc::offset_malloc_impl ) ) >= reinterpret_cast<uintptr_t>( end_pointer ) ) {
		throw std::bad_alloc();
	}

	return new ( begin_pointer ) offset_malloc::offset_malloc_impl( end_pointer );
}

offset_malloc::offset_malloc_impl* offset_malloc::offset_malloc_impl::bind( offset_malloc_impl* p_mem )
{
	if ( p_mem == nullptr ) {
		return nullptr;
	}

	p_mem->bind();
	return p_mem;
}

bool offset_malloc::offset_malloc_impl::teardown( offset_malloc::offset_malloc_impl* p_mem )
{
	if ( p_mem == nullptr ) {
		return false;
	}
	int cnt = p_mem->unbind();
	if ( cnt == 0 ) {
		p_mem->~offset_malloc_impl();
		return true;
	} else if ( cnt < 0 ) {
		psm_logoutput( psm_log_lv::kErr, "Error: teardown already, p_mem=%p", p_mem );
	} else {
		// nothing to do
	}
	return false;
}

}   // namespace ipsm
