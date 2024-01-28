/**
 * @file ipsm_time_util.hpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2024-01-19
 *
 * @copyright Copyright (c) 2024, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#ifndef IPSM_TIME_UTIL_HPP_
#define IPSM_TIME_UTIL_HPP_

#include <chrono>
#include <ctime>
#include <type_traits>

namespace ipsm {

namespace time_util {

/*
 * ctime:
 *   time_t -> 整数 or 浮動小数点
 *   struct timespec
 *   これらの型は値以上の意味を持たず、型としてクロックの種類（CLOCK_REALTIMEか、CLOCK_MONOTONICか、あるいはそれ以外か）の情報を持たない。
 *   さらには、時間差を表しているかもしれない。これも型として区別はつかない。
 *
 * C++:
 *   std::chrono::time_point<>
 *     std::chrono::system_clock
 *     std::chrono::steady_clock
 *   std::chrono::duration<>
 *   これらの型は、クロックの種類、時間差を型として区別することができる。
 *   ただし、ctimeでえられた時刻情報とクロックが同じであるかは実装依存で、また判別方法もない。
 *
 * 上記を踏まえ、ctimeでえられる時刻にstd::chronoの時刻(time_point)は、非可換であるという前提に立つべき。
 * その上で、性質が類似しているクロック間で時間差を介した計算は、誤差は発生するが可能と考える。具体的には、以下の組み合わせ。
 * ・CLOCK_REALTIMEとstd::chrono::system_clockは、時間差情報を介して差分計算可能
 * ・CLOCK_MONOTONICとstd::chrono::steady_clockは、時間差情報を介して差分計算可能
 * ・std::chrono::duration<>は、時間差情報として各クロックと演算可能
 *
 * また、成立する演算は、以下。（「時間差」はstd::chrono::duration<>を表す）
 * クロックX + クロックX -> 適用できない
 * クロックX + クロックY -> 適用できない
 * クロックX - クロックX -> 適用可。結果は、時間差
 * クロックX - クロックY -> 適用できない
 * クロックX + 時間差    -> 適用可。結果は、クロックX
 * クロックX - 時間差    -> 適用可。結果は、クロックX
 * 時間差 + 時間差       -> 適用可。結果は、時間差
 * 時間差 - 時間差       -> 適用可。結果は、時間差
 *
 */

/////////////////////////////////////////////////////////
struct is_exchangable_timediff_impl {
	template <typename ClockT, clockid_t CT>
	static auto chk( ClockT* p1 ) -> typename std::enable_if<ClockT::is_steady && ( ( CT == CLOCK_MONOTONIC ) || ( CT == CLOCK_MONOTONIC_COARSE ) || ( CT == CLOCK_MONOTONIC_RAW ) ), std::true_type>::type;

	template <typename ClockT, clockid_t CT>
	static auto chk( ClockT* p1 ) -> typename std::enable_if<!ClockT::is_steady && std::is_same<ClockT, std::chrono::system_clock>::value && ( ( CT == CLOCK_REALTIME ) || ( CT == CLOCK_REALTIME_COARSE ) ), std::true_type>::type;
	// 現時点の仕様としては、system_clockに制約する

	template <typename ClockT, clockid_t CT>
	static auto chk( ... ) -> std::false_type;
};

template <typename ClockT, clockid_t CT>
struct is_exchangable_timediff : public decltype( is_exchangable_timediff_impl::chk<ClockT, CT>( nullptr ) ) {};

/////////////////////////////////////////////////////////
struct is_steady_clock_impl {
	template <clockid_t CT>
	static auto chk( clockid_t* p1 ) -> typename std::enable_if<( CT == CLOCK_MONOTONIC ) || ( CT == CLOCK_MONOTONIC_COARSE ) || ( CT == CLOCK_MONOTONIC_RAW ) || ( CT == CLOCK_BOOTTIME ), std::true_type>::type;

	template <clockid_t CT>
	static auto chk( ... ) -> std::false_type;

	template <typename ClockT>
	static auto chk( ClockT* p1 ) -> typename std::conditional<ClockT::is_steady, std::true_type, std::false_type>::type;

	template <typename ClockT>
	static auto chk( ... ) -> std::false_type;
};

// template <typename ClockT>
// struct is_steady_clock : public decltype( is_steady_clock_impl::chk<ClockT>( nullptr ) ) {};
// template <>
// struct is_steady_clock<clockid_t> : public decltype( is_steady_clock_impl::chk<CTX>( nullptr ) ) {};

/////////////////////////////////////////////////////////

/**
 * @brief clock_gettime()によって取得できるクロックに基づくtime_point
 *
 * @tparam CT クロックの種類
 */
template <clockid_t CT>
class timespec_ct {
public:
	constexpr timespec_ct( void )
	  : tsp_ { 0 }
	{
	}

	template <typename TimePointT, clockid_t CTX = CT, typename std::enable_if<is_exchangable_timediff<typename TimePointT::clock, CTX>::value>::type* = nullptr>
	timespec_ct( const TimePointT& src )
	  : tsp_ { 0 }
	{
		auto timediff = src - TimePointT::clock::now();
		// clock_gettime()よりも先にClockT::now()を呼び出さないと、原理的に正しく動作しない。
		// 具体的には戻り値が、少なくともtpよりも早い時間となる可能性がある。その場合、Timeout時刻として仕様を満たさない。

		timespec_ct target_timespec = timespec_ct::now() + timediff;
		*this                       = target_timespec;
	}

	const struct timespec& get( void ) const
	{
		return tsp_;
	}

	constexpr clockid_t clockid( void ) const
	{
		return CT;
	}

	template <class Rep, class Period>
	timespec_ct& operator+=( const std::chrono::duration<Rep, Period>& op2 );
	template <class Rep, class Period>
	timespec_ct& operator-=( const std::chrono::duration<Rep, Period>& op2 );

	std::chrono::nanoseconds operator-( const timespec_ct& op2 ) const
	{
		return ts2duration_nanoseconds( tsp_ ) - ts2duration_nanoseconds( op2.tsp_ );
	}

	/**
	 * @brief Construct a new operator STD CTP object
	 *
	 * @tparam TimePointT time point type of std::chrono
	 */
	template <typename TimePointT, clockid_t CTX = CT, typename std::enable_if<is_exchangable_timediff<typename TimePointT::clock, CTX>::value>::type* = nullptr>
	operator TimePointT( void ) const
	{
		auto timediff = ( *this ) - timespec_ct::now();
		// ClockT::now()よりも先にをclock_gettime()呼び出さないと、原理的に正しく動作しない。
		// 具体的には戻り値が、少なくともtsp_よりも早い時間となる可能性がある。その場合、Timeout時刻として仕様を満たさない。

		TimePointT target_timespec = TimePointT::clock::now() + timediff;
		return target_timespec;
	}

	static timespec_ct now( void );

	static constexpr bool is_steady = decltype( is_steady_clock_impl::chk<CT>( nullptr ) )::value;

private:
	timespec_ct( const struct timespec& srcts )
	  : tsp_ { srcts }
	{
	}

	struct timespec tsp_;

	static std::chrono::nanoseconds ts2duration_nanoseconds( const timespec& src )
	{
		return std::chrono::seconds( src.tv_sec ) + std::chrono::nanoseconds( src.tv_nsec );
	}

	template <clockid_t CTX, class Rep, class Period>
	friend timespec_ct<CTX> operator+( const timespec_ct<CTX>& cp1, const std::chrono::duration<Rep, Period>& op2 );
	template <clockid_t CTX, class Rep, class Period>
	friend timespec_ct<CTX> operator+( const std::chrono::duration<Rep, Period>& op1, const timespec_ct<CTX>& cp2 );

	template <clockid_t CTX, class Rep, class Period>
	friend timespec_ct<CTX> operator-( const timespec_ct<CTX>& cp1, const std::chrono::duration<Rep, Period>& op2 );
	template <clockid_t CTX, class Rep, class Period>
	friend timespec_ct<CTX> operator-( const std::chrono::duration<Rep, Period>& op1, const timespec_ct<CTX>& cp2 );
};

using timespec_monotonic = timespec_ct<CLOCK_MONOTONIC>;
using timespec_realtime  = timespec_ct<CLOCK_REALTIME>;

///////////////////////////////////////////////////////////////////////////
// Implement
///////////////////////////////////////////////////////////////////////////
namespace internal {
template <class Rep, class Period>
inline struct timespec duration2timespec( const std::chrono::duration<Rep, Period>& src )
{
	if ( std::chrono::duration_cast<std::chrono::nanoseconds>( src ) < std::chrono::nanoseconds( 0 ) ) {
		throw std::range_error( "fail to convert to timespec, because time point value is negtive number" );
	}

	auto secs = std::chrono::duration_cast<std::chrono::seconds>( src );
	auto ns   = std::chrono::duration_cast<std::chrono::nanoseconds>( src ) - std::chrono::duration_cast<std::chrono::nanoseconds>( secs );

	return timespec { secs.count(), ns.count() };
}

struct timespec call_clock_gettime( const clockid_t clkid );
}   // namespace internal

template <clockid_t CT>
timespec_ct<CT> timespec_ct<CT>::now( void )
{
	return timespec_ct<CT>( internal::call_clock_gettime( CT ) );
}

template <clockid_t CT>
template <class Rep, class Period>
timespec_ct<CT>& timespec_ct<CT>::operator+=( const std::chrono::duration<Rep, Period>& op2 )
{
	auto tt = ts2duration_nanoseconds( tsp_ ) + op2;
	tsp_    = internal::duration2timespec( tt );
	return *this;
}

template <clockid_t CT>
template <class Rep, class Period>
timespec_ct<CT>& timespec_ct<CT>::operator-=( const std::chrono::duration<Rep, Period>& op2 )
{
	auto tt = ts2duration_nanoseconds( tsp_ ) - op2;
	tsp_    = internal::duration2timespec( tt );
	return *this;
}

template <clockid_t CT, class Rep, class Period>
timespec_ct<CT> operator+( const timespec_ct<CT>& op1, const std::chrono::duration<Rep, Period>& op2 )
{
	auto ans_du = timespec_ct<CT>::ts2duration_nanoseconds( op1.tsp_ ) + op2;

	return timespec_ct<CT>( internal::duration2timespec( ans_du ) );
}

template <clockid_t CT, class Rep, class Period>
timespec_ct<CT> operator+( const std::chrono::duration<Rep, Period>& op1, const timespec_ct<CT>& op2 )
{
	auto ans_du = op1 + timespec_ct<CT>::ts2duration_nanoseconds( op2.tsp_ );

	return timespec_ct<CT>( internal::duration2timespec( ans_du ) );
}

template <clockid_t CT, class Rep, class Period>
timespec_ct<CT> operator-( const timespec_ct<CT>& op1, const std::chrono::duration<Rep, Period>& op2 )
{
	auto ans_du = timespec_ct<CT>::ts2duration_nanoseconds( op1.tsp_ ) - op2;

	return timespec_ct<CT>( internal::duration2timespec( ans_du ) );
}

template <clockid_t CT, class Rep, class Period>
timespec_ct<CT> operator-( const std::chrono::duration<Rep, Period>& op1, const timespec_ct<CT>& op2 )
{
	auto ans_du = op1 - timespec_ct<CT>::ts2duration_nanoseconds( op2.tsp_ );

	return timespec_ct<CT>( internal::duration2timespec( ans_du ) );
}

}   // namespace time_util

}   // namespace ipsm

#endif   // IPSM_TIME_UTIL_HPP_
