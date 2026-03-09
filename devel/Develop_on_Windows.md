# Windows Development Guide

## Using xmake 

## Step 1: Install Tools and Dependencies

### Install Visual Studio 2022 (MSVC 14.44.35211)
- Install workload: "Desktop development with C++"
- Check:
  - **MSVC 14.44.35211**
  - **Windows 10/11 SDK**
  - After installation, launch VS once to initialize components

### Install xmake 
### Recommended to install using scoop or choco. 

- **scoop：**
  ```powershell
  scoop install xmake
  ```
- **chocolatey**：
  
  ```powershell
  choco install xmake
  ```
- Verify:
  ```powershell
  xmake --version
  ```
## Step 2: Get Source Code, Compile and Run
### Choose your working directory, for example E:\TestFile:

```powershell
cd E:\TestFile
git clone https://github.com/MoganLab/mogan.git
cd mogan
```

### Configure and Build with xmake

### xmake project configuration

```powershell
xmake config --yes -vD -m releasedbg --plat=windows
```

### If previous build failed, clean old cache first

```powershell
xmake f -c
xmake c -a
```

### Compile

```powershell
xmake build stem
```

### Install target

```powershell
xmake install stem
```

### Launch Mogan STEM

```powershell
xmake run stem
```

## Please format code before committing
[Formatting Guide](Format_EN.md)
