# Testing
There are two types of tests: unit tests and integration tests.

## Unit Tests
### C++ Unit Tests
| Method 1                       | Method 2                             | Description                    |
|-------------------------------|--------------------------------------|-------------------------------|
| bin/test_all                  | bash bin/test_all                    | Run all C++ tests.            |
| bin/test_only [target-name]   | bash bin/test_only [target-name]     | Run a single unit test only.  |

### Scheme Tests
To run Scheme tests:
+ Use `xmake run --yes -vD --group=scheme_tests` or `bin/test_all_scheme` to run all Scheme tests.

## Integration Tests
Integration test code and test documents are located in the `TeXmacs/tests` directory. Before running integration tests, you must build and install Mogan to a temporary directory:
+ Use `bin/test_all_integrated` or `bash bin/test_all_integrated` to run all integration tests.
+ Use `xmake run [target-name]` to run a single integration test, e.g. `xmake run 9_1`.
+ Use `xmake run --yes -vD --group=integration_tests` to run all integration tests.

## Listing All Targets
The following command lists all targets. Targets ending in `_test` are C++ unit tests; targets matching `[0-9]*_[0-9]*` are integration tests.
```shell
$ xmake show -l targets
11_36               converter_test         convert_test
12_1                tm_url_test            24_19
15_3_7              70_7                   data-test
43_15               15_3_5                 libmogan
15_3_3              generic-test           qt_utilities_test
smart_font_test     tm-define-test         66_7
environment-test    view_history_test      parse_variant_test
11_28               pdf_test              old-gui-test
minimal-test        math_test              xml_test
66_13               24_14                  image_files_test
cork-test           scheme-tools-test      11_4
9_1                 goldfish               keyword_parser_test
64_1                stem                   24_15
parsexml_test       tmlength-test          203_5
liii_packager       graphics-group-test    queryxml_test
tm-convert-test     71_41                  15_3_2
tm_file_test        201_6                  svg_parse_test
43_7                libmoebius             otmath_parse_test
regexp-test         env_length_test        15_3_1
url-test            43_14                  19_8
11_38               203_1                  menu-test
201_5
```
