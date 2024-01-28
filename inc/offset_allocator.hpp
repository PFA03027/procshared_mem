/**
 * @file offset_allocator.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2024-01-04
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef OFFSET_ALLOCATOR_HPP_
#define OFFSET_ALLOCATOR_HPP_

#include <exception>
#include <memory>
#include <type_traits>

#include "offset_malloc.hpp"

namespace ipsm {

template <typename T>
class offset_allocator {
public:
	using value_type                             = T;
	using propagate_on_container_move_assignment = std::false_type;   // メモリ管理領域が違う場合、moveであっても、move先のメモリ管理領域で値が構築されるべき。move/copy先が同じメモリ管理領域の場合に処理を最適化するかどうかは、allocatorを使用する側の都合で決定すべき。
	using propagate_on_container_copy_assignment = std::false_type;   // メモリ管理領域が違う場合、copyであっても、copy先のメモリ管理領域で値が構築されるべき。move/copy先が同じメモリ管理領域の場合に処理を最適化するかどうかは、allocatorを使用する側の都合で決定すべき。
	using size_type                              = size_t;
	using difference_type                        = ptrdiff_t;
	using is_always_equal                        = std::false_type;

	constexpr offset_allocator() noexcept                  = default;
	~offset_allocator()                                    = default;
	offset_allocator( const offset_allocator& )            = default;
	offset_allocator& operator=( const offset_allocator& ) = default;

	explicit offset_allocator( void* p_mem, size_t mem_bytes );   // bind and setup memory allocator implementation. caution: this instance does not become not p_mem area owner.
	explicit offset_allocator( void* p_mem );                     // bind to memory allocator that has already setup. caution: this instance does not become not p_mem area owner.
	explicit offset_allocator( const offset_malloc& src );        // bind to offset_malloc
	explicit offset_allocator( offset_malloc&& src );             // bind to offset_malloc
	template <typename U>
	offset_allocator( const offset_allocator<U>& src );

#if __has_cpp_attribute( nodiscard )
	[[nodiscard]]
#endif
	value_type*
	allocate( size_type n )
	{
		return reinterpret_cast<value_type*>( my_allocator_.allocate( sizeof( value_type[n] ) ) );
	}
	void deallocate( value_type* p, size_type n )
	{
		// TODO: nを使って、解放サイズの不整合をチェックできるようにする？
		my_allocator_.deallocate( p );
	}

	offset_allocator select_on_container_copy_construction( void ) const;

	int get_bind_count( void ) const
	{
		return my_allocator_.get_bind_count();
	}

private:
	offset_allocator( offset_allocator&& )            = delete;   // allocatorの性質上、moveはありえない。
	offset_allocator& operator=( offset_allocator&& ) = delete;   // allocatorの性質上、moveはありえない。

	offset_malloc my_allocator_;

	template <class XT, class XU>
	friend constexpr bool operator==( const offset_allocator<XT>& a, const offset_allocator<XU>& b ) noexcept;
	template <class XT, class XU>
	friend constexpr bool operator!=( const offset_allocator<XT>& a, const offset_allocator<XU>& b ) noexcept;

	template <typename U>
	friend class offset_allocator;
};

template <class T, class U>
constexpr bool operator==( const offset_allocator<T>& a, const offset_allocator<U>& b ) noexcept
{
	return ( a.my_allocator_ == b.my_allocator_ );
}
template <class T, class U>
constexpr bool operator!=( const offset_allocator<T>& a, const offset_allocator<U>& b ) noexcept
{
	return ( a.my_allocator_ != b.my_allocator_ );
}

//////////////////////////////////////////////////////////////////////////////
// implementation

template <typename T>
offset_allocator<T>::offset_allocator( void* p_mem, size_t mem_bytes )
  : my_allocator_( p_mem, mem_bytes )
{
}
template <typename T>
offset_allocator<T>::offset_allocator( void* p_mem )
  : my_allocator_( p_mem )
{
}
template <typename T>
offset_allocator<T>::offset_allocator( const offset_malloc& src )
  : my_allocator_( src )
{
}
template <typename T>
offset_allocator<T>::offset_allocator( offset_malloc&& src )
  : my_allocator_( std::move( src ) )
{
}

template <typename T>
template <typename U>
offset_allocator<T>::offset_allocator( const offset_allocator<U>& src )
  : my_allocator_( src.my_allocator_ )
{
}

template <typename T>
offset_allocator<T> offset_allocator<T>::select_on_container_copy_construction( void ) const
{
	// TODO: 仮実装として、自身のコピーオブジェクトを返す。
	// メンバ変数として保持しているoffset_mallocは、デフォルト構築された場合、メモリ確保できない。
	// そのため、いったん安全のために、offset_allocatorは、この実装を仮実装とする。
	// offset_allocatorの仕様として、デフォルト構築した場合に、通常のヒープからメモリを確保するしようとするかの決断が必要。
	return offset_allocator( *this );   // NOLINT(clang-diagnostic-error)
}

}   // namespace ipsm

#endif   // OFFSET_ALLOCATOR_HPP_
