/*
 * <p>Title: ntb-monitor_test.cpp</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 15, 2019 
 * @version 1.0
 */

#include <catch2/catch.hpp>

#include "ntb_monitor.h"
//#include "config.h"

ntm_manager_t ntm_mgr = NULL;

TEST_CASE("The ntb print_monitor method returns 0")
{
	int ret = print_monitor();
	REQUIRE(ret == 0);
}
