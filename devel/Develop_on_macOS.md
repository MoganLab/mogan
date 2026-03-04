# macOS Development Guide
This development guide is written for Mogan Research. Other components can also refer to this guide.

## Step 1: Install xmake and xrepo
For Homebrew users:
```bash
brew install xmake qt
brew install pkg-config
```

Keep xrepo updated:
```bash
xrepo update-repo
```

## Step 2: Configuration
```bash
xmake config -vD --yes 
```

If you encounter Qt dependency issues, you can use this command to delete xmake's Qt global cache:
```bash
rm -rf ~/.xmake/cache ~/.xmake/packages/qt
```

If Qt cannot be found, you can manually specify the Qt path, for example:
```bash
xmake config --qt=/opt/homebrew/share/qt
```
Please adjust the Qt-related directory as needed; it may not necessarily be `/opt/homebrew/share/qt`.

## Step 3: Build
```bash
xmake build stem
```

## Step 4: Testing
Refer to [How to Test](Test_EN.md)

## Step 5: Launch Mogan STEM
```bash
xmake run stem
```

## Please format code before committing
[Formatting Guide](Format_EN.md)
