# How to Perform Testing
For developers, there are two testing methods available: unit testing and integration testing.

## Listing all Targets
The following command line can list all targets. Targets ending with `_test` are C++ unit tests.
``` shell
$ xmake show -l targets
path_test            ntuple_test          fast_search_test
base64_test          blackbox_test        hashset_test
tm_timer_test        string_test          fast_alloc_test
promise_test         liblolly             array_test
url_test             sys_utils_test       hashmap_test
curl_test            iterator_test        hashfunc_test
hashtree_test        tm_ostream_test      list_test
modification_test    parse_string_test    generic_tree_test
analyze_test         rel_hashmap_test     tree_test
```

## Unit Tests
As shown above, unit tests are in the format of xxx_test. You can run the following command to perform the tests (ensure you have built the code beforehand, see [xmake](https://xmake.io) for details):

Run only one test:
```
xmake run xxx_test
bin\test_only.bat xxx_test # on Windows
bin/test_only xxx_test     # on Linux/macOS
```

Run all tests:
```
xmake run --group=tests
```