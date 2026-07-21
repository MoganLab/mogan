# Loro 协作编辑集成（0760）

`tools/loro-server`（服务端）+ `src/Plugins/Collab`（会话层）+ ImGui File→Collaborative
菜单。动这块前先读，避免重踩下面的坑。完整设计见 `devel/0760.md`。

## 架构一句话

一个文档 = 一个 editor = 一条 WS 连接。**服务端不做 CRDT 合并**（只做生命周期/持久化/
同步下发），内容一致性由客户端 shadow（`loro_shadow`）的 CRDT 保证。客户端会话层
（`src/Plugins/Collab/loro_collab.cpp`）单例，操作一律对「当前编辑器」动态取用。

## ImGui 菜单：interactive 项默认被滤掉

`menu-widget.scm:1129` 的过滤：`(and (not (qt-gui?)) interactive? (not imgui-supported?))`
→ 跳过。**带 `(:argument ...)` 的命令被判为 interactive**，ImGui 菜单默认不显示，除非
声明 `:imgui-supported`。

**坑**：`(:imgui-supported #t)` 内联在 `tm-define` 里**不生效**（`property` 查不到）。必须
用**独立 `tm-property` 块**（与 `choose-file` 同款）：

```scheme
(tm-define (collab-join-document doc-id)
  (:argument doc-id "文档 UUID") ...)
(tm-property (collab-join-document doc-id)
  (:interactive #t)
  (:imgui-supported #t))
```

## 菜单元素不能返回非 widget（否则 SIGABRT "widget expected"）

`(when cond body)` 在菜单 DSL 展开为 `$assuming`，把 body 返回值当作菜单项。若 body 是
纯副作用（如 `(collab-refresh-docs)` 返回 unspecified），渲染时 `object_promise_widget_rep::eval`
抛 "widget expected" → 崩。

**副作用必须放进 `(with var (begin side-effect value) ...)` 的值表达式**，菜单体只留真正的
菜单项（if/cond 分支、`---`、叶子项）。参考 `collab-docs-menu`。

## GUI 线程上绝不能做同步网络（会 freeze）

ImGui（含 WASM 的 `emscripten_set_main_loop_arg`）每帧重建菜单。若菜单展开触发**同步**
HTTP，GUI 线程阻塞 → freeze（无输出无报错，最难调）。

- **native**：fetch 必须跑 `std::thread` 后台线程（worker 只碰 std 容器，**绝不触 lolly
  分配器**——`fast_alloc` 进程全局非线程安全），GUI 线程读结果时才转 `array<string>`。参考
  `loro_collab_fetch_docs` 的 native 分支 + `tm_curl_websocket_client` 的线程化模式。
- **WASM**：无线程，用浏览器异步 `fetch` + 回调。见下。

## WASM：不要用 `Module.*`，用 C++ 全局 + ccall 回调

第一版把 fetch 结果存 `Module.__collabDocs*` JS 全局、菜单每帧经 EM_JS getter 轮询 →
**展开 Join 子菜单即 freeze**。两个坑：

1. **`Module` 在 EM_JS 内可能不可见**（MODULARIZE 等配置）→ 抛未捕获异常 → 帧回调卡死。
2. **每帧 EM_JS 往返**在 GLFW 主循环里有开销/风险。

**正解**（对齐代码库 `im_clipboard` / `im_ime` 既有模式）：

- 状态存 **C++ 普通全局**（WASM 单线程，无需原子/锁），菜单每帧**只读 C++ 变量**，无 EM_JS 往返。
- JS `fetch` 完成经 **`ccall` 调 `EMSCRIPTEN_KEEPALIVE` 回调**写入 C++ 全局：
  ```cpp
  extern "C" EMSCRIPTEN_KEEPALIVE void mogan_collab_docs_received (const char* text, int ok) { ... }
  EM_JS_DEPS (mogan_collab, "$ccall");   // 把 ccall 拉入 EM_JS 作用域
  EM_JS (void, collab_fetch_docs_js, (const char* url), {
    try { fetch(...).then(...).then(t => ccall('mogan_collab_docs_received', null, ['number','number'], [buf,2])) ... }
    catch (e) { ccall('mogan_collab_docs_received', null, ['number','number'], [0,3]); }
  });
  ```
- EM_JS **全程 try/catch**，杜绝帧内未捕获异常。

## WASM 跨源 fetch 要服务端 CORS

浏览器从 WASM 页面 `fetch('http://server/docs')` 是跨源请求，无 CORS 头会被拦（Console
报 CORS 错，但 WASM 侧静默）。服务端 HTTP 响应统一加 `access-control-allow-origin: *`
+ 处理 OPTIONS 预检（见 `server.js` 的 `setCors`）。WS 连接不强制 CORS，能直连。

部署注意：WASM 页面若 `https://`，连 `ws://`/`http://` 会被 Mixed Content 拦截，服务端需
上 `wss://`/`https://`。WASM 无 OS 环境变量，`collab-server-url` 的 `system-getenv` 在
浏览器返回空 → 用默认 `ws://127.0.0.1:8765`，部署需改由页面 location/查询参数配置。

## 协作开关门控：会话建立前不 seed/不镜像

`edit_modify_rep` 有 `loro_collab_on` 开关。`ensure_loro_seeded`/`mirror_loro`/
`route_through_loro` 在 `!loro_collab_on` 时直接 return。**否则"打开即连、首次编辑同步
全文"**——旧 relay 时代的行为，不适合云文档。会话 `become_ready` 时调 `collab_enable()`
置位（对 `get_current_editor()` 动态取用）。

## 会话不持有固定 editor 句柄

曾把 `get_current_editor()` 在 `loro_collab_create` 时存进 `target_ed`，DOC 回复（async）
时 `target_ed->collab_enable()`。一旦该句柄失效/与用户实际编辑的 editor 不是同一对象，
`loro_collab_on` 没置位在正确的 editor → `mirror_loro` 全部短路 → **不广播**（症状：编辑
有 `[loro-mod]` 日志但无 `[Loro] Local update generated`）。

**正解**：`become_ready`/`on_message` 里一律 `get_current_editor()->...` 动态取用，不存
句柄。`[loro-mod]` 是 `edit_announce` 的日志（始终打印），**不代表 mirror 跑了**——排查
广播问题时以 `[collab] 编辑器协作开关已置位` / `[Loro] Local update generated` 为准。

## JOIN 空文档要超时兜底

JOIN 流程：DOC → `await_frame`（等首帧 snapshot/updates）→ 收到首帧才 `become_ready`。
但**空文档无历史可发**，服务端不发任何二进制帧 → 客户端永久卡 `await_frame` → 永不
`collab_enable` → 不广播。`poll` 里加超时：DOC 后 1s 内未收到帧就按空文档 `become_ready`。

## 服务端 seq 并发竞态

`ws.on('message')` 是 fire-and-forget，多条 update 并发进入 `applyUpdate`。若 `await
load()` 后再 `seq++`，并发交错会让所有调用读到同一 seq 起点。**所有共享状态（seq/影子/
字节计数）只能在 `persistQueue` 串行 Promise 链上触碰**——`applyUpdate` 同步入队，`await`
移到链内。否则 seq 永远到不了 snapshot 阈值。

## glue 声明源是 `.lua`，编辑器方法自动加前缀

新增 scheme 可调的 C++ 函数：
- **编辑器方法**（`edit_modify_rep` 等的成员）：加到 `src/Scheme/Glue/glue_editor.lua`，规则
  自动给所有调用加 `get_current_editor()->` 前缀——**自由函数不能绑编辑器方法**，要包一层。
- **自由函数**：加到 `src/Scheme/L5/glue_widget.lua`（`standalone=true`，无前缀）。

改了 `.lua` 要重新构建（glue 在构建期生成 `glue_*.cpp`）。**不要改 `build-glue-editor.scm` /
`glue-symbols.scm`**——mogan 不用那两个 texmacs 遗留文件。

## 调试入口

- `[collab]` 前缀：会话层（连接/DOC/become_ready/上行/mirror 门控）。
- `[Loro]` 前缀：shadow（local update 生成、apply_remote diff）。
- `[loro-mod]`（LIII_DEBUG）：edit_announce 的 mod 日志，**不代表 mirror 执行**。
- WASM 上 `cout` 可能不进浏览器 Console——排查网络用浏览器 DevTools 的 Network/Console。
- 服务端日志：每条 update 的 `seq`、广播人数；snapshot 截断时 `snapshot.bin` 落盘。

## 关键文件

| 文件 | 职责 |
|------|------|
| `src/Plugins/Collab/loro_collab.{hpp,cpp}` | 协作会话层（WS 状态机 + 文档发现） |
| `src/Edit/Modify/edit_modify.{hpp,cpp}` | `loro_collab_on` 门控 + `collab_enable` |
| `src/Plugins/WebSocket/{libcurl,emscripten}/` | WS 客户端（native 线程化 / WASM 浏览器原生） |
| `TeXmacs/progs/texmacs/texmacs/tm-collab.scm` | scheme 编排（菜单驱动 CREATE/JOIN） |
| `TeXmacs/progs/texmacs/menus/file-menu.scm` | File→Collaborative 菜单 |
| `tools/loro-server/` | 协作服务端（协议/持久化/同步/`/docs`/CORS） |
