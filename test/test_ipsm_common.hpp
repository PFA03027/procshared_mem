/**
 * @file test_ipsm_common.hpp
 * @author PFA03027@nifty.com
 * @brief
 * @version 0.1
 * @date 2023-09-24
 *
 * @copyright Copyright (c) 2023-2024, PFA03027@nifty.com
 *
 */

#ifndef TEST_IPSM_COMMON_HPP_
#define TEST_IPSM_COMMON_HPP_

#include <functional>

struct child_proc_return_t {
	bool          is_exit_normaly_;
	unsigned char exit_code_;
	int           signal_code_;
};

child_proc_return_t call_pred_on_child_process( std::function<int()> pred_func_in_child_process );

#endif   // TEST_IPSM_COMMON_HPP_