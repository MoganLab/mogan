# GNU/Linux Development Guide

## Using xmake
### Step 1: Install xmake and related dependencies
On Debian or Debian derivatives:

Since the project uses xmake to manage Qt, you need to remove the local Qt installation:
```bash
sudo apt remove qt6-base-dev qt6-base-dev-tools qmake6 
```

If you encounter Qt dependency issues, you can use this command to delete xmake's Qt global cache:
```bash
rm -rf ~/.xmake/cache ~/.xmake/packages/qt
```

Install xmake and related dependencies:
```bash
sudo apt install xmake

sudo apt update
sudo apt install -y gcc git 7zip unzip curl build-essential fonts-noto-cjk libcurl4-openssl-dev libfreetype-dev libfontconfig-dev libmimalloc-dev libgit2-dev zlib1g-dev libssl-dev libjpeg62-turbo-dev cmake pandoc xmake python3.13 ninja-build libdbus-1-3 libglib2.0-0t64 libegl1 libgl-dev libxkbcommon0
```

Keep xrepo updated:
```bash
xrepo update-repo
```

### Step 2: Compile
-vD shows detailed debug logs. Qt installation cache is located at: /home/username/.xmake/packages
For example: /home/jiadong/.xmake/packages/q/qt6base/6.8.3/7c1ea54729db483fa6eee7744bd4a333
```bash
xmake config --yes -vD
xmake build stem
```

### Step 3: Testing
Refer to [How to Test](Test_EN.md)

### Step 4: Launch Mogan STEM
```bash
xmake run stem
```

### In some case, clearing the cache may be necessary (eg., when manu bar disappears, etc.)
Cache directory:
```bash
rm -rf ~/.cache/MoganLab 
```
Runtime cache:
```bash
rm -rf ~/.local/share/moganlab
```

### Using VSCode for code completion assistance
Install VSCode, Clangd, and the Clangd extension for VSCode.
Then execute the following command in the mogan folder:
```bash
xmake project -k compile_commands
```
This command will generate a compile_commands.json file in the mogan folder. Clangd reads this file to understand the project's structure, thereby avoiding errors related to missing header files.

## Please format code before committing
[Formatting Guide](Format_EN.md)
