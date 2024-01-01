/**
 * @file offset_shared_ptr.hpp
 * @author PFA03027@nifty.com
 * @brief Shared pointer based on offset_ptr
 * @version 0.1
 * @date 2023-10-20
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#include <memory>
#include <mutex>

#include "offset_shared_ptr.hpp"

namespace offset_shared_ptr_detail {

void offset_shared_ptr_impl_if::try_dispose_com( offset_ptr<offset_shared_ptr_impl_if>& p, long offset_shared_ptr_impl_if::ctrl_data::*mp_tcnt )
{
	if ( p == nullptr ) return;

	auto ac = p->get_ascer();
	( ac.ref().*mp_tcnt )--;
	if ( ( ac.ref().shrd_refc_ <= 0 ) && ( ac.ref().weak_refc_ <= 0 ) ) {
		alloc_if_abst* p_alloc_if = p->get_alloc_if();
		// 内部でnew式を使っているので、std::bad_allocがスローされる可能性がある。結果、noexceptとならない。
		// 今のところ、対策がないので、例外をキャッチした場合、メモリ構造を破壊するぐらいなら、解放を諦める。
		// ただ、キャッチするとしても、そのあとの処理がわからないので、とりあえず、放置してみる。。。std::terminateに以降するはず。たぶん。
		{
			offset_shared_ptr_impl_if::ascer tmp( std::move( ac ) );
			// 以降で、mutexのメモリ領域を解放するので、解放前に、ロックを解除し、acserとのコネクションを切る。
		}
		p_alloc_if->call_destroy_deallocate( p.get() );
		delete p_alloc_if;
	} else if ( ( ac.ref().shrd_refc_ <= 0 ) && ( ac.ref().weak_refc_ > 0 ) ) {
		p->dispose_resource();
	}

	return;
}

}   // namespace offset_shared_ptr_detail
