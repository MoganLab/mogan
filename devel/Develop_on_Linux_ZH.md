# 在GNU/Linux上开发

## 使用xmake
### 第一步：安装xmake和相关依赖
以Debian及衍生版为例：

由于本项目使用xmake管理qt,所以需要将本地qt移除
``` bash
sudo apt remove qt6-base-dev qt6-base-dev-tools qmake6 
``` 

若qt依赖有问题，可以使用该命令删除xmake的qt全局缓存
``` bash
rm -rf ~/.xmake/cache ~/.xmake/packages/qt
```

安装xmake和相关依赖
``` bash
sudo apt install xmake

sudo apt update
sudo apt install -y gcc git 7zip unzip curl build-essential fonts-noto-cjk libcurl4-openssl-dev libfreetype-dev libfontconfig-dev libmimalloc-dev libgit2-dev zlib1g-dev libssl-dev libjpeg62-turbo-dev cmake pandoc xmake python3.13 ninja-build libdbus-1-3 libglib2.0-0t64 libegl1 libgl-dev libxkbcommon0
```

有时候需要保持xrepo是最新的，以保证使用最新的依赖的构建定义
``` bash
xrepo update-repo
```

### 第二步：编译
-vD 会显示具体debug日志，qt的安装缓存：/home/username/.xmake/packages
比如 /home/jiadong/.xmake/packages/q/qt6base/6.8.3/7c1ea54729db483fa6eee7744bd4a333
``` bash
xmake config --yes -vD
xmake build stem
```

### 第三步：测试
见[如何测试](Test_ZH.md)。

### 第四步：启动墨干
``` bash
xmake run stem
```

### 某些情况下需要清除缓存（比如菜单栏消失等情况）
缓存目录
``` bash
rm -rf ~/.cache/MoganLab 
```
运行中的缓存
``` bash
rm -rf ~/.local/share/moganlab
```

## 使用VSCode帮助代码补全
安装VSCode，Clangd以及VSCode的Clangd插件。
之后在mogan文件夹执行
``` bash
xmake project -k compile_commands
```
这个命令会在mogan文件夹下生成一个compile_commands.json文件，Clangd会读取它以理解项目的组织结构，从而避免找不到头文件的错误。

## commit前请格式化
[格式化指南](Format_ZH.md)