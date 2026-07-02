/** \file base64_test.cpp
 *  \copyright GPLv3
 *  \details Unitests for base64
 *  \author charonxin
 *  \date   2023
 */
#include "a_lolly_test.hpp"
#include "lolly/data/base64.hpp"

using lolly::data::decode_base64;
using lolly::data::encode_base64;

TEST_CASE ("encode base64") {
  CHECK_EQ (encode_base64 ("") == "", true);
  CHECK_EQ (encode_base64 ("f") == "Zg==", true);
  CHECK_EQ (encode_base64 ("fo") == "Zm8=", true);
  CHECK_EQ (encode_base64 ("foo") == "Zm9v", true);
  CHECK_EQ (encode_base64 ("foob") == "Zm9vYg==", true);
  CHECK_EQ (encode_base64 ("fooba") == "Zm9vYmE=", true);
  CHECK_EQ (encode_base64 ("foobar") == "Zm9vYmFy", true);
  CHECK_EQ (decode_base64 ("Zg==") == "f", true);
  CHECK_EQ (decode_base64 ("Zm8=") == "fo", true);
  CHECK_EQ (decode_base64 ("Zm9v") == "foo", true);
  CHECK_EQ (decode_base64 ("Zm9vYg==") == "foob", true);
  CHECK_EQ (decode_base64 ("Zm9vYmE=") == "fooba", true);
  CHECK_EQ (decode_base64 ("Zm9vYmFy") == "foobar", true);
}

TEST_CASE ("decode base64") {
  CHECK_EQ (decode_base64 ("") == "", true);
  CHECK_EQ (decode_base64 ("Zg==") == "f", true);
  CHECK_EQ (decode_base64 ("Zm8=") == "fo", true);
  CHECK_EQ (decode_base64 ("Zm9v") == "foo", true);
  CHECK_EQ (decode_base64 ("Zm9vYg==") == "foob", true);
  CHECK_EQ (decode_base64 ("Zm9vYmE=") == "fooba", true);
  CHECK_EQ (decode_base64 ("Zm9vYmFy") == "foobar", true);
}