/*
 * <p>Title: shared_memory_test.cpp</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 </p>
 *
 * @author Bob Huang
 * @date Nov 14, 2019 
 * @version 1.0
 */

#include "catch2/catch.hpp"
#include "shared_memory.h"


TEST_CASE("The ntb malloc method returns 0")
{
	int ret = ntb_malloc();
	REQUIRE(ret == 0);
}

//int Factorial( int number ) {
//   return number <= 1 ? number : Factorial( number - 1 ) * number;  // fail
//// return number <= 1 ? 1      : Factorial( number - 1 ) * number;  // pass
//}
//
//TEST_CASE( "Factorial of 0 is 1 (fail)", "[single-file]" ) {
//    REQUIRE( Factorial(0) == 1 );
//}
//
//TEST_CASE( "Factorials of 1 and higher are computed (pass)", "[single-file]" ) {
//    REQUIRE( Factorial(1) == 1 );
//    REQUIRE( Factorial(2) == 2 );
//    REQUIRE( Factorial(3) == 6 );
//    REQUIRE( Factorial(10) == 3628800 );
//}

