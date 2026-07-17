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
    Container, ExportMode, LoroDoc, LoroMap, LoroText, LoroTree, Subscription, TreeID,
    TreeParentId, ValueOrContainer,
};

// doc 句柄 -> 其 local-update 订阅。Subscription 必须保活，否则自动取消订阅。
static LOCAL_SUBS: OnceLock<Mutex<HashMap<usize, Subscription>>> = OnceLock::new();
fn local_subs() -> &'static Mutex<HashMap<usize, Subscription>> {
    LOCAL_SUBS.get_or_init(|| Mutex::new(HashMap::new()))
}

const KIND_ATOMIC: u8 = 0;
const KIND_COMPOUND: u8 = 1;
const KIND_GENERIC: u8 = 2;

/// LoroTree 容器名（C++ 侧也用同名 root tree）
const TREE_NAME: &str = "tree";

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

fn read_node(tree: &LoroTree, id: TreeID) -> Result<IrNode, ()> {
    let meta = tree.get_meta(id).map_err(|_| ())?;
    let kind = get_meta_str(&meta, "kind").unwrap_or_default();
    let kind_byte = str_to_kind(&kind);
    let label = get_meta_bytes(&meta, "label").unwrap_or_default();
    let text = get_meta_text_or_binary(&meta).unwrap_or_default();
    let child_ids = tree.children(TreeParentId::Node(id)).unwrap_or_default();
    let mut children = Vec::with_capacity(child_ids.len());
    for cid in child_ids {
        children.push(read_node(tree, cid)?);
    }
    Ok(IrNode {
        kind: kind_byte,
        label,
        text,
        children,
    })
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

/// 递归写出"带 TreeID 的增强 IR"：每节点前缀 peer:u64 counter:i32，再跟普通 IR 节点。
/// 用于 Phase 3：B 导入后据此把 buffer rep 关联到导入的 TreeID，使 B 的后续编辑能合并。
fn write_node_with_id(tree: &LoroTree, id: TreeID, w: &mut Writer) {
    w.buf.extend_from_slice(&id.peer.to_le_bytes());
    w.buf.extend_from_slice(&id.counter.to_le_bytes());
    let (kind_byte, label, text, child_ids) = match tree.get_meta(id) {
        Ok(meta) => {
            let kind = get_meta_str(&meta, "kind").unwrap_or_default();
            let label = get_meta_bytes(&meta, "label").unwrap_or_default();
            let text = get_meta_text_or_binary(&meta).unwrap_or_default();
            let children = tree.children(TreeParentId::Node(id)).unwrap_or_default();
            (str_to_kind(&kind), label, text, children)
        }
        Err(_) => (KIND_COMPOUND, Vec::new(), Vec::new(), Vec::new()),
    };
    w.u8(kind_byte);
    w.bytes_field(&label);
    w.bytes_field(&text);
    w.u32(child_ids.len() as u32);
    for cid in child_ids {
        write_node_with_id(tree, cid, w);
    }
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
}
