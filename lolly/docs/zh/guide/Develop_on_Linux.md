# 在GNU/Linux上开发

## 使用xmake
### 第一步：安装xmake
以Debian及衍生版为例：
```
sudo add-apt-repository ppa:xmake-io/xmake
sudo apt install xmake
```

有时候需要保持xrepo是最新的，以保证使用最新的依赖的构建定义
```
xrepo update-repo
```

### 第二步：安装与使用
以xmake为例：
#### 方法一：安装到本地
```
xmake config --yes
xmake build liblolly
xmake install --admin liblolly
```

#### 方法二：添加xmake依赖
在需要用lolly的库中,在xmake.lua中添加：
```
add_requires("lolly", {system=false})
```
详细样例可见[mogan](https://github.com/XmacsLabs/mogan/blob/branch-1.2/misc/xmake/packages.lua)。


### 第三步：编译测试
见[如何测试](Test.md)。