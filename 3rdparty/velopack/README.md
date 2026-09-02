# Velopack libc（C/C++ runtime）

来源：https://github.com/velopack/velopack GitHub release `1.2.0`，资产 `velopack_libc_1.2.0.zip`。

许可证：MIT（见上游仓库 LICENSE）。

当前 vendor 的资产：

- Windows x64：
  - `include/Velopack.h`、`include/Velopack.hpp`：官方 C/C++ 头文件（C++ 为 C API 的薄封装）。
  - `lib/velopack_libc_win_x64_msvc.dll`：动态库本体。
  - `lib/velopack_libc_win_x64_msvc.dll.lib`：MSVC 导入库。
- macOS（universal x86_64 + arm64）：
  - `lib/libvelopack_libc.dylib`：来自 zip 内 `lib/velopack_libc_osx.dylib`，经下述三步
    预处理后按 `lib` 前缀命名（见「macOS 预处理」）。`lib-static/` 下的 `.a` 未 vendor：
    dylib 已预链 Rust 依赖，免去手动补 framework/link 参数。

Linux / Windows arm64 等资产后续按平台补充（zip 内其余文件未 vendor）。

## macOS 预处理

上游 dylib 直接链接会产生两个问题，vendor 时已做如下处理（复现命令）：

1. 上游 install id 是 CI 构建机的绝对路径（`/Users/runner/work/...`），直接链接会把
   该路径写进可执行文件的依赖表，运行时找不到库。改名 + 重写 id 为 `@rpath/`：
   ```bash
   mv libvelopack_libc_osx.dylib libvelopack_libc.dylib   # lib 前缀使 -lvelopack_libc 可解析
   install_name_tool -id @rpath/libvelopack_libc.dylib libvelopack_libc.dylib
   ```
   加载名必须与磁盘文件名一致（loader 按 install id 的文件名在各 rpath 目录查找），
   故 id 带 lib 前缀，不能对齐 Windows 的无前缀 `velopack_libc.dll`。
2. `install_name_tool` 改 id 会使原有 linker-signed 签名失效，arm64 上签名损坏的
   dylib 会被 AMFI 拒载，必须 ad-hoc 重签：
   ```bash
   codesign --force --sign - --timestamp=none libvelopack_libc.dylib
   ```

注意：与 Windows 的改名时机不同——Windows 在安装期把 DLL 改名为
`velopack_libc.dll`（见 `xmake/targets/stem.lua`），macOS 因链接期/运行期文件名
必须与磁盘文件一致，只能在 vendor 时改好提交。

dylib 的 `LC_BUILD_VERSION` minos 为 11.0，低于应用自身要求，无兼容性约束。
