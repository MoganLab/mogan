// [1265] R2: 复现 [1212] 9deb90253c 引入的 orthogonalize 混维度越界读
// moebius/Kernel/Types/point.cpp orthogonalize():
//   n = min(N(p2), N(p1))  → i 分配 n 维（array 容量按 2 的幂取整 → 64）
//   m = min(N(p3), N(p1))  → 循环按 m 跑，c += d[k] * i[k]
// 构造 N(p1)=100, N(p2)=34, N(p3)=100 → n=34（容量 64，512B ≥
// MAX_FAST=264 走 safe_malloc），m=100 → i[64..99] 越界 384B，
// ASan 报 heap-buffer-overflow READ。
// 构建/运行（需 ASan 才能观察到越界；常规构建下为静默脏读）：
//   MOE=moebius; LOLLY=lolly
//   INC="-I$MOE/Data/Convert -I$MOE/Data/Tree -I$MOE/Kernel/Types
//     -I$MOE/Kernel/Abstractions -I$MOE/Scheme -I$MOE
//     -I$LOLLY/Kernel/Abstractions -I$LOLLY/Kernel/Algorithms
//     -I$LOLLY/Kernel/Containers -I$LOLLY/Kernel/Types
//     -I$LOLLY/Data/String -I$LOLLY/System/Classes -I$LOLLY/System/Files
//     -I$LOLLY/System/IO -I$LOLLY/System/Language -I$LOLLY/System/Memory
//     -I$LOLLY/System/Misc -I$LOLLY/Plugins/Unix -I$LOLLY/Plugins
//     -I$LOLLY/lolly/data -I$LOLLY"
//   g++ -std=c++17 -g -fsanitize=address -c devel/1265_3.cpp -o /tmp/1265_3.o $INC
//   g++ -std=c++17 -g -fsanitize=address -c moebius/Kernel/Types/point.cpp
//       -o /tmp/1265_3_point.o $INC
//   g++ -fsanitize=address /tmp/1265_3.o /tmp/1265_3_point.o
//       -o /tmp/1265_3 -Lbuild/linux/x86_64/releasedbg -lmoebius -llolly -lm -lpthread
//   /tmp/1265_3   # → AddressSanitizer: heap-buffer-overflow READ of size 8
//                 #   at point.cpp:278 (c += d[k] * i[k])
#include "point.hpp"
#include <cstdio>

int
main () {
  point p1 (100), p2 (34), p3 (100);
  p2[0]= 1.0; // 非退化、非共线：p2=p1+e1, p3=p1+e2
  p3[1]= 1.0;
  point i, j;
  bool ok= orthogonalize (i, j, p1, p2, p3);
  printf ("orthogonalize returned %d, N(i)=%d, N(j)=%d\n", (int) ok,
          N (i), N (j));
  return ok ? 0 : 1;
}
