# 在Windows上开发

## 使用xmake
### 步骤1：安装xmake
xmake可以通过scoop或Windows的独立安装程序安装。

建议使用scoop，并以以下方式安装其他包
```
scoop install xmake
scoop install make
scoop install git
```

有时，需要xrepo仓库是最新的，以确保使用最新的依赖项构建定义：
```
xrepo update-repo
```

### 步骤2：安装和使用
以xmake为例：
#### 方法1：本地安装
```
xmake config --yes
xmake build liblolly
xmake install --admin liblolly
```

#### 方法2：添加xmake依赖
在需要lolly的库中，将以下内容添加到xmake.lua：
```
add_requires("lolly", {system=false})
```

有关详细示例，请参阅[mogan](https://github.com/XmacsLabs/mogan/blob/branch-1.2/misc/xmake/packages.lua)。

### 步骤3：编译和测试
请参阅[如何测试](Test.md)。