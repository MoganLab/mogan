# Code Formatting Guide

## Environment Setup

Requires clang-format version 19.1.x.

### Linux

Install via apt:
```bash
sudo apt install clang-format-19
```

After restarting the terminal, verify:
```bash
clang-format-19 --version
```

### macOS

Note: should match LLVM 19.

Install via Homebrew:
```bash
brew install llvm@19
```

Add to your ~/.zshrc:
```bash
# set clang-format to version 19
export PATH="$(brew --prefix llvm@19)/bin:$PATH"
```

Restart the terminal and verify:
```bash
clang-format --version
```

### Windows

Install via scoop:
```bash
scoop install llvm@19.1.0
```

Restart the terminal and verify:
```bash
clang-format --version
```

## Using the script

From the repository root, run:
```shell
elvish bin/format
```
