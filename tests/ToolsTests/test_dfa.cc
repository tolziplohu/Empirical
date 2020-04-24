#define CATCH_CONFIG_MAIN

#include "third-party/Catch/single_include/catch.hpp"

#include "tools/DFA.h"

#include <sstream>
#include <iostream>

TEST_CASE("Test DFA", "[tools]")
{
  emp::DFA dfa(5);
  dfa.SetTransition(0, 4, 'a');
  dfa.SetTransition(4, 3, 'b');
  dfa.SetTransition(3, 2, 'a');
  dfa.SetTransition(3, 0, 'c');
  dfa.SetTransition(2, 1, 'b');
  dfa.SetTransition(1, 0, 'c');
  
  int state = 0;
  REQUIRE( (state = dfa.Next(state, 'a')) == 4 );
  REQUIRE( (state = dfa.Next(state, 'b')) == 3 );
  REQUIRE( (state = dfa.Next(state, 'c')) == 0 );
  REQUIRE( (state = dfa.Next(state, 'a')) == 4 );
  REQUIRE( (state = dfa.Next(state, 'b')) == 3 );
  REQUIRE( (state = dfa.Next(state, 'a')) == 2 );
  REQUIRE( (state = dfa.Next(state, 'b')) == 1 );
  REQUIRE( (state = dfa.Next(state, 'c')) == 0 );
  REQUIRE( (state = dfa.Next(state, 'b')) == -1 );
  REQUIRE( (state = dfa.Next(state, 'c')) == -1 );
  
  // ASCII values of characters
  size_t a = 97;
  size_t b = 98;
  size_t c = 99;
  
  // GetTransitions
  emp::array<int, 128> transitions = dfa.GetTransitions(3);
  REQUIRE( transitions[a] == 2 );
  REQUIRE( transitions[b] == -1 ); // b not a transition from 3
  REQUIRE( transitions[c] == 0 );
  
  // Stop
  int state1 = 0;
  size_t state2 = 3;
  dfa.SetStop(state2);
  REQUIRE(dfa.IsStop(state2));
  REQUIRE(dfa.GetStop(state2) != 0);
  
  // IsActive is true if number passed in is not -1, otherwise false
  // what is the context to this function?
  REQUIRE(dfa.IsActive(state1));
  REQUIRE(dfa.IsActive(state2));
  
  // Print
  std::stringstream ss;
  dfa.Print(ss);
  REQUIRE(ss.str() == "Num states = 5\nStop IDs: 3\n 0 -> 'a':4\n 1 -> 'c':0\n 2 -> 'b':1\n 3 -> 'a':2 'c':0 [STOP=1]\n 4 -> 'b':3\n");
  
  
}