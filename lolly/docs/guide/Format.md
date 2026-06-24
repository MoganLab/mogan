# How to format the code
use elvish to execute(you need install elvish before):
```
elvish bin/format
```
## clang-format
Pin clang-format to `16.0.x`.

### macOS
```
brew install clang-format
```
### Windows
```
scoop install llvm@16.0.6
```

### Linux
Method 1:
You could install llvm by xrepo, but sometimes it takes a lot of time to compile.
```
xrepo install "llvm==16.0.5"
```

Method 2:
You could install clang-format directly.
Take ubuntu22.04 as example:

Step1 : get the key.
```
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
```

Step2 : insert the source code for the corresponding Ubuntu version from https://apt.llvm.org/ into your system.
```
sudo vim /etc/apt/sources.list
```

Step3 : install clang-format-16.
```
sudo apt update
sudo apt install clang-format-16
```