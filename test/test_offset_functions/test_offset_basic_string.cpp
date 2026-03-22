/**
 * @file test_offset_basic_string.cpp
 * @author Teruaki Ata (PFA03027@nifty.com)
 * @brief
 * @version 0.1
 * @date 2026-03-22
 *
 * @copyright Copyright (c) 2026, Teruaki Ata (PFA03027@nifty.com)
 *
 */

#include "gtest/gtest.h"

#include "offset_string.hpp"

TEST( TestOffsetBasicString, CanConstruct )
{
	// Arrange
	using sut_type = ipsm::offset_basic_string<char>;

	// Act
	sut_type sut;

	// Assert
	EXPECT_TRUE( sut.empty() );
	EXPECT_EQ( sut.size(), 0 );
}
