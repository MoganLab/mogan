# Developing on GNU/Linux

## Using xmake
### Step 1: Installing xmake
Using Debian and its derivatives as an example:
```
sudo add-apt-repository ppa:xmake-io/xmake
sudo apt install xmake
```

Sometimes, it's necessary to ensure that xrepo is up-to-date to ensure the usage of the latest dependency build definitions:
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