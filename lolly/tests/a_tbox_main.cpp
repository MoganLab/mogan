#define DOCTEST_CONFIG_IMPLEMENT

#include "doctest/doctest.h"
#include "lolly_doctests.hpp"
#include "sys_utils.hpp"

int
main (int argc, char** argv) {
  lolly::init_tbox ();
  doctest::Context context;

  int res= context.run ();

  if (context.shouldExit ()) return res;

  return res;
}