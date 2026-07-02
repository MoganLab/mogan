/** \file tm_timer_test.cpp
 *  \copyright GPLv3
 *  \details A unitest for tm_timer.
 *  \author Paradisuman
 *  \date   2023
 */
#include "a_lolly_test.hpp"
#include "analyze.hpp"
#include "lolly/system/timer.hpp"
#include "string.hpp"
#include "tm_timer.hpp"
#include <cstring>

#include "tbox/tbox.h"

using lolly::system::bench_print;
using lolly::system::timer_cumul;
using lolly::system::timer_reset;
using lolly::system::timer_start;

int
get_timing_cumul (string out) {
  // out: Task 'task1' took 0 ms
  // out: Task 'task2' took 12 ms
  int  pos= 0;
  int  result;
  auto tokens= tokenize (out, " took ");

  read_int (tokens[1], pos, result);
  return result;
}

int
get_timing_nr (string out) {
  // out: Task 'task1' took 0 ms (0 invocations)
  // out: Task 'task2' took 12 ms (1 invocations)
  int  pos= 0;
  int  result;
  auto tokens= tokenize (out, " (");

  if (N (tokens) == 1) return -1;
  if (read_int (tokens[1], pos, result) == false) return -1;
  return result;
}

bool
test_same (tm_ostream& a, string b) {
  string sa= a.unbuffer ();

  return sa == b;
}

string
to_zero (tm_ostream& out) {
  string   s1= out.unbuffer ();
  c_string ans (s1);
  char*    current= ans;
  while ((current= strstr (current, "took"))) {
    current+= strlen ("took");
    while (*current && (*current == '(')) {
      current+= 2;
    }
    while (*current && (*current < '0' || *current > '9')) {
      current++;
    }
    while (*current && *current >= '0' && *current <= '9') {
      *current= '0';
      current++;
    }
  }
  return (char*) ans;
}

TEST_MEMORY_LEAK_INIT

TEST_CASE ("if time_t and long 64 bit") { CHECK (sizeof (time_t) == 8); }

TEST_CASE ("function get_sec_time") {
  tb_timeval_t tp= {0};
  tb_gettimeofday (&tp, tb_null);
  time_t t1= tp.tv_sec;
  time_t t2= get_sec_time ();

  CHECK (t2 >= t1);
}

TEST_CASE ("function get_usec_time") {
  tb_timeval_t tp= {0};
  tb_gettimeofday (&tp, tb_null);
  time_t t1= tp.tv_usec;
  time_t t2= get_usec_time ();

  CHECK (t2 >= t1);
}

TEST_CASE ("function raw_time") {
  time_t startTime= raw_time ();

  // CHECK (startTime >= 0);

  time_t endTime= raw_time ();

  CHECK (endTime >= startTime);
}

TEST_CASE ("function texmacs_time") {
  long t1= texmacs_time ();

  CHECK (t1 >= 0);

  long t2= texmacs_time ();

  CHECK (t2 >= t1);
}

TEST_CASE ("function timer_start and timer_cumul") {
  tm_ostream ostream;
  ostream.buffer ();

  SUBCASE ("start once") {
    timer_start ("task1");
    bench_print (ostream, "task1");
    string out= ostream.unbuffer ();
    int    t1 = get_timing_cumul (out);

    CHECK (t1 == 0);

    timer_cumul ("task1");
    ostream.buffer ();
    bench_print (ostream, "task1");
    out   = ostream.unbuffer ();
    int t2= get_timing_cumul (out);

    CHECK (t2 >= 0);
  }

  SUBCASE ("start multiple times") {
    timer_start ("task2");

    timer_cumul ("task2");
    bench_print (ostream, "task1");
    string out= ostream.unbuffer ();
    int    t1 = get_timing_cumul (out);

    CHECK (t1 >= 0);
    CHECK (get_timing_nr (out) == -1);

    timer_start ("task2");

    timer_cumul ("task2");
    ostream.buffer ();
    bench_print (ostream, "task2");
    out   = ostream.unbuffer ();
    int t2= get_timing_cumul (out);
    int nr= get_timing_nr (out);

    CHECK (t2 >= 0);
    CHECK (nr == 2);
  }
}

TEST_CASE ("function timer_reset") {
  tm_ostream ostream;
  ostream.buffer ();

  SUBCASE ("if task empty") {
    timer_reset ("task");
    bench_print (ostream, "task");
    string out= ostream.unbuffer ();

    CHECK (get_timing_nr (out) == -1);
  }

  SUBCASE ("if task not empty") {
    timer_start ("task");
    timer_cumul ("task");
    timer_start ("task");
    timer_cumul ("task");

    timer_reset ("task");
    bench_print (ostream, "task");
    string out= ostream.unbuffer ();

    CHECK (get_timing_nr (out) == -1);
  }
}

TEST_CASE ("function bench_print") {
  tm_ostream ostream;
  ostream.buffer ();

  SUBCASE ("print one task") {
    bench_print (ostream, "task");
    string b= "Task 'task' took 0 ms\n";

    CHECK (test_same (ostream, b));

    ostream.buffer ();
    timer_reset ("task1");
    timer_start ("task1");
    timer_cumul ("task1");
    timer_start ("task1");
    timer_cumul ("task1");
    bench_print (ostream, "task1");

    string ans= "Task 'task1' took 0 ms (2 invocations)\n";
    string out= to_zero (ostream);

    CHECK (out == ans);
  }

  SUBCASE ("print multiple task") {
    timer_reset ("task1");
    timer_reset ("task2");

    timer_start ("task1");
    timer_cumul ("task1");
    timer_start ("task2");
    timer_cumul ("task2");
    bench_print (ostream);

    string out= to_zero (ostream);
    string b  = "Task 'task1' took 0 ms\nTask 'task2' took 0 ms\n";

    CHECK (out == b);
  }
}

TEST_CASE ("testing memory leakage of tm_timer") {
  timer_reset ("task");
  timer_reset ("task1");
  timer_reset ("task2");
  CHECK_EQ (mem_used (), mem_lolly);
}
