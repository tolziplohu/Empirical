#define CATCH_CONFIG_MAIN

#include "third-party/Catch/single_include/catch.hpp"

#include "base/array.h"

#include <sstream>
#include <iostream>

TEST_CASE("Test array", "[base]")
{
	const size_t SIZE = 10;
	emp::array<int, SIZE> arr = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	REQUIRE(arr[5] == 5);
	emp::array<int, SIZE> arr2(arr.begin(), arr.end());
	REQUIRE(arr == arr2);
	std::array<int, SIZE> s_arr = (std::array<int,SIZE>) arr2;
	REQUIRE(s_arr[8] == arr2[8]);
	REQUIRE(arr.back() == 9);
	REQUIRE(arr.front() == 0);
	
	// Testing emp::assert in array.h functions
	arr.resize(12);
	REQUIRE(emp::assert_last_fail);
	emp::assert_clear();
	
	arr.resize(15, 10);
	REQUIRE(emp::assert_last_fail);
	emp::assert_clear();
	
	int three = arr[3];
	REQUIRE(three == 3);
	REQUIRE(!emp::assert_last_fail);
	emp::assert_clear();
	
	arr.push_back(10);
	REQUIRE(emp::assert_last_fail);
	emp::assert_clear();
	
	arr.pop_back();
	REQUIRE(emp::assert_last_fail);
	emp::assert_clear();
	
	int one = arr[1];
	REQUIRE(one == 1);
	REQUIRE(!emp::assert_last_fail);
	emp::assert_clear();
	
	std::stringstream ss;
	ss << arr;
	REQUIRE(ss.str() == "0 1 2 3 4 5 6 7 8 9 ");
	
	emp::array<int, SIZE> arr3;
	ss >> arr3;
	REQUIRE(arr3[2] == 2);
	ss.str(std::string());
	
	// Seems like it shouldn't work, throws emp_assert false, but actually inserts...
	arr.insert(arr.begin(), 10);
	REQUIRE(emp::assert_last_fail);
	emp::assert_clear();
	
	// Same issue as insert
	arr.erase(arr.begin());
	REQUIRE(emp::assert_last_fail);
	emp::assert_clear();
	
	// Same issue as insert
	arr.emplace(arr.end(), 11);
	REQUIRE(emp::assert_last_fail);
	emp::assert_clear();
	
	arr.emplace_back(12);
	REQUIRE(emp::assert_last_fail);
	emp::assert_clear();
}