# 如何测试
对于开发者来说，一共有两种测试方式，其一为单元测试，其二为集成测试。

## 列出所有target
下面的命令行可以列出所有target，其中以`_test`结尾的是C++的单元测试。
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

## 单元测试
如上面显示，单元测试形如xxx_test，可以运行下列指令进行测试（运行前请进行构建，详细见[xmake](https://xmake.io)）：

运行单个单元测试：
```
xmake run xxx_test
bin\test_only.bat xxx_test # on Windows
bin/test_only xxx_test     # on Linux/macOS
```

运行所有单元测试：
```
xmake run --group=tests
```

