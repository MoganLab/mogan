

/******************************************************************************
 * MODULE     : locale_test.cpp
 * DESCRIPTION: tests on locale related routines
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "locale.hpp"

using lolly::locale::get_date;

TEST_MEMORY_LEAK_INIT

/******************************************************************************
 * strftime format tests
 ******************************************************************************/

TEST_CASE ("get_date strftime year") {
  string_eq (get_date ("english", "%Y", 2024, 1, 15), "2024");
  string_eq (get_date ("english", "%Y", 2023, 12, 31), "2023");
}

TEST_CASE ("get_date strftime month") {
  string_eq (get_date ("english", "%m", 2024, 1, 15), "01");
  string_eq (get_date ("english", "%m", 2024, 6, 25), "06");
  string_eq (get_date ("english", "%m", 2023, 12, 31), "12");
  string_eq (get_date ("english", "%m", 2024, 11, 9), "11");
}

TEST_CASE ("get_date strftime day") {
  string_eq (get_date ("english", "%d", 2024, 1, 15), "15");
  string_eq (get_date ("english", "%d", 2024, 6, 25), "25");
  string_eq (get_date ("english", "%d", 2024, 11, 9), "09");
}

TEST_CASE ("get_date strftime full") {
  string_eq (get_date ("english", "%Y-%m-%d", 2024, 1, 15), "2024-01-15");
  string_eq (get_date ("english", "%Y-%m-%d", 2023, 12, 31), "2023-12-31");
}

TEST_CASE ("get_date strftime month name") {
  // %B is locale-dependent via strftime; only verify it is supported.
  string r= get_date ("english", "%B", 2024, 1, 15);
  CHECK (N (r) > 0);
}

TEST_CASE ("get_date strftime weekday") {
  // %A is locale-dependent via strftime; only verify it is supported.
  string r= get_date ("english", "%A", 2024, 1, 15);
  CHECK (N (r) > 0);
}

TEST_CASE ("get_date strftime time") {
  string_eq (get_date ("english", "%H:%M", 2024, 1, 15), "00:00");
  string_eq (get_date ("english", "%H:%M:%S", 2024, 6, 25), "00:00:00");
}

/******************************************************************************
 * Qt format tests
 ******************************************************************************/

TEST_CASE ("get_date qt year") {
  string_eq (get_date ("english", "yyyy", 2024, 1, 15), "2024");
  string_eq (get_date ("english", "yyyy", 2023, 12, 31), "2023");
}

TEST_CASE ("get_date qt month") {
  string_eq (get_date ("english", "M", 2024, 1, 15), "1");
  string_eq (get_date ("english", "M", 2024, 6, 25), "6");
  string_eq (get_date ("english", "M", 2023, 12, 31), "12");
}

TEST_CASE ("get_date qt day") {
  string_eq (get_date ("english", "d", 2024, 1, 15), "15");
  string_eq (get_date ("english", "d", 2024, 11, 9), "9");
  string_eq (get_date ("english", "d", 2023, 12, 31), "31");
}

TEST_CASE ("get_date qt month name") {
  string_eq (get_date ("english", "MMMM", 2024, 1, 15), "January");
  string_eq (get_date ("english", "MMMM", 2024, 6, 25), "June");
  string_eq (get_date ("english", "MMMM", 2023, 12, 31), "December");
}

TEST_CASE ("get_date qt weekday") {
  string_eq (get_date ("english", "dddd", 2024, 1, 15), "Monday");
  string_eq (get_date ("english", "dddd", 2024, 6, 25), "Tuesday");
  string_eq (get_date ("english", "dddd", 2023, 12, 31), "Sunday");
}

/******************************************************************************
 * Default format tests
 ******************************************************************************/

TEST_CASE ("get_date default english") {
  string_eq (get_date ("english", "", 2024, 1, 15), "January 15, 2024");
  string_eq (get_date ("english", "", 2024, 6, 25), "June 25, 2024");
}

TEST_CASE ("get_date default british") {
  string_eq (get_date ("british", "", 2024, 1, 15), "January 15, 2024");
  string_eq (get_date ("british", "", 2024, 6, 25), "June 25, 2024");
}

TEST_CASE ("get_date default american") {
  string_eq (get_date ("american", "", 2024, 1, 15), "January 15, 2024");
  string_eq (get_date ("american", "", 2024, 6, 25), "June 25, 2024");
}

TEST_CASE ("get_date default german") {
  string_eq (get_date ("german", "", 2024, 1, 15), "15. Januar 2024");
  string_eq (get_date ("german", "", 2024, 6, 25), "25. Juni 2024");
}

TEST_CASE ("get_date default chinese") {
  string_eq (get_date ("chinese", "", 2024, 1, 15),
             "2024<#5e74>1<#6708>15<#65e5>");
}

TEST_CASE ("get_date default japanese") {
  string_eq (get_date ("japanese", "", 2024, 6, 25),
             "2024<#5e74>6<#6708>25<#65e5>");
}

TEST_CASE ("get_date default korean") {
  string_eq (get_date ("korean", "", 2024, 1, 15),
             "2024<#b144> 1<#c6d4> 15<#c77c>");
}

TEST_CASE ("get_date default unknown language") {
  string_eq (get_date ("unknown_language", "", 2024, 1, 15), "15 January 2024");
  string_eq (get_date ("unknown_language", "", 2024, 6, 25), "25 June 2024");
}

TEST_CASE ("get_date current date fallback") {
  string r= get_date ("english", "%Y");
  CHECK (N (r) == 4);
  for (int i= 0; i < N (r); i++)
    CHECK ((r[i] >= '0' && r[i] <= '9'));
}

TEST_MEMORY_LEAK_ALL
