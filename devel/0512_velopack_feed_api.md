# Mogan Velopack 更新源后端 API 设计（审阅稿）

> 本文件是 **后端更新源（feed 服务）的 API 设计文档**，供人工审阅。
> 按当前决策，Mogan 的 Velopack 迁移 **不实现** 自定义后端；首选静态文件托管。
> 只有在需要鉴权、灰度、临时 URL 或商业版权限控制时才需要本设计中的"受控入口"。

## 1. 决策：静态 feed 优先

Velopack 客户端通过 `UpdateManager(url)` 读取 `releases.<channel>.json`，再下载其中列出的完整包
与 delta 包。静态文件托管（对象存储 / GitHub Releases / 任意 HTTPS 静态站）即可满足 stable 渠道：

```
https://updates.mogan.app/mogan/windows-x64/stable/releases.stable.json
https://updates.mogan.app/mogan/windows-x64/stable/mogan-1.2.0-full.nupkg
https://updates.mogan.app/mogan/windows-x64/stable/mogan-1.1.0-1.2.0-delta.nupkg
```

- 平台/架构/渠道目录隔离，各平台互不影响。
- 资产不可覆盖（每次发布版本号唯一，文件一旦发布不改写）。
- 传输层 HTTPS，下载源仅需匿名 GET；写权限由发布账号最小化控制。

## 2. 静态 feed 语义

`releases.<channel>.json` 由 `vpk pack` 生成（或 `vpk upload` 上传），结构与 Velopack 官方一致：

```json
{
  "baseUrl": "https://updates.mogan.app/mogan/windows-x64/stable",
  "channel": "stable",
  "releases": [
    {
      "version": "1.2.0",
      "runtime": "win-x64",
      "publishedAt": "2026-08-07T00:00:00Z",
      "files": [
        { "type": "full", "url": "mogan-1.2.0-full.nupkg", "sha1": "...", "size": 0 },
        { "type": "delta", "url": "mogan-1.1.0-1.2.0-delta.nupkg", "sha1": "...", "size": 0 }
      ]
    }
  ]
}
```

- 每个 release 至少一个 `full` 文件；`delta` 可缺省。
- 客户端（Velopack C++ runtime）本地校验 SHA-1/SHA-256 后应用，失败自动回退 full。
- 渠道只启用 `stable`；`beta`/`canary` 在稳定流程验证后再增加。

## 3. 受控入口（可选，未实现）

仅在需要以下能力时启用，且由后端团队决定是否实现：

| 能力 | 说明 |
| --- | --- |
| 鉴权 | 客户端携带令牌获取 feed 与资产签名 URL，防止未授权抓取 |
| 灰度 | 按版本/用户分桶返回不同 `releases.<channel>.json` |
| 临时 URL | 资产经短期签名 URL 下发，feed 不暴露长期静态路径 |
| 商业版权限 | 订阅/许可校验后放行更新 |

### 3.1 约定的 HTTP 契约（草案）

```
GET /mogan/{platform}-{arch}/{channel}/feed?client=<mogan-version>&user=<id>&token=<jwt>
-> 200 application/json  返回 releases.<channel>.json（受控内容）
-> 403                 无权限 / 灰度未命中
-> 409                 版本被拒绝（降级 / 已撤回）

GET /mogan/{platform}-{arch}/{channel}/download/{file}?token=<jwt>
-> 200 application/octet-stream  资产（或 302 跳转到临时 URL）
-> 404                 资产不存在或已撤回
```

- 响应头 `ETag`/`Cache-Control: no-cache`，客户端每次启动检查都拉取最新 feed。
- 下载端点必须校验版本单调性：不允许 `version <= 当前已装版本` 的资产被提供为更新。
- 已撤回版本在 feed 中标记，客户端拒绝安装并提示原因。

### 3.2 一致性要求

- feed 内容与资产强一致：发布流水线先上传资产、再原子切换 feed。
- 每个平台渠道的 feed 独立；跨平台不得混用 packId 或版本序列。
- 若采用受控入口，客户端不得先请求自定义 JSON 再自行拼接 Velopack 资产路径——
  Velopack `UpdateManager` 只能消费官方 feed 语义，自定义逻辑应实现为 C++ custom update source。

## 4. 安全与威胁模型

- 下载源 HTTPS、发布账号最小权限、资产不可覆盖。
- feed 与资产提供 SHA-1/SHA-256 完整性校验；客户端校验失败即丢弃。
- 若威胁模型要求"feed 自身独立签名"（防篡改 feed 诱骗下载旧版/恶意包），
  接入前必须实现签名 feed 或 custom update source，不能只依赖 URL 保密。
- 版本单调性：拒绝版本倒退与同版本覆盖；受控回退须显式走发布流程。

## 5. 后端实施范围（当前不做）

- 不实现鉴权服务、灰度服务、签名 URL 服务。
- 不实现自定义 `IUpdateSource`/C++ custom update source。
- 生产发布先以静态 feed + HTTPS 对象存储上线，稳定后再评估受控入口。
