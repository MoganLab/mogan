# Developing on Windows

## Using xmake
### Step 1: Installing xmake
Xmake can be installed either from scoop, or from standalone installer for windows.

It is recommended to use scoop, and other packages need to be installed in this way
```
scoop install xmake
scoop install make
scoop install git
```

Sometimes, it's necessary to ensure that xrepo repository is up-to-date to ensure the usage of the latest dependency build definitions:
```
xrepo update-repo
```

### Step 2: Installation and Usage
Taking xmake as an example:
#### Method 1: Local Installation
```
xmake config --yes
xmake build liblolly
xmake install --admin liblolly
```

#### Method 2: Adding xmake Dependency
In the library where lolly is required, add the following to xmake.lua:
```
add_requires("lolly", {system=false})
```

For a detailed example, refer to [mogan](https://github.com/XmacsLabs/mogan/blob/branch-1.2/misc/xmake/packages.lua).

### Step 3: Compiling and Testing
Refer to [How to Test](Test.md).