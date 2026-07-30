//! Mogan ↔ Loro CRDT 的最小 C FFI shim。
//!
//! Loro 官方只提供 UniFFI 绑定（Swift/Python 等），无现成 C 接口，故本 crate 自行
//! 暴露一组 `extern "C"` 函数。FFI 边界用**粗粒度字节接口**：C++ 侧把 moebius 文档
//! 树的中性 IR（`loro_ir_node`）用扁平二进制编码喂进来，本层据此构建 LoroDoc 的
//! LoroTree（可移动树），导出 snapshot 字节；反向 import snapshot 后遍历 LoroTree
//! 输出同一编码。这样边界只有 2 个转换函数 + 1 个 free，无需 TreeID 跨界。
//!
//! # IR 扁平编码（小端）
//! ```text
//! node := kind:u8  label_len:u32 label:bytes  text_len:u32 text:bytes
//!         n_children:u32  node × n_children
//! ```
//! - `kind`：0 = atomic，1 = compound，2 = generic（与 C++ 侧 loro_ir.hpp 一致）
//! - 节点映射：compound/generic 写 `label`；atomic 写 `text`；`kind` 字段始终写入
//!
//! 每个节点的元数据存在其关联 LoroMap 上（key: `kind`/`label`/`text`）。
//! 
//! Author: JimZhouZZY & Claude
//! Date  : Thu Jul 16, 2026
//! Copyright (C) 2026 Liii Network
//!
//! This program is free software: you can redistribute it and/or modify
//! it under the terms of the GNU General Public License as published by
//! the Free Software Foundation, either version 3 of the License, or
//! (at your option) any later version.
//!
//! This program is distributed in the hope that it will be useful,
//! but WITHOUT ANY WARRANTY; without even the implied warranty of
//! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//! GNU General Public License for more details.
//!
//! You should have received a copy of the GNU General Public License
//! along with this program. If not, see <https://www.gnu.org/licenses/>.
//!

use std::collections::HashMap;
use std::sync::{Arc, Mutex, OnceLock};

use loro::{
    Container, ContainerTrait, ExportMode, LoroDoc, LoroMap, LoroMovableList,
    LoroText, LoroTree, Subscription, TreeID, TreeParentId, ValueOrContainer,
};
use loro::cursor::{Cursor, Side};

// doc 句柄 -> 其 local-update 订阅。Subscription 必须保活，否则自动取消订阅。
static LOCAL_SUBS: OnceLock<Mutex<HashMap<usize, Subscription>>> = OnceLock::new();
fn local_subs() -> &'static Mutex<HashMap<usize, Subscription>> {
    LOCAL_SUBS.get_or_init(|| Mutex::new(HashMap::new()))
}

// doc 句柄 -> 手动广播的 vv 水位（mogan_loro_doc_export_local_update 专用）。
// subscribe_local_update 只发送每个 commit 事务内的 op：seed 的 create ops 在
// 首个 commit 里被发成"仅创建"骨架，文本丢失。故 seed 后需用本函数导出
// "自上次广播以来"的完整增量（含 create+文本），经 C++ 侧保存的回调送出。
static EXPORT_VVS: OnceLock<Mutex<HashMap<usize, loro::VersionVector>>> = OnceLock::new();
fn export_vvs() -> &'static Mutex<HashMap<usize, loro::VersionVector>> {
    EXPORT_VVS.get_or_init(|| Mutex::new(HashMap::new()))
}

const KIND_ATOMIC: u8 = 0;
const KIND_COMPOUND: u8 = 1;
const KIND_GENERIC: u8 = 2;

/// LoroTree 容器名（C++ 侧也用同名 root tree）
const TREE_NAME: &str = "tree";

/// 文本节点 meta 里存放 SPLIT 结构边界 marker 的 MovableList 键名。
/// 每条 marker 是一个元素，value 为其在文本 LoroText 中边界的稳定位置
/// （`Cursor` postcard 字节）。元素自身有 CRDT 身份（元素 Cursor 编码），
/// 只增删、不动 LoroText 字符，故 SPLIT/JOIN 不破坏字符身份。
const SPLIT_KEY: &str = "__split__";

// =============================================================================
// Rust 侧 IR 节点
// =============================================================================

struct IrNode {
    kind: u8,
    label: Vec<u8>,
    text: Vec<u8>,
    children: Vec<IrNode>,
}

// =============================================================================
// 扁平编码：Reader / Writer
// =============================================================================

struct Reader<'a> {
    buf: &'a [u8],
    pos: usize,
}

impl<'a> Reader<'a> {
    fn u8(&mut self) -> Result<u8, ()> {
        match self.buf.get(self.pos) {
            Some(&v) => {
                self.pos += 1;
                Ok(v)
            }
            None => Err(()),
        }
    }

    fn u32(&mut self) -> Result<u32, ()> {
        if self.pos + 4 > self.buf.len() {
            return Err(());
        }
        let mut arr = [0u8; 4];
        arr.copy_from_slice(&self.buf[self.pos..self.pos + 4]);
        self.pos += 4;
        Ok(u32::from_le_bytes(arr))
    }

    fn bytes(&mut self, n: usize) -> Result<&'a [u8], ()> {
        if self.pos + n > self.buf.len() {
            return Err(());
        }
        let s = &self.buf[self.pos..self.pos + n];
        self.pos += n;
        Ok(s)
    }

    /// 读一个 length-prefixed 字节串（保留原始字节，不做 UTF-8 lossy 转换——
    /// atomic 内容可能是图片等任意二进制）
    fn bytes_field(&mut self) -> Result<Vec<u8>, ()> {
        let n = self.u32()? as usize;
        let b = self.bytes(n)?;
        Ok(b.to_vec())
    }

    fn node(&mut self) -> Result<IrNode, ()> {
        let kind = self.u8()?;
        let label = self.bytes_field()?;
        let text = self.bytes_field()?;
        let n = self.u32()? as usize;
        let mut children = Vec::with_capacity(n);
        for _ in 0..n {
            children.push(self.node()?);
        }
        Ok(IrNode {
            kind,
            label,
            text,
            children,
        })
    }
}

struct Writer {
    buf: Vec<u8>,
}

impl Writer {
    fn u8(&mut self, v: u8) {
        self.buf.push(v);
    }

    fn u32(&mut self, v: u32) {
        self.buf.extend_from_slice(&v.to_le_bytes());
    }

    fn bytes_field(&mut self, b: &[u8]) {
        self.u32(b.len() as u32);
        self.buf.extend_from_slice(b);
    }

    fn node(&mut self, n: &IrNode) {
        self.u8(n.kind);
        self.bytes_field(&n.label);
        self.bytes_field(&n.text);
        self.u32(n.children.len() as u32);
        for c in &n.children {
            self.node(c);
        }
    }
}

// =============================================================================
// IR → LoroDoc
// =============================================================================

fn kind_to_str(k: u8) -> &'static str {
    match k {
        KIND_ATOMIC => "atomic",
        KIND_GENERIC => "generic",
        _ => "compound",
    }
}

fn build_node(tree: &LoroTree, parent: TreeParentId, ir: &IrNode) -> Result<(), ()> {
    let id = tree.create(parent).map_err(|_| ())?;
    let meta = tree.get_meta(id).map_err(|_| ())?;
    meta.insert("kind", kind_to_str(ir.kind).to_string())
        .map_err(|_| ())?;
    match ir.kind {
        KIND_ATOMIC => {
            // 文本（合法 UTF-8）→ LoroText 子容器（字符级 CRDT，可增量 insert/delete）；
            // 二进制（如图片，非法 UTF-8）→ Binary 值
            if let Ok(s) = std::str::from_utf8(&ir.text) {
                let txt = meta.insert_container("text", LoroText::new()).map_err(|_| ())?;
                if !s.is_empty() {
                    txt.insert(0, s).map_err(|_| ())?;
                }
            } else {
                meta.insert("text", ir.text.clone()).map_err(|_| ())?;
            }
        }
        _ => meta.insert("label", ir.label.clone()).map_err(|_| ())?,
    }
    for child in &ir.children {
        build_node(tree, TreeParentId::Node(id), child)?;
    }
    Ok(())
}

fn doc_from_ir(root: &IrNode) -> Result<LoroDoc, ()> {
    let doc = LoroDoc::new();
    let tree = doc.get_tree(TREE_NAME);
    // 启用分数索引，保证子节点顺序在并发与往返中稳定
    tree.enable_fractional_index(0);
    // 根 IR 节点作为 LoroTree 的唯一 root
    build_node(&tree, TreeParentId::Root, root)?;
    Ok(doc)
}

/// 把 IR 子树作为一个新的、带 `__section__` 标签的 root 装入 LoroTree。
///
/// 与 [`build_node`] 的区别：复用已创建的 root 节点，把 IR 根的 kind/label/text
/// 「提升」进 root 自身（而非在 root 下再挂一层 wrapper），其子节点再递归 build。
/// 这样 [`read_node`](root) 能直接还原该 section 树，多余的 `__section__` 元数据键
/// 被 read_node 忽略。供 `mogan_loro_doc_seed_section` 使用——把 body 之外的文档部分
/// （style/initial 等）作为独立 root 纳入同一 LoroDoc/LoroTree。
fn seed_section_root(tree: &LoroTree, name: &[u8], ir_root: &IrNode) -> Result<TreeID, ()> {
    let id = tree.create(TreeParentId::Root).map_err(|_| ())?;
    let meta = tree.get_meta(id).map_err(|_| ())?;
    meta.insert("__section__", name.to_vec()).map_err(|_| ())?;
    meta.insert("kind", kind_to_str(ir_root.kind).to_string())
        .map_err(|_| ())?;
    match ir_root.kind {
        KIND_ATOMIC => {
            // 文本（合法 UTF-8）→ LoroText 子容器；二进制 → Binary 值（同 build_node）
            if let Ok(s) = std::str::from_utf8(&ir_root.text) {
                let txt = meta.insert_container("text", LoroText::new()).map_err(|_| ())?;
                if !s.is_empty() {
                    txt.insert(0, s).map_err(|_| ())?;
                }
            } else {
                meta.insert("text", ir_root.text.clone()).map_err(|_| ())?;
            }
        }
        _ => {
            meta.insert("label", ir_root.label.clone()).map_err(|_| ())?;
        }
    }
    for child in &ir_root.children {
        build_node(tree, TreeParentId::Node(id), child)?;
    }
    Ok(id)
}

// =============================================================================
// LoroDoc → IR
// =============================================================================

/// 从节点的元数据 Map 中取字符串字段
fn get_meta_str(meta: &LoroMap, key: &str) -> Option<String> {
    match meta.get(key)? {
        ValueOrContainer::Value(v) => {
            let arc: Arc<String> = Arc::<String>::try_from(v).ok()?;
            Some(arc.as_str().to_string())
        }
        _ => None,
    }
}

/// 从节点的元数据 Map 中取二进制字段（label 以 Binary 存储，保留原始字节）
fn get_meta_bytes(meta: &LoroMap, key: &str) -> Option<Vec<u8>> {
    match meta.get(key)? {
        ValueOrContainer::Value(v) => {
            let arc: Arc<Vec<u8>> = Arc::<Vec<u8>>::try_from(v).ok()?;
            Some((*arc).clone())
        }
        _ => None,
    }
}

/// 取原子节点的 text：文本原子存为 LoroText 子容器，二进制原子存为 Binary 值
fn get_meta_text_or_binary(meta: &LoroMap) -> Option<Vec<u8>> {
    match meta.get("text")? {
        ValueOrContainer::Value(v) => {
            let arc: Arc<Vec<u8>> = Arc::<Vec<u8>>::try_from(v).ok()?;
            Some((*arc).clone())
        }
        ValueOrContainer::Container(Container::Text(t)) => {
            let mut s = String::new();
            t.iter(|chunk| {
                s.push_str(chunk);
                true
            });
            Some(s.into_bytes())
        }
        _ => None,
    }
}

fn str_to_kind(s: &str) -> u8 {
    match s {
        "atomic" => KIND_ATOMIC,
        "generic" => KIND_GENERIC,
        _ => KIND_COMPOUND,
    }
}

/// 把一个文本节点的 LoroText 内容按 SPLIT marker 切成 1..=N+1 个 IrNode 原子。
/// 无 marker 或锚点失效时退化为 1 个原子（与旧行为一致）。`child_ids` 恒为空
/// （文本容器不是 LoroTree 子节点，不计入树子节点数）。
fn read_text_segments(doc: &LoroDoc, meta: &LoroMap) -> Vec<IrNode> {
    let text = get_meta_text_or_binary(meta).unwrap_or_default();
    let boundaries = match get_split(meta) {
        Some(list) => split_boundaries(doc, &list),
        None => Vec::new(),
    };
    let n = text.len();
    let mut segs = Vec::new();
    let mut start = 0usize;
    for &b in &boundaries {
        if b > start && b <= n {
            segs.push(IrNode {
                kind: KIND_ATOMIC,
                label: Vec::new(),
                text: text[start..b].to_vec(),
                children: Vec::new(),
            });
            start = b;
        }
    }
    segs.push(IrNode {
        kind: KIND_ATOMIC,
        label: Vec::new(),
        text: text[start..].to_vec(),
        children: Vec::new(),
    });
    segs
}

/// 无 LoroDoc 句柄时的退化切段：只按整段文本返回 1 个原子（无法解析 marker
/// cursor 偏移；仅防御，正常路径都有 doc）。
fn read_text_segments_no_doc(meta: &LoroMap) -> Vec<IrNode> {
    let text = get_meta_text_or_binary(meta).unwrap_or_default();
    vec![IrNode {
        kind: KIND_ATOMIC,
        label: Vec::new(),
        text,
        children: Vec::new(),
    }]
}

/// 读一个 LoroTree 节点为 1..=N+1 个 IrNode（1↔N 物化）。
/// 文本原子有 SPLIT marker 时切成多段，各段作为兄弟扁平展开（调用方 extend）。
fn read_node_vec(tree: &LoroTree, id: TreeID) -> Result<Vec<IrNode>, ()> {
    let meta = tree.get_meta(id).map_err(|_| ())?;
    let kind = get_meta_str(&meta, "kind").unwrap_or_default();
    let kind_byte = str_to_kind(&kind);
    if kind_byte == KIND_ATOMIC {
        // 文本原子：按 SPLIT marker 切成 1..=N+1 段（扁平返回）。
        return Ok(match tree.doc() {
            Some(d) => read_text_segments(&d, &meta),
            None => read_text_segments_no_doc(&meta),
        });
    }
    // 复合节点：递归子节点（每个可能展开为多段），扁平收集。
    let child_ids = tree.children(TreeParentId::Node(id)).unwrap_or_default();
    let mut children = Vec::with_capacity(child_ids.len());
    for cid in child_ids {
        children.extend(read_node_vec(tree, cid)?);
    }
    let label = get_meta_bytes(&meta, "label").unwrap_or_default();
    Ok(vec![IrNode {
        kind: kind_byte,
        label,
        text: Vec::new(),
        children,
    }])
}

fn read_node(tree: &LoroTree, id: TreeID) -> Result<IrNode, ()> {
    // 根节点必定只返回 1 个 IrNode（根是复合，其子节点可能展开但归入根的 children）。
    let mut v = read_node_vec(tree, id)?;
    if v.len() == 1 {
        Ok(v.remove(0))
    } else {
        // 根不应该是文本原子（不会有多段）；防御：取第一个
        Ok(v.remove(0))
    }
}

fn doc_to_ir(doc: &LoroDoc) -> Result<IrNode, ()> {
    let tree = doc.get_tree(TREE_NAME);
    let roots = tree.roots();
    // 单 root：即 moebius 文档树的根
    let root = *roots.get(0).ok_or(())?;
    read_node(&tree, root)
}

// =============================================================================
// 输出缓冲区管理：Rust 分配，C 侧用 mogan_loro_free 释放
// =============================================================================

/// 把 Vec 泄漏成裸指针交给 C；容量已 shrink 到 == 长度，便于 from_raw_parts 回收
fn emit_out(data: Vec<u8>, out: *mut *mut u8, out_len: *mut usize) {
    let mut data = data;
    data.shrink_to_fit();
    let len = data.len();
    let p = data.as_mut_ptr();
    std::mem::forget(data);
    unsafe {
        if !out.is_null() {
            *out = p;
        }
        if !out_len.is_null() {
            *out_len = len;
        }
    }
}

/// 释放由本 crate 通过 emit_out 分配的缓冲区
///
/// # Safety
/// `ptr` 必须是本 crate 返回的、长度为 `len` 的缓冲区
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_free(ptr: *mut u8, len: usize) {
    if ptr.is_null() {
        return;
    }
    drop(Vec::from_raw_parts(ptr, len, len));
}

// =============================================================================
// 对外 C ABI
// =============================================================================

/// IR 扁平字节 → Loro snapshot 字节。
///
/// 成功返回 0，并把 snapshot 写入 `*out`（长度 `*out_len`，Rust 分配，需
/// [`mogan_loro_free`] 释放）；失败返回负数错误码。
///
/// # Safety
/// `ir` 须指向 `ir_len` 字节的有效缓冲区；`out`/`out_len` 可空（空则不写出）。
#[no_mangle]
pub extern "C" fn mogan_loro_encode(
    ir: *const u8,
    ir_len: usize,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if ir.is_null() {
        return -1;
    }
    let buf = unsafe { std::slice::from_raw_parts(ir, ir_len) };
    let mut r = Reader { buf, pos: 0 };
    let root = match r.node() {
        Ok(n) => n,
        Err(_) => return -2, // IR 解码失败
    };
    let doc = match doc_from_ir(&root) {
        Ok(d) => d,
        Err(_) => return -3, // LoroTree 构建失败
    };
    doc.commit();
    let snap = match doc.export(ExportMode::Snapshot) {
        Ok(b) => b,
        Err(_) => return -4, // snapshot 导出失败
    };
    emit_out(snap, out, out_len);
    0
}

/// Loro snapshot 字节 → IR 扁平字节。
///
/// 成功返回 0，并把 IR 编码写入 `*out`（长度 `*out_len`，Rust 分配，需
/// [`mogan_loro_free`] 释放）；失败返回负数错误码。
///
/// # Safety
/// `snap` 须指向 `snap_len` 字节的有效缓冲区；`out`/`out_len` 可空。
#[no_mangle]
pub extern "C" fn mogan_loro_decode(
    snap: *const u8,
    snap_len: usize,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if snap.is_null() {
        return -1;
    }
    let buf = unsafe { std::slice::from_raw_parts(snap, snap_len) };
    let doc = LoroDoc::new();
    if doc.import(buf).is_err() {
        return -2; // snapshot 导入失败
    }
    doc.commit();
    doc.checkout_to_latest();
    let root = match doc_to_ir(&doc) {
        Ok(n) => n,
        Err(_) => return -3, // LoroTree 遍历失败
    };
    let mut w = Writer { buf: Vec::new() };
    w.node(&root);
    emit_out(w.buf, out, out_len);
    0
}

// =============================================================================
// live-doc API（Phase 2：增量 op 镜像）
// =============================================================================

/// TreeID 的 C 表示（与 loro::TreeID {peer:u64, counter:i32} 布局一致，按值跨界）
#[repr(C)]
#[derive(Clone, Copy)]
pub struct MoganTreeId {
    pub peer: u64,
    pub counter: i32,
}

/// 表示 Root 父节点的哨兵 peer 值（C++ 传 Root 时用 {MOGAN_ROOT_PEER, 0}）
const MOGAN_ROOT_PEER: u64 = u64::MAX;
/// 失败返回值（真实 TreeID 的 peer 不会是 0）
const NULL_ID: MoganTreeId = MoganTreeId { peer: 0, counter: 0 };

fn parent_from(id: MoganTreeId) -> TreeParentId {
    if id.peer == MOGAN_ROOT_PEER {
        TreeParentId::Root
    } else {
        TreeParentId::Node(treeid_from(id))
    }
}

fn treeid_from(id: MoganTreeId) -> TreeID {
    TreeID {
        peer: id.peer,
        counter: id.counter,
    }
}

fn treeid_to(id: TreeID) -> MoganTreeId {
    MoganTreeId {
        peer: id.peer,
        counter: id.counter,
    }
}

/// 新建 live doc（已 enable 分数索引）。返回句柄，用 mogan_loro_doc_free 释放。
#[no_mangle]
pub extern "C" fn mogan_loro_doc_new() -> *mut LoroDoc {
    let doc = LoroDoc::new();
    doc.get_tree(TREE_NAME).enable_fractional_index(0);
    Box::into_raw(Box::new(doc))
}

/// # Safety: doc 须为 mogan_loro_doc_new 返回的句柄
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_free(doc: *mut LoroDoc) {
    if !doc.is_null() {
        // 先摘除 local-update 订阅（保活），再释放 doc
        if let Ok(mut subs) = local_subs().lock() {
            subs.remove(&(doc as usize));
        }
        if let Ok(mut vvs) = export_vvs().lock() {
            vvs.remove(&(doc as usize));
        }
        drop(Box::from_raw(doc));
    }
}

/// 显式提交 pending 本地 op。auto_commit 是延迟的（debounced），不会同步触发
/// local-update 事件；事件级增量同步需在镜像后显式 commit 以同步触发回调。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_commit(doc: *mut LoroDoc) {
    if !doc.is_null() {
        (*doc).commit();
    }
}

/// 订阅本地编辑产生的增量 update：每次本地 op 提交时回调 cb(user_data, bytes, len)，
/// bytes 是这次编辑的 delta（仅新增 op，非整 snapshot）。事件级增量同步的核心。
/// 返回 0 成功。订阅随 doc 释放自动取消。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_on_local_update(
    doc: *mut LoroDoc,
    cb: extern "C" fn(*mut std::ffi::c_void, *const u8, usize),
    user_data: *mut std::ffi::c_void,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    // 把裸指针转 usize 捕获（usize 天然 Send+Sync，避开 edition2021 离散捕获 *mut 的 !Send 问题）
    let ud = user_data as usize;
    let sub = (*doc).subscribe_local_update(Box::new(move |bytes: &Vec<u8>| {
        cb(ud as *mut std::ffi::c_void, bytes.as_ptr(), bytes.len());
        true // 保持订阅
    }));
    local_subs().lock().unwrap().insert(doc as usize, sub);
    0
}

/// 用 IR 扁平字节 seed（构建整棵树）。成功返回 0。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_seed(
    doc: *mut LoroDoc,
    ir: *const u8,
    ir_len: usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let buf = if ir.is_null() || ir_len == 0 {
        &[][..]
    } else {
        std::slice::from_raw_parts(ir, ir_len)
    };
    let mut r = Reader { buf, pos: 0 };
    let root = match r.node() {
        Ok(n) => n,
        Err(_) => return -2,
    };
    let tree = (*doc).get_tree(TREE_NAME);
    match build_node(&tree, TreeParentId::Root, &root) {
        Ok(()) => 0,
        Err(()) => -3,
    }
}

/// 导出 snapshot 字节到 *out/*out_len（Rust 分配，需 mogan_loro_free 释放）。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_export(
    doc: *mut LoroDoc,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    (*doc).commit();
    match (*doc).export(ExportMode::Snapshot) {
        Ok(b) => {
            emit_out(b, out, out_len);
            0
        }
        Err(_) => -2,
    }
}

/// live doc 的树 → IR 扁平字节（与 Phase 1 encode 输出兼容）。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_to_ir(
    doc: *mut LoroDoc,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    match doc_to_ir(&*doc) {
        Ok(root) => {
            let mut w = Writer { buf: Vec::new() };
            w.node(&root);
            emit_out(w.buf, out, out_len);
            0
        }
        Err(()) => -2,
    }
}

/// 把 snapshot/update 字节 import（合并）进 live doc。成功返回 0。
/// 用于 Phase 3 反向同步：远端 Loro 数据 → 本地 shadow doc。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_import(
    doc: *mut LoroDoc,
    bytes: *const u8,
    len: usize,
) -> i32 {
    if doc.is_null() || bytes.is_null() {
        return -1;
    }
    let buf = std::slice::from_raw_parts(bytes, len);
    match (*doc).import(buf) {
        Ok(_) => {
            (*doc).commit();
            0
        }
        Err(_) => -2,
    }
}

/// 导出"自上次本函数调用以来"的本地增量 update 到 *out/*out_len，并把内部 vv
/// 水位推进到当前（下次只导出新增部分）。seed 后主动广播初始状态用：
/// subscribe_local_update 的首个 commit 只含 create ops（无文本），接收端须靠
/// 这次导出拿到完整初始内容。返回 0 成功。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_export_local_update(
    doc: *mut LoroDoc,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    (*doc).commit();
    let mut vvs = export_vvs().lock().unwrap();
    let vv = vvs.entry(doc as usize).or_default();
    match (*doc).export(ExportMode::updates(&*vv)) {
        Ok(b) => {
            *vv = (*doc).oplog_vv();
            emit_out(b, out, out_len);
            0
        }
        Err(_) => -2,
    }
}

/// 把内部 export vv 水位推进到当前 oplog_vv，但不导出任何字节。用于 import 远端
/// 数据（JOIN 同步）后标记"当前所有 op 已知、不要再上行"——否则 export_local_update
/// 会把刚收到的 snapshot/update 当本地增量全量回传服务端（连接即有一次空上行）。
/// 注意：这会把当前所有 op（含本地未上行的）都标记为已知，故只能在"无 pending
/// 本地 op"时调用（即首次 import 同步；重连场景有 pending 本地 op，不可调用）。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_advance_export_vv(doc: *mut LoroDoc) {
    if doc.is_null() {
        return;
    }
    (*doc).commit();
    let mut vvs = export_vvs().lock().unwrap();
    let vv = vvs.entry(doc as usize).or_default();
    *vv = (*doc).oplog_vv();
}

/// 把一个 TreeID 编码为 16 字节数组（前 8 peer + 4 counter + 4 padding）。
fn treeid_to_16(id: TreeID) -> [u8; 16] {
    let mut b = [0u8; 16];
    b[0..8].copy_from_slice(&id.peer.to_le_bytes());
    b[8..12].copy_from_slice(&id.counter.to_le_bytes());
    b
}

/// 把 16 字节身份写进 buffer（peer 8 + counter 4 = 12 字节有效）。
fn write_mogan_id(w: &mut Writer, id16: &[u8]) {
    w.buf.extend_from_slice(&id16[0..8]);
    w.buf.extend_from_slice(&id16[8..12]);
}

/// 递归写出"带 TreeID 的增强 IR"，1↔N 扁平展开。
/// 文本原子有 SPLIT marker 时展开为 N+1 个原子 IR 节点（扁平兄弟，无 wrapper）。
/// 返回写的 IR 节数（1 或 N+1），供父节点计正确的 n_children。
fn write_node_with_id(tree: &LoroTree, id: TreeID, w: &mut Writer) -> usize {
    let id_bytes = treeid_to_16(id);
    let meta = match tree.get_meta(id) {
        Ok(m) => m,
        Err(_) => {
            write_mogan_id(w, &id_bytes);
            w.u8(KIND_COMPOUND);
            w.bytes_field(&Vec::new());
            w.bytes_field(&Vec::new());
            w.u32(0);
            return 1;
        }
    };
    let kind_byte = str_to_kind(&get_meta_str(&meta, "kind").unwrap_or_default());

    if kind_byte == KIND_ATOMIC {
        let segs = match tree.doc() {
            Some(d) => read_text_segments(&d, &meta),
            None => read_text_segments_no_doc(&meta),
        };
        if segs.len() > 1 {
            // 有 marker：展开为 N+1 个原子 IR 节点（扁平，无 wrapper）。
            let elem_ids: Vec<Option<Vec<u8>>> = match get_split(&meta) {
                Some(list) => (0..list.len()).map(|i| elem_identity(&list, i)).collect(),
                None => Vec::new(),
            };
            for (si, s) in segs.iter().enumerate() {
                let seg_id16= if si == 0 {
                    id_bytes                         // 前缀段：真 TreeID
                } else {
                    // marker[si-1] 元素身份截断 16 字节；失败回退真 TreeID
                    elem_ids.get(si - 1).and_then(|o| o.as_ref()).map_or(id_bytes, |v| {
                        let mut b= [0u8; 16];
                        if v.len() >= 16 { b.copy_from_slice(&v[0..16]); } else { b.copy_from_slice(&id_bytes); }
                        b
                    })
                };
                write_mogan_id(w, &seg_id16);
                w.u8(KIND_ATOMIC);
                w.bytes_field(&s.label);
                w.bytes_field(&s.text);
                w.u32(0);
            }
            return segs.len();
        }
        // 无 marker：1 个原子。
        write_mogan_id(w, &id_bytes);
        w.u8(KIND_ATOMIC);
        w.bytes_field(&Vec::new());
        w.bytes_field(&segs.into_iter().next().unwrap().text);
        w.u32(0);
        return 1;
    }

    // 复合节点：先把子节点写进 temp（展开计数），再写 header + children。
    let label = get_meta_bytes(&meta, "label").unwrap_or_default();
    let child_ids = tree.children(TreeParentId::Node(id)).unwrap_or_default();
    let mut temp = Writer { buf: Vec::new() };
    let mut n = 0usize;
    for cid in &child_ids {
        n += write_node_with_id(tree, *cid, &mut temp);
    }
    write_mogan_id(w, &id_bytes);
    w.u8(kind_byte);
    w.bytes_field(&label);
    w.bytes_field(&Vec::new());
    w.u32(n as u32);
    w.buf.extend(temp.buf);
    1
}

/// live doc -> 带 TreeID 的增强 IR 字节。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_to_ir_with_ids(
    doc: *mut LoroDoc,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let root = match tree.roots().get(0) {
        Some(r) => *r,
        None => return -2,
    };
    let mut w = Writer { buf: Vec::new() };
    write_node_with_id(&tree, root, &mut w);
    emit_out(w.buf, out, out_len);
    0
}

/// 在 LoroTree 下创建一个新的、带 `__section__` = name 标签的 root，并把 IR 子树
/// 提升进该 root（见 [`seed_section_root`]）。用于把 body 之外的文档部分（style/
/// initial 等）作为独立 root 纳入同一 LoroDoc。返回新 root 的 TreeID；失败 NULL_ID。
///
/// # Safety
/// `doc` 须为 `mogan_loro_doc_new` 返回的句柄；`name`/`ir` 须指向有效缓冲区。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_seed_section(
    doc: *mut LoroDoc,
    name: *const u8,
    name_len: usize,
    ir: *const u8,
    ir_len: usize,
) -> MoganTreeId {
    if doc.is_null() || name.is_null() {
        return NULL_ID;
    }
    let name_buf = std::slice::from_raw_parts(name, name_len);
    let ir_buf = if ir.is_null() || ir_len == 0 {
        &[][..]
    } else {
        std::slice::from_raw_parts(ir, ir_len)
    };
    let mut r = Reader { buf: ir_buf, pos: 0 };
    let ir_root = match r.node() {
        Ok(n) => n,
        Err(_) => return NULL_ID,
    };
    let tree = (*doc).get_tree(TREE_NAME);
    match seed_section_root(&tree, name_buf, &ir_root) {
        Ok(id) => treeid_to(id),
        Err(()) => NULL_ID,
    }
}

/// 从指定 root TreeID 读出 section 子树为 IR 扁平字节（复用 [`read_node`]，不读
/// roots[0]）。供 shadow 把 meta section 还原为 tree。成功返回 0。
///
/// # Safety
/// `doc` 须为有效句柄；`out`/`out_len` 可空。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_section_to_ir(
    doc: *mut LoroDoc,
    root_id: MoganTreeId,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let root = match read_node(&tree, treeid_from(root_id)) {
        Ok(n) => n,
        Err(_) => return -2,
    };
    let mut w = Writer { buf: Vec::new() };
    w.node(&root);
    emit_out(w.buf, out, out_len);
    0
}

/// 枚举 LoroTree 所有带 `__section__` 标签的 root（body 等无标签的 root 被跳过），
/// 输出到 *out：每条为 `name_len:u32 name:bytes root_peer:u64 root_counter:i32`
/// （均小端）。供 shadow 在 import 远端数据后重建 section -> root_id 账本。成功 0。
///
/// # Safety
/// `doc` 须为有效句柄；`out`/`out_len` 可空。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_doc_list_sections(
    doc: *mut LoroDoc,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let mut buf = Vec::new();
    for id in tree.roots() {
        let name_bytes = match tree
            .get_meta(id)
            .ok()
            .and_then(|m| get_meta_bytes(&m, "__section__"))
        {
            Some(n) => n,
            None => continue, // 无 __section__（body 等）→ 跳过
        };
        buf.extend_from_slice(&(name_bytes.len() as u32).to_le_bytes());
        buf.extend_from_slice(&name_bytes);
        buf.extend_from_slice(&id.peer.to_le_bytes());
        buf.extend_from_slice(&id.counter.to_le_bytes());
    }
    emit_out(buf, out, out_len);
    0
}

/// 在 parent 的 index 处创建子节点。compound/generic 传 label；atomic 不用。
/// 返回新节点 TreeID；失败返回 NULL_ID。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_create(
    doc: *mut LoroDoc,
    parent: MoganTreeId,
    index: u32,
    kind: u8,
    label: *const u8,
    label_len: usize,
) -> MoganTreeId {
    if doc.is_null() {
        return NULL_ID;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let id = match tree.create_at(parent_from(parent), index as usize) {
        Ok(id) => id,
        Err(_) => return NULL_ID,
    };
    if let Ok(meta) = tree.get_meta(id) {
        let _ = meta.insert("kind", kind_to_str(kind).to_string());
        if kind != KIND_ATOMIC && !label.is_null() && label_len > 0 {
            let lbl = std::slice::from_raw_parts(label, label_len);
            let _ = meta.insert("label", lbl.to_vec());
        }
    }
    treeid_to(id)
}

/// 删除节点（子节点不真正删除，从状态中消失）。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_delete(doc: *mut LoroDoc, id: MoganTreeId) -> i32 {
    if doc.is_null() {
        return -1;
    }
    (*doc)
        .get_tree(TREE_NAME)
        .delete(treeid_from(id))
        .map(|_| 0)
        .unwrap_or(-2)
}

/// 移动 target 到 parent 的 index 处（包/拆/重排）。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_mov(
    doc: *mut LoroDoc,
    target: MoganTreeId,
    parent: MoganTreeId,
    index: u32,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    (*doc)
        .get_tree(TREE_NAME)
        .mov_to(treeid_from(target), parent_from(parent), index as usize)
        .map(|_| 0)
        .unwrap_or(-2)
}

/// 原子文本 JOIN：把 Y 的文本追加到 X 的 LoroText 末尾（unicode 长度处）+ 删除 Y。
/// X/Y 都须是文本容器（meta.text = LoroText）。用于精确翻译 MOD_JOIN（退格合并）。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_join_text(
    doc: *mut LoroDoc,
    x_id: MoganTreeId,
    y_id: MoganTreeId,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let x_meta = match tree.get_meta(treeid_from(x_id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let y_meta = match tree.get_meta(treeid_from(y_id)) {
        Ok(m) => m,
        Err(_) => return -3,
    };
    let x_text = match x_meta.get("text") {
        Some(ValueOrContainer::Container(Container::Text(t))) => t,
        _ => return -4, // X 非文本容器（调用方改走复合 join）
    };
    // X 的 unicode 长度（追加位置）
    let mut x_len = 0usize;
    x_text.iter(|c| {
        x_len += c.chars().count();
        true
    });
    // Y 的文本内容
    let mut y_content = String::new();
    match y_meta.get("text") {
        Some(ValueOrContainer::Container(Container::Text(t))) => {
            t.iter(|c| {
                y_content.push_str(c);
                true
            });
        }
        _ => return -5, // Y 非文本容器
    }
    if !y_content.is_empty() && x_text.insert(x_len, &y_content).is_err() {
        return -6;
    }
    if tree.delete(treeid_from(y_id)).is_err() {
        return -7;
    }
    0
}

/// 取 parent 的子节点 TreeID 列表，序列化为 N*12 字节（每条 peer:u64 LE + counter:i32 LE）。
/// 写入 *out（*out_len = 字节数），Rust 分配，需 mogan_loro_free 释放。REMOVE 按位置删用。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_children(
    doc: *mut LoroDoc,
    parent: MoganTreeId,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let children = match tree.children(parent_from(parent)) {
        Some(c) => c,
        None => return -2,
    };
    let mut buf = Vec::with_capacity(children.len() * 12);
    for c in children {
        buf.extend_from_slice(&c.peer.to_le_bytes());
        buf.extend_from_slice(&c.counter.to_le_bytes());
    }
    emit_out(buf, out, out_len);
    0
}

/// 改 compound/generic 节点的 label。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_set_label(
    doc: *mut LoroDoc,
    id: MoganTreeId,
    label: *const u8,
    label_len: usize,
) -> i32 {
    if doc.is_null() || label.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match tree.get_meta(treeid_from(id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let lbl = std::slice::from_raw_parts(label, label_len);
    meta.insert("label", lbl.to_vec()).map(|_| 0).unwrap_or(-3)
}

/// 设置二进制原子的 text（图片等）。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_set_binary(
    doc: *mut LoroDoc,
    id: MoganTreeId,
    bytes: *const u8,
    len: usize,
) -> i32 {
    if doc.is_null() || bytes.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match tree.get_meta(treeid_from(id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let b = std::slice::from_raw_parts(bytes, len);
    meta.insert("text", b.to_vec()).map(|_| 0).unwrap_or(-3)
}

/// 取节点的 LoroText（没有则新建），用于文本原子的字符级编辑。
fn get_or_create_text(meta: &LoroMap) -> Option<LoroText> {
    if let Some(ValueOrContainer::Container(Container::Text(t))) = meta.get("text") {
        return Some(t);
    }
    meta.insert_container("text", LoroText::new()).ok()
}

/// 文本原子：在 pos 处插入 bytes（须为合法 UTF-8）。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_text_insert(
    doc: *mut LoroDoc,
    id: MoganTreeId,
    pos: u32,
    bytes: *const u8,
    len: usize,
) -> i32 {
    if doc.is_null() || bytes.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match tree.get_meta(treeid_from(id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let txt = match get_or_create_text(&meta) {
        Some(t) => t,
        None => return -3,
    };
    let s = match std::str::from_utf8(std::slice::from_raw_parts(bytes, len)) {
        Ok(s) => s,
        Err(_) => return -4,
    };
    if !s.is_empty() {
        txt.insert(pos as usize, s).map(|_| 0).unwrap_or(-5)
    } else {
        0
    }
}

/// 文本原子：在 pos 处删除 len 个字符。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_text_delete(
    doc: *mut LoroDoc,
    id: MoganTreeId,
    pos: u32,
    len: u32,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match tree.get_meta(treeid_from(id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let txt = match meta.get("text") {
        Some(ValueOrContainer::Container(Container::Text(t))) => t,
        _ => return -3,
    };
    txt.delete(pos as usize, len as usize)
        .map(|_| 0)
        .unwrap_or(-4)
}

// =============================================================================
// 稳定光标位置（Cursor，op-id 锚定）——多光标 CRDT 级同步
// =============================================================================

/// 把原子文本节点（tree_id 的 meta.text LoroText）在 unicode `offset` 处的稳定
/// 位置编码为 postcard 字节，写入 `*out`/`*out_len`（Rust 分配，需
/// [`mogan_loro_free`] 释放）。稳定位置由 op-id 锚定，并发编辑下自动跟随内容
/// 位移，是 CRDT 级光标同步的基础。
///
/// 返回 0 成功；-1 doc 空；-2 节点不存在；-3 非文本容器；-4 offset 越界。
///
/// # Safety
/// `doc` 须为合法句柄；`out`/`out_len` 可空。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_encode_cursor(
    doc: *mut LoroDoc,
    tree_id: MoganTreeId,
    offset: u32,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match tree.get_meta(treeid_from(tree_id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let txt = match meta.get("text") {
        Some(ValueOrContainer::Container(Container::Text(t))) => t,
        _ => return -3, // 非文本容器（复合节点走结构编码，不调本函数）
    };
    // 取稳定位置。末尾（offset==len）以 Middle 可能返回 None，退到前一字符右侧锚定
    // （= 同一逻辑位置）。offset==0 且空容器时 get_cursor 返回 end-anchor（id=None）。
    let cursor = txt
        .get_cursor(offset as usize, Side::Middle)
        .or_else(|| {
            if offset > 0 {
                txt.get_cursor(offset as usize - 1, Side::Right)
            } else {
                None
            }
        });
    match cursor {
        Some(c) => {
            emit_out(c.encode(), out, out_len);
            0
        }
        None => -4,
    }
}

/// 反序列化 Cursor 字节并按**当前 doc** 解析为 unicode 偏移，写入 `*out_offset`。
/// 锚点字符被并发删除时 Loro 自愈到邻近活位置（clamp）；仅当整个容器消失 / 历史
/// 已 GC / id 找不到时返回负数（调用方据此丢弃该远程光标）。
///
/// 返回 0 成功；-1 doc/bytes 空；-2 字节反序列化失败；-3 位置不可解析。
///
/// # Safety
/// `doc` 须为合法句柄；`bytes` 须指向 `len` 字节有效缓冲区；`out_offset` 可空。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_decode_cursor(
    doc: *mut LoroDoc,
    bytes: *const u8,
    len: usize,
    out_offset: *mut u32,
) -> i32 {
    if doc.is_null() || bytes.is_null() {
        return -1;
    }
    let buf = std::slice::from_raw_parts(bytes, len);
    let cursor = match Cursor::decode(buf) {
        Ok(c) => c,
        Err(_) => return -2,
    };
    // plain LoroText（无 style/attachment）：event index == unicode index
    match (*doc).get_cursor_pos(&cursor) {
        Ok(r) => {
            if !out_offset.is_null() {
                *out_offset = r.current.pos as u32;
            }
            0
        }
        Err(_) => -3, // ContainerDeleted / HistoryCleared / IdNotFound → 丢弃
    }
}

// =============================================================================
// SPLIT 结构边界 marker（保字符身份）
// =============================================================================
// marker 存在文本节点 meta 的 LoroMovableList（SPLIT_KEY）里，value 为边界的稳定
// 位置（文本 Cursor postcard 字节）。元素自身有 CRDT 身份（元素 Cursor 编码）。
// SPLIT 只插 marker、JOIN 只删 marker，绝不 delete+insert 文本，故字符身份不变。

/// 取文本节点的 meta Map（须存在）。
fn node_meta(tree: &LoroTree, id: TreeID) -> Result<LoroMap, ()> {
    tree.get_meta(id).map_err(|_| ())
}

/// 取文本节点 meta 下的 LoroText（没有则 None）。
fn get_text(meta: &LoroMap) -> Option<LoroText> {
    match meta.get("text") {
        Some(ValueOrContainer::Container(Container::Text(t))) => Some(t),
        _ => None,
    }
}

/// 取或创建文本节点 meta 下的 SPLIT MovableList。
fn get_or_create_split(meta: &LoroMap) -> Option<LoroMovableList> {
    match meta.get(SPLIT_KEY) {
        Some(ValueOrContainer::Container(Container::MovableList(l))) => Some(l),
        Some(_) => None,
        None => meta
            .insert_container(SPLIT_KEY, LoroMovableList::new())
            .ok(),
    }
}

/// 只读地取 SPLIT MovableList（没有则 None）。
fn get_split(meta: &LoroMap) -> Option<LoroMovableList> {
    match meta.get(SPLIT_KEY) {
        Some(ValueOrContainer::Container(Container::MovableList(l))) => Some(l),
        _ => None,
    }
}

/// 读元素 value 里的边界 Cursor 字节（元素 value 是 Binary(postcard)）。
fn marker_cursor_bytes(list: &LoroMovableList, i: usize) -> Option<Vec<u8>> {
    match list.get(i) {
        Some(ValueOrContainer::Value(v)) => {
            let arc: std::sync::Arc<Vec<u8>> = std::sync::Arc::<Vec<u8>>::try_from(v).ok()?;
            Some((*arc).clone())
        }
        _ => None,
    }
}

/// 把各 marker 的边界 Cursor 解析成 unicode 偏移并升序排序（去重）。
/// 解析失败（锚点容器被删等）的 marker 丢弃——文本节点已删则整个不导出。
fn split_boundaries(doc: &LoroDoc, list: &LoroMovableList) -> Vec<usize> {
    let mut offs: Vec<usize> = Vec::new();
    for i in 0..list.len() {
        if let Some(bytes) = marker_cursor_bytes(list, i) {
            if let Ok(c) = Cursor::decode(&bytes) {
                if let Ok(r) = doc.get_cursor_pos(&c) {
                    offs.push(r.current.pos);
                }
            }
        }
    }
    offs.sort_unstable();
    offs.dedup();
    offs
}

/// 元素身份 = 该元素在 MovableList 中的稳定位置 Cursor（op-id 锚定）编码。
/// marker 只增不删可移动，故元素身份稳定且随 update 跨端可解析。
fn elem_identity(list: &LoroMovableList, i: usize) -> Option<Vec<u8>> {
    list.get_cursor(i, Side::Middle)
        .or_else(|| {
            if i > 0 {
                list.get_cursor(i - 1, Side::Right)
            } else {
                None
            }
        })
        .map(|c| c.encode())
}

/// 在文本节点 offset 处创建 SPLIT marker，返回元素身份（postcard 字节）。
/// 字符不动：只在文本 LoroText 上取边界 Cursor，向 MovableList 插入一条 marker。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_split_marker_create(
    doc: *mut LoroDoc,
    id: MoganTreeId,
    offset: u32,
    out: *mut *mut u8,
    out_len: *mut usize,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match node_meta(&tree, treeid_from(id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let txt = match get_text(&meta) {
        Some(t) => t,
        None => return -3,
    };
    // 边界稳定位置（op-id 锚定，非整数 offset 持久化）。
    let boundary = txt
        .get_cursor(offset as usize, Side::Middle)
        .or_else(|| {
            if offset > 0 {
                txt.get_cursor(offset as usize - 1, Side::Right)
            } else {
                None
            }
        });
    let boundary = match boundary {
        Some(c) => c,
        None => return -4,
    };
    let list = match get_or_create_split(&meta) {
        Some(l) => l,
        None => return -5,
    };
    // 按当前边界把 marker 插到正确次序（保持段内按位置升序）。
    let pos = doc_boundary_pos(&*doc, &boundary);
    let mut insert_at = list.len();
    for i in 0..list.len() {
        if let Some(bytes) = marker_cursor_bytes(&list, i) {
            if let Ok(c) = Cursor::decode(&bytes) {
                if let Ok(r) = (*doc).get_cursor_pos(&c) {
                    if r.current.pos > pos {
                        insert_at = i;
                        break;
                    }
                }
            }
        }
    }
    let bytes = boundary.encode();
    if list.insert(insert_at, bytes).is_err() {
        return -6;
    }
    // 元素身份：插入后该位置元素的 Cursor 编码。
    match elem_identity(&list, insert_at) {
        Some(id_bytes) => {
            emit_out(id_bytes, out, out_len);
            0
        }
        None => -7,
    }
}

/// 边界 Cursor 在文本里的当前 unicode 偏移（解析失败返回 usize::MAX）。
fn doc_boundary_pos(doc: &LoroDoc, c: &Cursor) -> usize {
    match doc.get_cursor_pos(c) {
        Ok(r) => r.current.pos,
        Err(_) => usize::MAX,
    }
}

/// 按元素身份删除 SPLIT marker（JOIN）。marker_id 是 create 返回的元素 Cursor 字节。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_split_marker_delete(
    doc: *mut LoroDoc,
    id: MoganTreeId,
    marker_id: *const u8,
    marker_id_len: usize,
) -> i32 {
    if doc.is_null() || marker_id.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match node_meta(&tree, treeid_from(id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let list = match get_split(&meta) {
        Some(l) => l,
        None => return -3,
    };
    let buf = std::slice::from_raw_parts(marker_id, marker_id_len);
    let target = match Cursor::decode(buf) {
        Ok(c) => c,
        Err(_) => return -4,
    };
    // 找到元素身份与 target 相同的元素下标（比较各自元素 Cursor 的 op-id）。
    for i in 0..list.len() {
        if let Some(id_bytes) = elem_identity(&list, i) {
            if let Ok(c) = Cursor::decode(&id_bytes) {
                if c == target {
                    if list.delete(i, 1).is_err() {
                        return -6;
                    }
                    return 0;
                }
            }
        }
    }
    -5 // 找不到该 marker
}

/// 按下标删除 SPLIT marker（JOIN 用）。index 是 MovableList 中的元素位置。
/// 返回 0 成功；<0 错误。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_split_marker_delete_at(
    doc: *mut LoroDoc,
    id: MoganTreeId,
    index: u32,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match node_meta(&tree, treeid_from(id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    let list = match get_split(&meta) {
        Some(l) => l,
        None => return -3,
    };
    if index as usize >= list.len() {
        return -4;
    }
    if list.delete(index as usize, 1).is_err() {
        return -5;
    }
    0
}

/// 文本节点当前是否含有效 SPLIT marker（用于测试/诊断）。返回 1=有，0=无，<0=错误。
#[no_mangle]
pub unsafe extern "C" fn mogan_loro_node_has_split_markers(
    doc: *mut LoroDoc,
    id: MoganTreeId,
) -> i32 {
    if doc.is_null() {
        return -1;
    }
    let tree = (*doc).get_tree(TREE_NAME);
    let meta = match node_meta(&tree, treeid_from(id)) {
        Ok(m) => m,
        Err(_) => return -2,
    };
    match get_split(&meta) {
        Some(list) => {
            let b = split_boundaries(&*doc, &list);
            if b.is_empty() {
                0
            } else {
                1
            }
        }
        None => 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn atomic(text: &str) -> IrNode {
        IrNode {
            kind: KIND_ATOMIC,
            label: vec![],
            text: text.as_bytes().to_vec(),
            children: vec![],
        }
    }

    fn compound(label: &str, children: Vec<IrNode>) -> IrNode {
        IrNode {
            kind: KIND_COMPOUND,
            label: label.as_bytes().to_vec(),
            text: vec![],
            children,
        }
    }

    /// 隔离诊断：分步定位 round-trip 失败在「读路径」还是「export/import」。
    /// 运行：cd 3rdparty/mogan-loro-ffi && cargo test -- --nocapture
    #[test]
    fn roundtrip_atomic_isolate() {
        let orig = atomic("hello 世界");
        let doc = doc_from_ir(&orig).expect("doc_from_ir");

        // (1) 直接读 encoding doc（不经 export/import）——验证 get_meta 读路径
        let direct = doc_to_ir(&doc).expect("doc_to_ir direct");
        eprintln!(
            "[direct]  kind={} label={:?} text={:?}",
            direct.kind, direct.label, direct.text
        );
        assert_eq!(direct.kind, KIND_ATOMIC, "direct read: kind mismatch");
        assert_eq!(
            direct.text,
            "hello 世界".as_bytes(),
            "direct read: text mismatch (get_meta 路径有问题)"
        );

        // (2) 经 export / import ——验证快照是否保留 tree + 元数据
        let snap = doc.export(ExportMode::Snapshot).expect("export");
        eprintln!("[snap]    len={}", snap.len());
        let doc2 = LoroDoc::new();
        doc2.import(&snap).expect("import");
        doc2.commit();
        doc2.checkout_to_latest();
        let tree2 = doc2.get_tree(TREE_NAME);
        eprintln!("[import]  roots={}", tree2.roots().len());
        let back = doc_to_ir(&doc2).expect("doc_to_ir after import");
        eprintln!(
            "[back]    kind={} label={:?} text={:?}",
            back.kind, back.label, back.text
        );
        assert_eq!(back.kind, KIND_ATOMIC, "after import: kind mismatch");
        assert_eq!(
            back.text,
            "hello 世界".as_bytes(),
            "after import: text mismatch (export/import 丢数据)"
        );
    }

    /// 二进制（非法 UTF-8，如 PNG 头 89 50 4E 47）必须逐字节保留——这是真实图片
    /// 文档往返的关键。用合法 UTF-8 的测试压不到这条路径。
    #[test]
    fn roundtrip_binary_atomic() {
        let binary= vec![0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x80, 0xFE, 0xFF, 0xC0];
        let orig = IrNode {
            kind: KIND_ATOMIC,
            label: vec![],
            text: binary.clone(),
            children: vec![],
        };
        let doc = doc_from_ir(&orig).expect("doc_from_ir");
        let snap = doc.export(ExportMode::Snapshot).expect("export");
        let doc2 = LoroDoc::new();
        doc2.import(&snap).expect("import");
        doc2.commit();
        doc2.checkout_to_latest();
        let back = doc_to_ir(&doc2).expect("doc_to_ir");
        eprintln!("[binary] orig={:?} back={:?}", orig.text, back.text);
        assert_eq!(back.text, binary, "二进制字节未保留（EFBFBD 即 U+FFFD 替换）");
    }

    #[test]
    fn roundtrip_compound_isolate() {
        let orig = compound(
            "document",
            vec![compound("para", vec![atomic("hi")]), atomic("x")],
        );
        let doc = doc_from_ir(&orig).expect("doc_from_ir");

        let direct = doc_to_ir(&doc).expect("direct");
        eprintln!(
            "[direct]  label={:?} n_children={}",
            direct.label,
            direct.children.len()
        );
        assert_eq!(direct.label, "document".as_bytes(), "direct: label");
        assert_eq!(direct.children.len(), 2, "direct: n_children");

        let snap = doc.export(ExportMode::Snapshot).expect("export");
        let doc2 = LoroDoc::new();
        doc2.import(&snap).expect("import");
        doc2.commit();
        doc2.checkout_to_latest();
        let back = doc_to_ir(&doc2).expect("back");
        eprintln!(
            "[back]    label={:?} n_children={}",
            back.label,
            back.children.len()
        );
        assert_eq!(back.label, "document".as_bytes(), "after import: label");
        assert_eq!(back.children.len(), 2, "after import: n_children");
        assert_eq!(back.children[0].label, "para".as_bytes(), "after import: child0 label");
    }

    /// 全 FFI 字节往返：IrNode -> Writer -> mogan_loro_encode -> mogan_loro_decode
    /// -> Reader -> IrNode。打印 Writer 字节，用于与 C++ 侧 loro_ir_encode 输出比对。
    #[test]
    fn full_ffi_byte_roundtrip() {
        let orig = compound("document", vec![atomic("hi"), atomic("x")]);
        let mut w = Writer { buf: vec![] };
        w.node(&orig);
        eprintln!("[rust-writer] {} bytes: {:?}", w.buf.len(), w.buf);

        let ir_bytes= w.buf.clone();
        let mut snap_ptr: *mut u8 = std::ptr::null_mut();
        let mut snap_len: usize = 0;
        let rc = mogan_loro_encode(
            ir_bytes.as_ptr(),
            ir_bytes.len(),
            &mut snap_ptr,
            &mut snap_len,
        );
        assert_eq!(rc, 0, "mogan_loro_encode rc={}", rc);
        let snap: Vec<u8> =
            unsafe { std::slice::from_raw_parts(snap_ptr, snap_len) }.to_vec();
        unsafe { mogan_loro_free(snap_ptr, snap_len) };

        let mut out_ptr: *mut u8 = std::ptr::null_mut();
        let mut out_len: usize = 0;
        let rc =
            mogan_loro_decode(snap.as_ptr(), snap.len(), &mut out_ptr, &mut out_len);
        assert_eq!(rc, 0, "mogan_loro_decode rc={}", rc);
        let ir_bytes2: Vec<u8> =
            unsafe { std::slice::from_raw_parts(out_ptr, out_len) }.to_vec();
        eprintln!("[rust-decode] {} bytes: {:?}", ir_bytes2.len(), ir_bytes2);
        unsafe { mogan_loro_free(out_ptr, out_len) };

        let mut r = Reader { buf: &ir_bytes2, pos: 0 };
        let back = r.node().expect("Reader::node");
        eprintln!(
            "[rust-back] kind={} label={:?} n_children={}",
            back.kind,
            back.label,
            back.children.len()
        );
        assert_eq!(back.label, "document".as_bytes());
        assert_eq!(back.children.len(), 2);
        assert_eq!(back.children[0].text, "hi".as_bytes());
        assert_eq!(back.children[1].text, "x".as_bytes());
        // Writer 输出（encode 输入）应与 decode 输出逐字节一致
        assert_eq!(ir_bytes, ir_bytes2, "ir bytes changed across FFI roundtrip");
    }

    /// live API：doc_new → seed(IR) → to_ir 往返；且 export snapshot 非空。
    #[test]
    fn live_seed_and_export() {
        let orig = compound("document", vec![atomic("hi"), atomic("世界")]);
        let mut w = Writer { buf: Vec::new() };
        w.node(&orig);
        let ir = w.buf;

        let doc = mogan_loro_doc_new();
        let rc = unsafe { mogan_loro_doc_seed(doc, ir.as_ptr(), ir.len()) };
        assert_eq!(rc, 0, "seed");

        let mut out = std::ptr::null_mut();
        let mut out_len = 0usize;
        let rc = unsafe { mogan_loro_doc_to_ir(doc, &mut out, &mut out_len) };
        assert_eq!(rc, 0, "to_ir");
        let back_bytes = unsafe { std::slice::from_raw_parts(out, out_len) }.to_vec();
        unsafe { mogan_loro_free(out, out_len) };
        let back = Reader {
            buf: &back_bytes,
            pos: 0,
        }
        .node()
        .expect("parse back");
        assert_eq!(back.label, b"document");
        assert_eq!(back.children.len(), 2);
        assert_eq!(back.children[0].text, b"hi");
        assert_eq!(back.children[1].text, "世界".as_bytes());

        let mut snap = std::ptr::null_mut();
        let mut snap_len = 0usize;
        let rc = unsafe { mogan_loro_doc_export(doc, &mut snap, &mut snap_len) };
        assert_eq!(rc, 0, "export");
        assert!(snap_len > 0, "snapshot non-empty");
        unsafe { mogan_loro_free(snap, snap_len) };

        unsafe { mogan_loro_doc_free(doc) };
    }

    /// live API：node_create + 文本 insert/delete（Phase 2 字符级镜像的核心）。
    #[test]
    fn live_node_create_and_text() {
        let doc = mogan_loro_doc_new();
        let root = unsafe {
            mogan_loro_node_create(
                doc,
                MoganTreeId {
                    peer: MOGAN_ROOT_PEER,
                    counter: 0,
                },
                0,
                KIND_COMPOUND,
                b"document".as_ptr(),
                8,
            )
        };
        assert!(root.peer != 0, "root created");
        let atom =
            unsafe { mogan_loro_node_create(doc, root, 0, KIND_ATOMIC, b"".as_ptr(), 0) };
        assert!(atom.peer != 0, "atom created");

        let rc = unsafe { mogan_loro_node_text_insert(doc, atom, 0, b"hi".as_ptr(), 2) };
        assert_eq!(rc, 0, "insert hi");
        let rc = unsafe { mogan_loro_node_text_insert(doc, atom, 2, b"x".as_ptr(), 1) };
        assert_eq!(rc, 0, "insert x → hix");
        let rc = unsafe { mogan_loro_node_text_delete(doc, atom, 2, 1) };
        assert_eq!(rc, 0, "delete → hi");

        let mut out = std::ptr::null_mut();
        let mut out_len = 0usize;
        unsafe { mogan_loro_doc_to_ir(doc, &mut out, &mut out_len) };
        let bb = unsafe { std::slice::from_raw_parts(out, out_len) }.to_vec();
        unsafe { mogan_loro_free(out, out_len) };
        let back = Reader { buf: &bb, pos: 0 }.node().expect("parse");
        assert_eq!(back.children[0].kind, KIND_ATOMIC);
        assert_eq!(back.children[0].text, b"hi");

        unsafe { mogan_loro_doc_free(doc) };
    }

    /// 聚焦诊断：split_marker 各步是否成功。运行：
    ///   cd 3rdparty/mogan-loro-ffi && cargo test split_marker_focus -- --nocapture
    #[test]
    fn split_marker_focus() {
        // (paragraph "ABCDEF")
        let ir = compound("paragraph", vec![atomic("ABCDEF")]);
        let doc = doc_from_ir(&ir).expect("doc");
        doc.commit();

        let tree = doc.get_tree(TREE_NAME);
        let para_id = tree.roots()[0];
        let kids = tree.children(TreeParentId::Node(para_id)).expect("children");
        let atom_id = kids[0]; // "ABCDEF" 原子

        // 检查 meta + text 容器
        let meta = tree.get_meta(atom_id).expect("meta");
        let txt = get_text(&meta);
        assert!(txt.is_some(), "atom must have LoroText");
        let txt = txt.unwrap();

        // 检查 get_cursor
        let cur = txt.get_cursor(3, Side::Middle);
        assert!(cur.is_some(), "get_cursor must succeed");
        let boundary = cur.unwrap();

        // 创建 SPLIT MovableList
        let list = get_or_create_split(&meta);
        assert!(list.is_some(), "get_or_create_split must succeed");
        let list = list.unwrap();

        // 插入 marker（Vec<u8> → LoroValue::Binary）
        let bytes = boundary.encode();
        let rc = list.insert(0, bytes);
        assert!(rc.is_ok(), "list.insert must succeed: {:?}", rc);

        doc.commit();

        // split_boundaries 应解析出 1 个边界
        let bounds = split_boundaries(&doc, &list);
        eprintln!("[focus] split_boundaries = {:?}", bounds);
        assert_eq!(bounds.len(), 1, "must have 1 boundary");

        // to_ir：root=paragraph，其子节点 = 扁平展开的 2 段
        let back = doc_to_ir(&doc).expect("to_ir");
        eprintln!(
            "[focus] to_ir root: kind={} n_children={}",
            back.kind,
            back.children.len()
        );
        assert_eq!(back.children.len(), 2, "root must have 2 segment children");
        assert_eq!(back.children[0].text, b"ABC", "seg0 = ABC");
        assert_eq!(back.children[1].text, b"DEF", "seg1 = DEF");
    }
}
