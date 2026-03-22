/**
 * @file offset_string.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief string class based on offset_ptr
 * @version 0.1
 * @date 2024-02-05
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef OFFSET_STRING_HPP_
#define OFFSET_STRING_HPP_

#include <cstring>
#include <memory>   // std::allocator_traits<>
#include <string>   // std::char_traits<>

#include "offset_allocator.hpp"
#include "offset_ptr.hpp"
#include "offset_unique_ptr.hpp"

namespace ipsm {

/**
 * @brief Offset pointer base string class which use Allocator
 *
 * @tparam charT charactor type
 * @tparam traits traits of charactor type
 * @tparam Allocator Allocator to allocate memory of string
 */
template <typename charT,
          class traits    = std::char_traits<charT>,
          class Allocator = std::allocator<charT>>
class offset_basic_string {
public:
	using traits_type     = traits;
	using value_type      = typename traits::char_type;
	using allocator_type  = typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;
	using size_type       = typename std::allocator_traits<allocator_type>::size_type;
	using difference_type = typename std::allocator_traits<allocator_type>::difference_type;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using pointer         = typename std::allocator_traits<allocator_type>::pointer;

	~offset_basic_string();
	offset_basic_string( void ) noexcept( noexcept( Allocator() ) )
	  : offset_basic_string( Allocator() )
	{
	}
	offset_basic_string( const offset_basic_string& str );
	offset_basic_string( offset_basic_string&& str ) noexcept;
	offset_basic_string& operator=( const offset_basic_string& str );
	offset_basic_string& operator=( offset_basic_string&& str );

	void swap( offset_basic_string& str );

	explicit offset_basic_string( const Allocator& a ) noexcept
	  : alloc_( a )
	  , oup_outer_buffer_( nullptr )
	  , outer_buffer_array_size_( 0 )
	  , op_string_( nullptr )
	  , soo_buff_ { 0 }
	{
		op_string_ = soo_buff_;
	}
	offset_basic_string( const offset_basic_string& str, const Allocator& a );
	offset_basic_string( offset_basic_string&& str, const Allocator& a );
	offset_basic_string( const charT* s, const Allocator& a = Allocator() );

	const pointer c_str( void ) const noexcept
	{
		return op_string_;
	}
	const pointer data( void ) const noexcept
	{
		return op_string_;
	}

	size_t size( void ) const
	{
		value_type* p   = op_string_.get();
		size_t      ans = 0;
		while ( *p != 0 ) {
			ans++;
		}
		return ans;
	}

	void clear( void )
	{
		soo_buff_[0] = 0;
		op_string_   = soo_buff_;
		deallocate_outer_buffer();
	}

	bool empty( void ) const
	{
		return ( *op_string_ ) == 0;
	}

	allocator_type get_allocator() const noexcept
	{
		return alloc_;
	}

private:
	using allocator_traits = typename std::allocator_traits<allocator_type>::template rebind_traits<value_type>;

	static constexpr size_t soo_buff_size_ = sizeof( std::basic_string<value_type> ) / sizeof( value_type ) + 1;

	void allocate_outer_buffer( size_t n );
	void deallocate_outer_buffer( void );

	allocator_type         alloc_;
	offset_ptr<value_type> oup_outer_buffer_;
	size_t                 outer_buffer_array_size_;
	offset_ptr<value_type> op_string_;
	value_type             soo_buff_[soo_buff_size_];   //!< small object optimization buffer
};

using offset_string = offset_basic_string<char>;

///////////////////////////////////////////////////////////////////////////////////////////////
/// implementation

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>::~offset_basic_string()
{
	deallocate_outer_buffer();
}

template <typename charT, class traits, class Allocator>
void offset_basic_string<charT, traits, Allocator>::allocate_outer_buffer( size_t n )
{
	if ( ( oup_outer_buffer_ != nullptr ) && ( outer_buffer_array_size_ >= n ) ) {
		return;   // 既存のバッファが十分なサイズを持つ場合、領域を流用するため、領域確保、初期化不要。
	}

	if ( oup_outer_buffer_ != nullptr ) {
		deallocate_outer_buffer();   // 既存バッファの破棄
	}

	value_type* p = nullptr;

	// 新しいバッファの確保
	p = allocator_traits::allocate( alloc_, n );
	if ( p == nullptr ) {
		return;
	}

	// 新しいバッファの初期化
	size_t i = 0;
	try {
		for ( ; i < n; i++ ) {
			allocator_traits::construct( alloc_, &( p[i] ) );
		}
	} catch ( ... ) {
		if ( i > 0 ) {
			for ( size_t j = i - 1; i > 0; i-- ) {
				allocator_traits::destroy( alloc_, &( p[j] ) );
			}
			allocator_traits::destroy( alloc_, &( p[0] ) );
		}
		allocator_traits::deallocate( alloc_, n );
		throw;
	}

	oup_outer_buffer_        = p;
	outer_buffer_array_size_ = n;
}

template <typename charT, class traits, class Allocator>
void offset_basic_string<charT, traits, Allocator>::deallocate_outer_buffer( void )
{
	if ( oup_outer_buffer_ == nullptr ) {
		return;
	}

	value_type* const p_top = oup_outer_buffer_.get();
	for ( size_t i = 0; i < outer_buffer_array_size_; i++ ) {
		value_type* p = &( p_top[i] );
		allocator_traits::destroy( alloc_, p );
	}
	allocator_traits::deallocate( alloc_, oup_outer_buffer_.get(), outer_buffer_array_size_ );

	oup_outer_buffer_        = nullptr;
	outer_buffer_array_size_ = 0;
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>::offset_basic_string( const offset_basic_string& str )
  : offset_basic_string( str, allocator_traits::select_on_container_copy_construction( str.alloc_ ) )
{
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>::offset_basic_string( const offset_basic_string& str, const Allocator& a )
  : offset_basic_string( a )
{
	if ( str.oup_outer_buffer_ == nullptr ) {
		op_string_ = soo_buff_;
		std::memcpy( soo_buff_, str.soo_buff_, soo_buff_size_ );
	} else {
		allocate_outer_buffer( str.outer_buffer_array_size_ );
		if ( oup_outer_buffer_ == nullptr ) {
			throw std::bad_alloc();
		}
		std::memcpy( oup_outer_buffer_.get(), str.oup_outer_buffer_.get(), str.outer_buffer_array_size_ );
		op_string_ = oup_outer_buffer_;
	}
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>::offset_basic_string( offset_basic_string&& str ) noexcept
  : offset_basic_string( str.alloc_ )
{
	if ( str.oup_outer_buffer_ == nullptr ) {
		std::memcpy( soo_buff_, str.soo_buff_, soo_buff_size_ );
		op_string_ = soo_buff_;

		str.soo_buff_[0] = 0;
		str.op_string_   = str.soo_buff_;
	} else {
		oup_outer_buffer_        = str.oup_outer_buffer_;
		outer_buffer_array_size_ = str.outer_buffer_array_size_;
		op_string_               = oup_outer_buffer_;

		str.oup_outer_buffer_        = nullptr;
		str.outer_buffer_array_size_ = 0;
		str.op_string_               = str.soo_buff_;
		str.soo_buff_[0]             = 0;
	}
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>::offset_basic_string( const charT* s, const Allocator& a )
  : offset_basic_string( a )
{
	size_t num_chars = 0;
	charT* p         = s;
	while ( *p != 0 ) {
		num_chars++;
	}
	num_chars++;   // null文字分を追加

	const size_t copy_bytes = sizeof( charT[num_chars] );
	if ( num_chars <= soo_buff_size_ ) {
		std::memcpy( soo_buff_, s, copy_bytes );
	} else {
		allocate_outer_buffer( num_chars );
		if ( oup_outer_buffer_ == nullptr ) {
			throw std::bad_alloc();
		}
		std::memcpy( oup_outer_buffer_.get(), s, copy_bytes );
		op_string_ = oup_outer_buffer_;
	}
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>& offset_basic_string<charT, traits, Allocator>::operator=( const offset_basic_string& str )
{
	if ( this == &str ) {
		return *this;
	}

	if constexpr ( allocator_traits::propagate_on_container_copy_assignment::value ) {
		// コピー代入で、アロケータをコピー伝搬させる場合
		deallocate_outer_buffer();
		alloc_ = str.alloc_;
	}

	if ( str.oup_outer_buffer_ == nullptr ) {
		deallocate_outer_buffer();
		std::memcpy( soo_buff_, str.soo_buff_, soo_buff_size_ );
		op_string_ = soo_buff_;
	} else {
		allocate_outer_buffer( str.outer_buffer_array_size_ );
		const size_t copy_bytes = sizeof( charT[str.outer_buffer_array_size_] );
		std::memcpy( oup_outer_buffer_.get(), str.oup_outer_buffer_.get(), copy_bytes );
		op_string_ = oup_outer_buffer_;
	}

	return *this;
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>& offset_basic_string<charT, traits, Allocator>::operator=( offset_basic_string&& str )
{
	if ( this == &str ) {
		return *this;
	}

	if constexpr ( allocator_traits::propagate_on_container_move_assignment::value ) {
		// ムーブ代入で、アロケータをコピー伝搬させる場合
		deallocate_outer_buffer();
		alloc_ = str.alloc_;
	}

	if ( str.oup_outer_buffer_ == nullptr ) {
		deallocate_outer_buffer();
		std::memcpy( soo_buff_, str.soo_buff_, soo_buff_size_ );
		op_string_ = soo_buff_;
	} else {
		allocate_outer_buffer( str.outer_buffer_array_size_ );
		const size_t copy_bytes = sizeof( charT[str.outer_buffer_array_size_] );
		std::memcpy( oup_outer_buffer_.get(), str.oup_outer_buffer_.get(), copy_bytes );
		op_string_ = oup_outer_buffer_;
	}

	str.deallocate_outer_buffer();
	str.op_string_   = str.soo_buff_;
	str.soo_buff_[0] = 0;

	return *this;
}

template <typename charT, class traits, class Allocator>
void offset_basic_string<charT, traits, Allocator>::swap( offset_basic_string& str )
{
	if ( this == &str ) {
		return;
	}

	if constexpr ( allocator_traits::propagate_on_container_swap::value ) {
		// swapで、アロケータをコピー伝搬させる場合
		allocator_type swap_tmp_alloc = str.alloc_;
		str.alloc_                    = alloc_;
		alloc_                        = swap_tmp_alloc;
		oup_outer_buffer_.swap( str.oup_outer_buffer_ );
		std::swap( outer_buffer_array_size_, str.outer_buffer_array_size_ );

		offset_ptr<value_type> op_swap_backup = op_string_;
		if ( str.op_string_ == str.soo_buff_ ) {
			op_string_ = soo_buff_;
		} else {
			op_string_ = oup_outer_buffer_;
		}
		if ( op_swap_backup == soo_buff_ ) {
			str.op_string_ = str.soo_buff_;
		} else {
			str.op_string_ = str.oup_outer_buffer_;
		}

		for ( size_t i = 0; i < soo_buff_size_; i++ ) {
			std::swap( soo_buff_[i], str.soo_buff_[i] );
		}
	} else {
		size_t     this_size_plus_1  = size() + 1;
		size_t     other_size_plus_1 = str.size() + 1;
		value_type this_backup[this_size_plus_1];
		value_type other_backup[other_size_plus_1];

		std::memcpy( this_backup, op_string_.get(), this_size_plus_1 );
		std::memcpy( other_backup, str.op_string_.get(), other_size_plus_1 );

		clear();
		str.clear();

		if ( other_size_plus_1 > soo_buff_size_ ) {
			// outerバッファを用意する
			allocate_outer_buffer( other_size_plus_1 );
			std::memcpy( oup_outer_buffer_.get(), other_backup, other_size_plus_1 );
			op_string_ = oup_outer_buffer_;
		} else {
			std::memcpy( soo_buff_, other_backup, other_size_plus_1 );
			op_string_ = soo_buff_;
		}
		if ( this_size_plus_1 > soo_buff_size_ ) {
			// outerバッファを用意する
			str.allocate_outer_buffer( this_size_plus_1 );
			std::memcpy( str.oup_outer_buffer_.get(), this_backup, this_size_plus_1 );
			str.op_string_ = str.oup_outer_buffer_;
		} else {
			std::memcpy( str.soo_buff_, this_backup, this_size_plus_1 );
			str.op_string_ = str.soo_buff_;
		}
	}
}

}   // namespace ipsm

#endif   // OFFSET_STRING_HPP_
