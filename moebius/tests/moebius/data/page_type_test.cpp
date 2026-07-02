/** \file page_type_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for page size data base
 *  \author Da Shen
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include "string.hpp"
#include <moebius/data/page_type.hpp>
#include <moebius/vars.hpp>

using namespace moebius;

using moebius::data::page_get_feature;

TEST_CASE ("test page_get_feature common sizes portrait") {
  CHECK_EQ (page_get_feature ("a4", PAGE_WIDTH, false) == "210mm", true);
  CHECK_EQ (page_get_feature ("a4", PAGE_HEIGHT, false) == "297mm", true);
  CHECK_EQ (page_get_feature ("a4", PAR_WIDTH, false) == "150mm", true);
  CHECK_EQ (page_get_feature ("a4", "standard", false) == "yes", true);

  CHECK_EQ (page_get_feature ("letter", PAGE_WIDTH, false) == "8.5in", true);
  CHECK_EQ (page_get_feature ("letter", PAGE_HEIGHT, false) == "11in", true);
  CHECK_EQ (page_get_feature ("letter", PAR_WIDTH, false) == "6.5in", true);

  CHECK_EQ (page_get_feature ("a5", PAGE_WIDTH, false) == "148mm", true);
  CHECK_EQ (page_get_feature ("a5", PAGE_HEIGHT, false) == "210mm", true);

  CHECK_EQ (page_get_feature ("legal", PAGE_WIDTH, false) == "8.5in", true);
  CHECK_EQ (page_get_feature ("legal", PAGE_HEIGHT, false) == "14in", true);
}

TEST_CASE ("test page_get_feature landscape swaps width and height") {
  CHECK_EQ (page_get_feature ("a4", PAGE_WIDTH, true) == "297mm", true);
  CHECK_EQ (page_get_feature ("a4", PAGE_HEIGHT, true) == "210mm", true);
  CHECK_EQ (page_get_feature ("a4", PAR_WIDTH, true) == "237mm", true);

  CHECK_EQ (page_get_feature ("letter", PAGE_WIDTH, true) == "11in", true);
  CHECK_EQ (page_get_feature ("letter", PAGE_HEIGHT, true) == "8.5in", true);
}

TEST_CASE ("test page_get_feature margins") {
  CHECK_EQ (page_get_feature ("a4", PAGE_ODD, false) == "30mm", true);
  CHECK_EQ (page_get_feature ("a4", PAGE_EVEN, false) == "30mm", true);
  CHECK_EQ (page_get_feature ("a4", PAGE_RIGHT, false) == "30mm", true);
  CHECK_EQ (page_get_feature ("a4", PAGE_TOP, false) == "30mm", true);
  CHECK_EQ (page_get_feature ("a4", PAGE_BOT, false) == "30mm", true);

  CHECK_EQ (page_get_feature ("letter", PAGE_ODD, false) == "1in", true);
  CHECK_EQ (page_get_feature ("letter", PAGE_TOP, false) == "1in", true);
}

TEST_CASE ("test page_get_feature unknown type falls back to a4") {
  CHECK_EQ (page_get_feature ("unknown", PAGE_WIDTH, false) ==
                page_get_feature ("a4", PAGE_WIDTH, false),
            true);
  CHECK_EQ (page_get_feature ("unknown", PAR_WIDTH, true) ==
                page_get_feature ("a4", PAR_WIDTH, true),
            true);
}
