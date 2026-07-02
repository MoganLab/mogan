# 如何格式化代码
使用 Elvish 执行（需要确保你已经安装elvish）：
```
elvish bin/format
```
## clang-format
将 clang-format 固定在 `16.0.x` 版本。

### macOS
```
brew install clang-format
```
### Windows
```
scoop install llvm@16.0.6
```

### Linux
方法一：
你可以使用xrepo来安装llvm，但有时编译需要很长时间。
```
xrepo install "llvm==16.0.5"
```

方法二：
你可以直接安装clang-format。
以ubuntu22.04为例：

步骤1：获取密钥。
```
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
```

步骤2：将来自https://apt.llvm.org/ 的相应Ubuntu版本的源代码插入到系统中。
```
sudo vim /etc/apt/sources.list
```

步骤3：安装clang-format-16。
```
sudo apt update
sudo apt install clang-format-16
```