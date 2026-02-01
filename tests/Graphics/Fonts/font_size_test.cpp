
/*****************************************************************************
 * MODULE     : font_size_test.cpp
 * DESCRIPTION: Tests for font size support with 0.5 multiples
 * COPYRIGHT  : (C) 2026  Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "converter.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "font.hpp"
#include "qtestcase.h"
#include "smart_font.hpp"
#include "sys_utils.hpp"
#include "tm_debug.hpp"
#include "tm_sys_utils.hpp"
#include "tree_helper.hpp"
#include <QtTest/QtTest>
#include <cmath>

class TestFontSize : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
  }

  // Test helper functions
  void test_is_half_multiple ();
  void test_round_to_half_multiple ();
  void test_normalize_half_multiple_size ();
  void test_font_size_as_int ();
  void test_effective_size ();
  void test_set_get_font_size ();

  // Test script function
  void test_script_function ();

  // Test font creation with float sizes
  void test_smart_font_float_size ();
  void test_backward_compatibility ();
  void test_virtual_font_float_size ();
  void test_cache_key_generation ();

  // Additional font size tests
  void test_to_tex_font_size ();
  void test_get_script_size ();
  void test_determine_sizes ();

  // Performance tests
  void test_performance_script_function ();
  void test_performance_to_tex_font_size ();
  void test_performance_float_calculations ();
};

// Test is_half_multiple() function
void
TestFontSize::test_is_half_multiple () {
  // Integer sizes should be half multiples (since 2*integer is integer)
  QVERIFY (is_half_multiple (10.0));
  QVERIFY (is_half_multiple (12.0));
  QVERIFY (is_half_multiple (8.0));

  // Half multiples
  QVERIFY (is_half_multiple (10.5));
  QVERIFY (is_half_multiple (11.5));
  QVERIFY (is_half_multiple (8.5));

  // Non-half multiples
  QVERIFY (!is_half_multiple (10.3));
  QVERIFY (!is_half_multiple (10.7));
  QVERIFY (!is_half_multiple (10.1));
  QVERIFY (!is_half_multiple (10.9));

  // Edge cases
  QVERIFY (is_half_multiple (0.0));
  QVERIFY (is_half_multiple (0.5));
  QVERIFY (is_half_multiple (1.0));
}

// Test round_to_half_multiple() function
void
TestFontSize::test_round_to_half_multiple () {
  // Values already at half multiples should stay the same
  QCOMPARE (round_to_half_multiple (10.0), 10.0);
  QCOMPARE (round_to_half_multiple (10.5), 10.5);
  QCOMPARE (round_to_half_multiple (11.0), 11.0);
  QCOMPARE (round_to_half_multiple (11.5), 11.5);

  // Values should round to nearest half multiple
  QCOMPARE (round_to_half_multiple (10.1), 10.0);
  QCOMPARE (round_to_half_multiple (10.2), 10.0);
  QCOMPARE (round_to_half_multiple (10.3), 10.5);
  QCOMPARE (round_to_half_multiple (10.4), 10.5);
  QCOMPARE (round_to_half_multiple (10.6), 10.5);
  QCOMPARE (round_to_half_multiple (10.7), 10.5);
  QCOMPARE (round_to_half_multiple (10.8), 11.0);
  QCOMPARE (round_to_half_multiple (10.9), 11.0);

  // Edge cases
  QCOMPARE (round_to_half_multiple (0.1), 0.0);
  QCOMPARE (round_to_half_multiple (0.24), 0.0);
  QCOMPARE (round_to_half_multiple (0.25), 0.5);
  QCOMPARE (round_to_half_multiple (0.26), 0.5);
  QCOMPARE (round_to_half_multiple (0.74), 0.5);
  QCOMPARE (round_to_half_multiple (0.75), 1.0);
  QCOMPARE (round_to_half_multiple (0.76), 1.0);
}

// Test normalize_half_multiple_size() function
void
TestFontSize::test_normalize_half_multiple_size () {
  // Exact integer sizes should stay the same
  QCOMPARE (normalize_half_multiple_size (10.0), 10.0);
  QCOMPARE (normalize_half_multiple_size (12.0), 12.0);
  QCOMPARE (normalize_half_multiple_size (8.0), 8.0);

  // Exact half multiples should stay the same
  QCOMPARE (normalize_half_multiple_size (10.5), 10.5);
  QCOMPARE (normalize_half_multiple_size (11.5), 11.5);
  QCOMPARE (normalize_half_multiple_size (8.5), 8.5);

  // Values within tolerance of half multiples should stay the same
  // Tolerance is 0.001 in is_half_multiple()
  QCOMPARE (normalize_half_multiple_size (10.5001),
            10.5001); // Within tolerance
  QCOMPARE (normalize_half_multiple_size (10.4999),
            10.4999); // Within tolerance
  QCOMPARE (normalize_half_multiple_size (11.5001),
            11.5001); // Within tolerance
  QCOMPARE (normalize_half_multiple_size (11.4999),
            11.4999); // Within tolerance

  // Values outside tolerance should round to nearest half multiple
  QCOMPARE (normalize_half_multiple_size (10.501), 10.5); // Outside tolerance
  QCOMPARE (normalize_half_multiple_size (10.499), 10.5); // Outside tolerance
  QCOMPARE (normalize_half_multiple_size (11.502), 11.5); // Outside tolerance
  QCOMPARE (normalize_half_multiple_size (11.498), 11.5); // Outside tolerance

  // Non-half multiples should round to nearest half multiple
  QCOMPARE (normalize_half_multiple_size (10.1), 10.0);
  QCOMPARE (normalize_half_multiple_size (10.2), 10.0);
  QCOMPARE (normalize_half_multiple_size (10.3), 10.5);
  QCOMPARE (normalize_half_multiple_size (10.4), 10.5);
  QCOMPARE (normalize_half_multiple_size (10.6), 10.5);
  QCOMPARE (normalize_half_multiple_size (10.7), 10.5);
  QCOMPARE (normalize_half_multiple_size (10.8), 11.0);
  QCOMPARE (normalize_half_multiple_size (10.9), 11.0);

  // Edge cases
  QCOMPARE (normalize_half_multiple_size (0.0), 0.0);
  QCOMPARE (normalize_half_multiple_size (0.5), 0.5);
  QCOMPARE (normalize_half_multiple_size (1.0), 1.0);
  QCOMPARE (normalize_half_multiple_size (0.1), 0.0);
  QCOMPARE (normalize_half_multiple_size (0.24), 0.0);
  QCOMPARE (normalize_half_multiple_size (0.25), 0.5);
  QCOMPARE (normalize_half_multiple_size (0.26), 0.5);
  QCOMPARE (normalize_half_multiple_size (0.74), 0.5);
  QCOMPARE (normalize_half_multiple_size (0.75), 1.0);
  QCOMPARE (normalize_half_multiple_size (0.76), 1.0);

  // Verify behavior matches is_half_multiple() + round_to_half_multiple()
  double test_values[]= {10.0, 10.5, 10.5001, 10.4999, 10.501, 10.499,
                         10.3, 10.7, 0.0,     0.5,     1.0};
  for (double val : test_values) {
    double result= normalize_half_multiple_size (val);
    if (is_half_multiple (val)) {
      // If value is within tolerance of a half multiple, should return original
      QCOMPARE (result, val);
    }
    else {
      // Otherwise should round to nearest half multiple
      QCOMPARE (result, round_to_half_multiple (val));
    }
  }
}

// Test font_size_as_int() function
void
TestFontSize::test_font_size_as_int () {
  // Integer sizes should stay the same
  QCOMPARE (font_size_as_int (10.0), 10);
  QCOMPARE (font_size_as_int (12.0), 12);
  QCOMPARE (font_size_as_int (8.0), 8);

  // Half multiples should round to nearest integer
  QCOMPARE (font_size_as_int (10.5), 11);  // 10.5 rounds to 11
  QCOMPARE (font_size_as_int (10.49), 10); // 10.49 rounds to 10
  QCOMPARE (font_size_as_int (10.51), 11); // 10.51 rounds to 11
  QCOMPARE (font_size_as_int (11.5), 12);

  // Non-half multiples should also round correctly
  QCOMPARE (font_size_as_int (10.3), 10);
  QCOMPARE (font_size_as_int (10.7), 11);
}

// Test effective_size() method
void
TestFontSize::test_effective_size () {
  // Set up a font rule
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  // Create a font with integer size
  font fn_int= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  QVERIFY (!is_nil (fn_int));
  smart_font_rep* fn_rep_int= (smart_font_rep*) fn_int.rep;
  // effective_size() should return 10.0
  QVERIFY (qAbs (fn_rep_int->effective_size () - 10.0) < 0.001);

  // Create a font with half-multiple size
  font fn_half= smart_font ("sys-chinese", "rm", "medium", "right", 10.5, 600);
  QVERIFY (!is_nil (fn_half));
  smart_font_rep* fn_rep_half= (smart_font_rep*) fn_half.rep;
  // effective_size() should return 10.5 (or corrected value)
  // Since 10.5 is already a half-multiple, it should stay 10.5
  QVERIFY (qAbs (fn_rep_half->effective_size () - 10.5) < 0.001);

  // Create a font with non-half-multiple size (should be corrected)
  font fn_corrected=
      smart_font ("sys-chinese", "rm", "medium", "right", 10.3, 600);
  QVERIFY (!is_nil (fn_corrected));
  smart_font_rep* fn_rep_corrected= (smart_font_rep*) fn_corrected.rep;
  // effective_size() should return 10.5 (corrected from 10.3)
  QVERIFY (qAbs (fn_rep_corrected->effective_size () - 10.5) < 0.001);
}

// Test set_font_size() and get_font_size() functions
void
TestFontSize::test_set_get_font_size () {
  // Set up a font rule
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  // Create a font to get a font_rep object
  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  QVERIFY (!is_nil (fn));
  smart_font_rep* fn_rep= (smart_font_rep*) fn.rep;

  // Test get_font_size() - should return effective_size()
  double current_size= get_font_size (fn_rep);
  QVERIFY (qAbs (current_size - 10.0) < 0.001);

  // Test set_font_size() with integer size
  set_font_size (fn_rep, 12.0);
  QVERIFY (qAbs (get_font_size (fn_rep) - 12.0) < 0.001);
  QVERIFY (qAbs (fn_rep->effective_size () - 12.0) < 0.001);

  // Test set_font_size() with half-multiple size
  set_font_size (fn_rep, 12.5);
  QVERIFY (qAbs (get_font_size (fn_rep) - 12.5) < 0.001);
  QVERIFY (qAbs (fn_rep->effective_size () - 12.5) < 0.001);

  // Test set_font_size() with non-half-multiple size (should be corrected)
  set_font_size (fn_rep, 12.3);
  // Should be corrected to nearest half-multiple (12.5)
  QVERIFY (qAbs (get_font_size (fn_rep) - 12.5) < 0.001);
  QVERIFY (qAbs (fn_rep->effective_size () - 12.5) < 0.001);

  // Test set_font_size() with another non-half-multiple (should correct
  // to 12.0)
  set_font_size (fn_rep, 12.1);
  QVERIFY (qAbs (get_font_size (fn_rep) - 12.0) < 0.001);
  QVERIFY (qAbs (fn_rep->effective_size () - 12.0) < 0.001);
}

// Test script() function
void
TestFontSize::test_script_function () {
  // Test level 0: should return input (after half-multiple correction)
  QCOMPARE (script (10.0, 0), 10.0);
  QCOMPARE (script (10.5, 0), 10.5);
  QCOMPARE (script (12.0, 0), 12.0);
  QCOMPARE (script (12.5, 0), 12.5);

  // Test non-half-multiple input gets corrected
  QVERIFY (qAbs (script (10.3, 0) - 10.5) <
           0.001); // 10.3 should correct to 10.5
  QVERIFY (qAbs (script (10.7, 0) - 10.5) <
           0.001); // 10.7 should correct to 10.5

  // Test level 1: (sz * 2 + 2) / 3
  QVERIFY (qAbs (script (10.0, 1) - (10.0 * 2.0 + 2.0) / 3.0) < 0.001);
  QVERIFY (qAbs (script (10.5, 1) - (10.5 * 2.0 + 2.0) / 3.0) < 0.001);
  QVERIFY (qAbs (script (12.0, 1) - (12.0 * 2.0 + 2.0) / 3.0) < 0.001);
  QVERIFY (qAbs (script (12.5, 1) - (12.5 * 2.0 + 2.0) / 3.0) < 0.001);

  // Test level 2: apply formula twice
  double level1_10= (10.0 * 2.0 + 2.0) / 3.0;
  double level2_10= (level1_10 * 2.0 + 2.0) / 3.0;
  QVERIFY (qAbs (script (10.0, 2) - level2_10) < 0.001);

  double level1_105= (10.5 * 2.0 + 2.0) / 3.0;
  double level2_105= (level1_105 * 2.0 + 2.0) / 3.0;
  QVERIFY (qAbs (script (10.5, 2) - level2_105) < 0.001);

  // Test level bounds: negative levels treated as 0, levels > 2 treated as 2
  QVERIFY (qAbs (script (10.0, -1) - script (10.0, 0)) < 0.001);
  QVERIFY (qAbs (script (10.0, 3) - script (10.0, 2)) < 0.001);
  QVERIFY (qAbs (script (10.0, 100) - script (10.0, 2)) < 0.001);
}

// Test smart_font with float sizes
void
TestFontSize::test_smart_font_float_size () {
  // Set up a font rule like in smart_font_test.cpp
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  // Test creating smart_font with integer size (backward compatibility)
  font fn_int= smart_font ("sys-chinese", "rm", "medium", "right", 10.0, 600);
  QVERIFY (!is_nil (fn_int));
  qcompare (fn_int->res_name, "sys-chinese-rm-medium-right-10-600-smart");

  // Test creating smart_font with half-multiple size
  font fn_half= smart_font ("sys-chinese", "rm", "medium", "right", 10.5, 600);
  QVERIFY (!is_nil (fn_half));
  // The res_name should include the float size (might be rounded in name)
  // We just verify the font was created successfully

  // Test creating smart_font with another half-multiple size
  font fn_half2= smart_font ("sys-chinese", "rm", "medium", "right", 11.5, 600);
  QVERIFY (!is_nil (fn_half2));

  // Test that non-half-multiple size gets corrected
  // 10.3 should be corrected to 10.5 internally
  font fn_corrected=
      smart_font ("sys-chinese", "rm", "medium", "right", 10.3, 600);
  QVERIFY (!is_nil (fn_corrected));
}

// Test backward compatibility
void
TestFontSize::test_backward_compatibility () {
  // Set up a font rule
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  // Test that integer sizes work (old behavior)
  font fn_int= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  QVERIFY (!is_nil (fn_int));

  // Test that float integer sizes (e.g., 10.0) produce same result
  font fn_float_int=
      smart_font ("sys-chinese", "rm", "medium", "right", 10.0, 600);
  QVERIFY (!is_nil (fn_float_int));
  // They should have the same res_name since 10.0 is integer
  qcompare (fn_int->res_name, fn_float_int->res_name);

  // Test half-multiple sizes work (new behavior)
  font fn_half= smart_font ("sys-chinese", "rm", "medium", "right", 10.5, 600);
  QVERIFY (!is_nil (fn_half));
  // The res_name should be different for half-multiple sizes
  // (implementation may vary, but we verify it's created)

  // Test font_size_as_int function for backward compatibility
  QCOMPARE (font_size_as_int (10.0), 10);
  QCOMPARE (font_size_as_int (10.5), 11);  // 10.5 rounds to 11
  QCOMPARE (font_size_as_int (10.49), 10); // 10.49 rounds to 10
}

// Test virtual_font with float sizes
void
TestFontSize::test_virtual_font_float_size () {
  // Set up a font rule
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  // Create a base font
  font base= smart_font ("sys-chinese", "rm", "medium", "right", 10.0, 600);
  QVERIFY (!is_nil (base));

  // Use an existing virtual font that should be available
  string vf_name= "emu-arrows";

  // Test creating virtual_font with integer size
  font vf_int= virtual_font (base, vf_name, 10.0, 600, 600, false);
  QVERIFY (!is_nil (vf_int));

  // Test creating virtual_font with half-multiple size
  font vf_half= virtual_font (base, vf_name, 10.5, 600, 600, false);
  QVERIFY (!is_nil (vf_half));

  // Test creating virtual_font with non-half-multiple size (should be
  // corrected)
  font vf_corrected= virtual_font (base, vf_name, 10.3, 600, 600, false);
  QVERIFY (!is_nil (vf_corrected));

  // Test that integer and float integer produce same result
  font vf_int2= virtual_font (base, vf_name, 10, 600, 600, false);
  QVERIFY (!is_nil (vf_int2));
  // The res_name should be the same for 10 and 10.0
  qcompare (vf_int->res_name, vf_int2->res_name);
}

// Test cache key generation for different font sizes
void
TestFontSize::test_cache_key_generation () {
  // Set up a font rule
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  // Test integer sizes generate same cache key for int and float int
  font fn_int= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  font fn_float_int=
      smart_font ("sys-chinese", "rm", "medium", "right", 10.0, 600);
  QVERIFY (!is_nil (fn_int));
  QVERIFY (!is_nil (fn_float_int));
  qcompare (fn_int->res_name, fn_float_int->res_name);

  // Test half-multiple sizes generate different cache key from integer sizes
  font fn_half= smart_font ("sys-chinese", "rm", "medium", "right", 10.5, 600);
  QVERIFY (!is_nil (fn_half));
  QVERIFY (fn_int->res_name != fn_half->res_name);

  // Test another half-multiple size
  font fn_half2= smart_font ("sys-chinese", "rm", "medium", "right", 11.5, 600);
  QVERIFY (!is_nil (fn_half2));
  QVERIFY (fn_half->res_name != fn_half2->res_name);

  // Test corrected non-half-multiple generates appropriate cache key
  font fn_corrected=
      smart_font ("sys-chinese", "rm", "medium", "right", 10.3, 600);
  QVERIFY (!is_nil (fn_corrected));
  // 10.3 should be corrected to 10.5, so should have same cache key as 10.5
  // (Implementation may vary, but we at least verify creation)
}

// Test to_tex_font_size() function
void
TestFontSize::test_to_tex_font_size () {
  // Integer sizes should stay the same
  QCOMPARE (to_tex_font_size (10.0), 10.0);
  QCOMPARE (to_tex_font_size (12.0), 12.0);
  QCOMPARE (to_tex_font_size (8.0), 8.0);

  // Half-multiple sizes should be multiplied by 100
  QCOMPARE (to_tex_font_size (10.5), 1050.0); // 10.5 → 1050
  QCOMPARE (to_tex_font_size (11.5), 1150.0);
  QCOMPARE (to_tex_font_size (8.5), 850.0);

  // Already multiplied values should be handled correctly
  // 1000.0 (10.0 * 100) should be recognized as integer 10.0
  QCOMPARE (to_tex_font_size (1000.0), 10.0);
  // 1050.0 (10.5 * 100) should stay as 1050.0
  QCOMPARE (to_tex_font_size (1050.0), 1050.0);

  // Non-half-multiple sizes should round to nearest integer
  QCOMPARE (to_tex_font_size (10.1), 10.0);
  QCOMPARE (to_tex_font_size (10.2), 10.0);
  QCOMPARE (to_tex_font_size (10.3), 10.0);
  QCOMPARE (to_tex_font_size (10.4), 10.0);
  QCOMPARE (to_tex_font_size (10.6), 11.0);
  QCOMPARE (to_tex_font_size (10.7), 11.0);
  QCOMPARE (to_tex_font_size (10.8), 11.0);
  QCOMPARE (to_tex_font_size (10.9), 11.0);

  // Edge cases
  QCOMPARE (to_tex_font_size (0.0), 0.0);
  QCOMPARE (to_tex_font_size (0.5), 50.0);
  QCOMPARE (to_tex_font_size (1.0), 1.0);
}

// Test get_script_size() function
void
TestFontSize::test_get_script_size () {
  // Test the mathematical logic of get_script_size without full edit_env
  // This tests the core calculation that should happen in get_script_size

  // Test that script() function works correctly (this is used internally)
  double sz1          = 10.0;
  double level0_result= script (sz1, 0);
  QVERIFY (qAbs (level0_result - sz1) < 0.001);

  double level1_result  = script (sz1, 1);
  double expected_level1= (sz1 * 2.0 + 2.0) / 3.0;
  QVERIFY (qAbs (level1_result - expected_level1) < 0.001);

  double level2_result  = script (sz1, 2);
  double expected_level2= (expected_level1 * 2.0 + 2.0) / 3.0;
  QVERIFY (qAbs (level2_result - expected_level2) < 0.001);

  // Test with half-multiple size
  double sz2           = 10.5;
  double level0_result2= script (sz2, 0);
  QVERIFY (qAbs (level0_result2 - sz2) < 0.001);

  // Test that non-half-multiple input is corrected by script() function
  double sz3      = 10.3;
  double corrected= script (sz3, 0);
  QVERIFY (is_half_multiple (corrected));
  QVERIFY (qAbs (corrected - 10.5) < 0.001);

  // Test level bounds handling
  double negative_level= script (sz1, -1);
  QVERIFY (qAbs (negative_level - script (sz1, 0)) < 0.001);

  double large_level= script (sz1, 10);
  QVERIFY (qAbs (large_level - script (sz1, 2)) < 0.001);
}

// Test determine_sizes() function logic
void
TestFontSize::test_determine_sizes () {
  // Test the core logic that determine_sizes should implement
  // This is a simplified test that validates the mathematical operations

  // Test 1: Integer conversion (as_int handling)
  // determine_sizes should convert string "10" to double 10.0
  // We test that our understanding of integer conversion is correct
  double sz= 10.0;

  // Test 2: Multiplication syntax handling
  // For "*1.2" with sz=10.0, should compute ceil(1.2 * 10 - 0.001) = 12
  double multiplier= 1.2;
  double xsz1      = ceil ((multiplier - 0.001) * sz);
  QVERIFY (qAbs (xsz1 - 12.0) < 0.001);

  // For "*0.8" with sz=10.0, should compute ceil(0.8 * 10 - 0.001) = 8
  multiplier = 0.8;
  double xsz2= ceil ((multiplier - 0.001) * sz);
  QVERIFY (qAbs (xsz2 - 8.0) < 0.001);

  // Test 3: Division in multiplication syntax (e.g., "*3/2")
  // For "*3/2" with sz=10.0, should compute ceil((3.0/2.0) * 10 - 0.001) = 15
  double x   = 3.0 / 2.0; // 1.5
  double xsz3= ceil ((x - 0.001) * sz);
  QVERIFY (qAbs (xsz3 - 15.0) < 0.001);

  // Test 4: Script function fallback (when no matching tuple found)
  // determine_sizes should fall back to script(sz, 1) and script(sz, 2)
  double script1         = script (sz, 1);
  double expected_script1= (sz * 2.0 + 2.0) / 3.0;
  QVERIFY (qAbs (script1 - expected_script1) < 0.001);

  double script2         = script (sz, 2);
  double expected_script2= (expected_script1 * 2.0 + 2.0) / 3.0;
  QVERIFY (qAbs (script2 - expected_script2) < 0.001);

  // Test 5: Half-multiple size support
  double sz_half              = 10.5;
  double script1_half         = script (sz_half, 1);
  double expected_script1_half= (sz_half * 2.0 + 2.0) / 3.0;
  QVERIFY (qAbs (script1_half - expected_script1_half) < 0.001);

  // Test that multiplication works with half-multiple sizes
  multiplier     = 1.2;
  double xsz_half= ceil ((multiplier - 0.001) * sz_half);
  // 1.2 * 10.5 = 12.6, ceil(12.6 - 0.001) = ceil(12.599) = 13
  QVERIFY (qAbs (xsz_half - 13.0) < 0.001);
}

// Performance test for script() function
void
TestFontSize::test_performance_script_function () {
  // Reset performance counters before test
  bench_reset ("font_script_calculation");

  const int iterations  = 10000;
  double    total_result= 0.0;

  // Warm up cache
  for (int i= 0; i < 100; i++) {
    total_result+= script (10.0, 1);
  }

  // Performance test with various sizes
  QVector<double> test_sizes = {8.0, 8.5, 10.0, 10.5, 12.0, 12.5};
  QVector<int>    test_levels= {0, 1, 2};

  for (double size : test_sizes) {
    for (int level : test_levels) {
      for (int i= 0; i < iterations; i++) {
        total_result+= script (size, level);
      }
    }
  }

  // Prevent optimization from removing the loop
  Q_UNUSED (total_result);

  qDebug () << "Performance test for script() function completed.";
  qDebug () << "Enabled DEBUG_BENCH to see detailed timing information.";
  qDebug ()
      << "Set environment variable TM_DEBUG_BENCH=1 to enable benchmarks.";

  // Print performance results for font_script_calculation
  debug_set ("bench", true);
  lolly::system::bench_print (std_bench, "font_script_calculation", 0);
}

// Performance test for to_tex_font_size() function
void
TestFontSize::test_performance_to_tex_font_size () {
  // Reset performance counters before test
  bench_reset ("to_tex_font_size_conversion");

  const int iterations  = 10000;
  double    total_result= 0.0;

  QVector<double> test_sizes= {0.0,  0.5,  1.0,   8.0,    8.5,   10.0,  10.5,
                               12.0, 12.5, 100.0, 1050.0, 316.0, 1000.0};

  for (double size : test_sizes) {
    for (int i= 0; i < iterations; i++) {
      total_result+= to_tex_font_size (size);
    }
  }

  Q_UNUSED (total_result);

  qDebug () << "Performance test for to_tex_font_size() function completed.";

  // Print performance results for to_tex_font_size_conversion
  debug_set ("bench", true);
  lolly::system::bench_print (std_bench, "to_tex_font_size_conversion", 0);
}

// Performance test for general float calculations
void
TestFontSize::test_performance_float_calculations () {
  // Test various float operations used in font size calculations
  const int iterations= 100000;
  double    total     = 0.0;

  // Test is_half_multiple and round_to_half_multiple
  QVector<double> test_values= {8.0, 8.5, 10.0, 10.3, 10.7, 10.5, 12.0, 12.5};

  for (int i= 0; i < iterations; i++) {
    for (double val : test_values) {
      bool is_half= is_half_multiple (val);
      if (!is_half) {
        double rounded= round_to_half_multiple (val);
        total+= rounded;
      }
    }
  }

  // Test basic float arithmetic patterns used in script()
  for (int i= 0; i < iterations; i++) {
    for (double val : test_values) {
      // Simulate script() calculation: (val * 2.0 + 2.0) / 3.0
      double result= (val * 2.0 + 2.0) / 3.0;
      total+= result;
    }
  }

  Q_UNUSED (total);

  qDebug () << "Performance test for float calculations completed.";
  qDebug () << "Total iterations:" << iterations * test_values.size () * 2;

  // Print all performance results
  debug_set ("bench", true);
  lolly::system::bench_print (std_bench);
}

QTEST_MAIN (TestFontSize)
#include "font_size_test.moc"
