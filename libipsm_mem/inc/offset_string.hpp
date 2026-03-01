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
          class Allocator = offset_allocator<charT>>
class offset_basic_string {
public:
	using traits_type     = traits;
	using value_type      = traits::char_type;
	using allocator_type  = typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;
	using size_type       = std::allocator_traits<allocator_type>::size_type;
	using difference_type = std::allocator_traits<allocator_type>::difference_type;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using pointer         = std::allocator_traits<allocator_type>::pointer;

	offset_basic_string( void ) noexcept( noexcept( Allocator() ) )
	  : offset_basic_string( Allocator() )
	{
	}

	~offset_basic_string() = default;

	explicit offset_basic_string( const Allocator& a ) noexcept
	  : alloc_( a )
	  , oup_outer_buffer_( nullptr )
	  , op_string_( soo_buff_ )
	  , soo_buff_ { 0 }
	{
	}

	offset_basic_string( const offset_basic_string& str );
	offset_basic_string( const offset_basic_string& str, const Allocator& a );
	offset_basic_string( offset_basic_string&& str ) noexcept;
	offset_basic_string( const charT* s, const Allocator& a = Allocator() );
	offset_basic_string( const std::basic_string<charT>& s, const Allocator& a = Allocator() );

	offset_basic_string& operator=( const offset_basic_string& str );
	offset_basic_string& operator=( offset_basic_string&& str ) noexcept;

	const pointer c_str( void ) const noexcept
	{
		return op_string_;
	}
	const pointer data( void ) const noexcept
	{
		return op_string_;
	}

	allocator_type get_allocator() const noexcept
	{
		return alloc_;
	}

private:
	using allocator_traits = typename std::allocator_traits<allocator_type>::template rebind_traits<value_type>;

	static constexpr size_t soo_buff_size_ = sizeof( std::basic_string<value_type> ) / sizeof( value_type ) + 1;

	allocator_type                alloc_;
	offset_unique_ptr<value_type> oup_outer_buffer_;
	offset_ptr<value_type>        op_string_;
	value_type                    soo_buff_[soo_buff_size_];   //!< small object optimization buffer
};

///////////////////////////////////////////////////////////////////////////////////////////////
/// implementation

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>::offset_basic_string( const offset_basic_string& str )
  : offset_basic_string( str, Allocator() )
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
		size_t cur_len = strlen( str.oup_outer_buffer_.get() );
		charT* p       = alloc_.allocate( cur_len + 1 );
		if ( p == nullptr ) {
			throw std::bad_alloc();
		}
		oup_outer_buffer_ = p;
		std::memcpy( p, str.oup_outer_buffer_.get(), cur_len + 1 );
		op_string_ = p;
	}
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>::offset_basic_string( offset_basic_string&& str ) noexcept
  : offset_basic_string( str.alloc_ )
{
	if ( str.oup_outer_buffer_ == nullptr ) {
		op_string_ = soo_buff_;
		std::memcpy( soo_buff_, str.soo_buff_, soo_buff_size_ );
	} else {
		oup_outer_buffer_ = std::move( str.oup_outer_buffer_ );
		op_string_        = oup_outer_buffer_.get();
	}
	str.oup_outer_buffer_ = nullptr;
	str.op_string_        = str.soo_buff_;
	str.soo_buff_[0]      = 0;
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>::offset_basic_string( const charT* s, const Allocator& a = Allocator() )
{
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>& offset_basic_string<charT, traits, Allocator>::operator=( const offset_basic_string& str )
{
}

template <typename charT, class traits, class Allocator>
offset_basic_string<charT, traits, Allocator>& offset_basic_string<charT, traits, Allocator>::operator=( offset_basic_string&& str ) noexcept
{
}

}   // namespace ipsm

#endif   // OFFSET_STRING_HPP_
