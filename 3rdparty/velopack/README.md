# Velopack libc（C/C++ runtime）

来源：https://github.com/velopack/velopack GitHub release `1.2.0`，资产 `velopack_libc_1.2.0.zip`。

许可证：MIT（见上游仓库 LICENSE）。

当前仅 vendor Windows x64 资产：
- `include/Velopack.h`、`include/Velopack.hpp`：官方 C/C++ 头文件（C++ 为 C API 的薄封装）。
- `lib/velopack_libc_win_x64_msvc.dll`：动态库本体。
- `lib/velopack_libc_win_x64_msvc.dll.lib`：MSVC 导入库。

macOS / Linux / Windows arm64 等资产后续按平台补充（zip 内其余文件未 vendor）。
