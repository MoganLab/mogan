#include <Velopack.hpp>
#include <cstdlib>
#include <iostream>

// 仅验证 Velopack C++ runtime 启动钩子可编译/链接；未安装环境下 Run() 为空操作。
int main () {
#if defined (_WIN32) && defined (_M_X64)
  try {
    Velopack::VelopackApp::Build ().Run ();
    std::cout << "velopack startup hook: ok\n";
  } catch (const std::exception &e) {
    std::cerr << "velopack startup hook failed: " << e.what () << "\n";
    return 1;
  }
#endif
  return 0;
}
