# 在 Windows 上开发

## 使用 xmake

## 第一步：安装工具与依赖

### 安装 Visual Studio 2022 (MSVC 14.44.35211)
- 安装工作负载：**“使用 C++ 的桌面开发”**  
- 勾选：
  - **MSVC 14.44.35211**
  - **Windows 10/11 SDK**
- 打开 VS Installer，选择 **Desktop development with C++**
- 勾选 **MSVC、Windows 10/11 SDK**
- 安装完成后，启动一次 VS 以初始化组件

### 安装 xmake
### 推荐使用 **scoop** 或 **choco** 安装。任选其一：

- **scoop：**
  ```powershell
  scoop install xmake
  ```
- **chocolatey**：
  
  ```powershell
  choco  install xmake
  ```
- 验证：
  ```powershell
  xmake --version
  ```
## 第二步：获取源码进行编译运行
### 选择你自己的工作目录，例如 E:\TestFile：

```powershell
cd E:\TestFile
git clone https://github.com/MoganLab/mogan.git
cd mogan
```

### 使用 xmake 配置与构建

### xmake 项目配置

```powershell
xmake config --yes -vD -m releasedbg --plat=windows
```

### 之前构建失败，先清理旧缓存

```powershell
xmake f -c
xmake c -a
```

### 编译项目

```powershell
xmake build stem
```

### 安装目标

```powershell
xmake install stem
```

### 运行程序

```powershell
xmake run stem
```

## commit前请格式化
[格式化指南](Format_ZH.md)