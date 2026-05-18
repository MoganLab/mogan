
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-filesystem-test.scm
;; DESCRIPTION : Test suite for tm-file-system
;; COPYRIGHT   : (C) 2025  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-filesystem-test)
  (:use (kernel texmacs tm-file-system))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for tmfs-decompose-name
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-tmfs-decompose-name)
  ;; 标准路径分解
  (check (tmfs-decompose-name "hello/world") => '("hello" "world"))
  ;; 无斜杠时默认类别为 "file"
  (check (tmfs-decompose-name "plain") => '("file" "plain"))
  ;; 去除 tmfs:// 前缀
  (check (tmfs-decompose-name "tmfs://aux/test") => '("aux" "test"))
  ;; 空字符串
  (check (tmfs-decompose-name "") => '("file" ""))
  ;; 多级路径
  (check (tmfs-decompose-name "a/b/c/d") => '("a" "b/c/d"))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for tmfs-pair, tmfs-car, tmfs-cdr
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-tmfs-pair)
  ;; 包含斜杠的路径
  (check (integer? (tmfs-pair? "foo/bar")) => #t)
  ;; 不包含斜杠的路径
  (check (tmfs-pair? "plain") => #f)
  ;; tmfs-car 取第一个组件
  (check (tmfs-car "foo/bar") => "foo")
  (check (tmfs-car "foo/bar/baz") => "foo")
  ;; 无斜杠时返回 #f
  (check (tmfs-car "plain") => #f)
  ;; tmfs-cdr 取剩余部分
  (check (tmfs-cdr "foo/bar") => "bar")
  (check (tmfs-cdr "foo/bar/baz") => "bar/baz")
  ;; 无斜杠时返回 #f
  (check (tmfs-cdr "plain") => #f)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for tmfs->list and list->tmfs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-tmfs-list)
  ;; 多级路径分解为列表
  (check (tmfs->list "a/b/c") => '("a" "b" "c"))
  ;; 单元素路径
  (check (tmfs->list "plain") => '("plain"))
  ;; 空字符串
  (check (tmfs->list "") => '(""))
  ;; 列表组合为路径
  (check (list->tmfs '("a" "b" "c")) => "a/b/c")
  ;; 单元素列表
  (check (list->tmfs '("plain")) => "plain")
  ;; 互逆测试
  (check (list->tmfs (tmfs->list "a/b/c/d")) => "a/b/c/d")
  (check (tmfs->list (list->tmfs '("x" "y" "z"))) => '("x" "y" "z"))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for list->query, query->list, query-ref
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-query)
  ;; 基本键值对序列化
  (check (list->query '(("a" . "1") ("b" . "2"))) => "a=1&b=2")
  ;; 空列表
  (check (list->query '()) => "")
  ;; 单键值对
  (check (list->query '(("key" . "value"))) => "key=value")
  ;; 包含冒号的值需要转义
  (check (list->query '(("a" . "b:c"))) => "a=b%3Ac")
  ;; 解析查询字符串
  (check (query->list "a=1&b=2") => '(("a" . "1") ("b" . "2")))
  ;; 空字符串解析为空列表
  (check (query->list "") => '(("" . "")))
  ;; 反转义测试
  (check (query->list "a=b%3Ac") => '(("a" . "b:c")))
  ;; 互逆测试
  (check (query->list (list->query '(("x" . "1") ("y" . "2"))))
    =>
    '(("x" . "1") ("y" . "2"))
  ) ;check
  ;; query-ref 提取变量值
  (check (query-ref "a=1&b=2" "a") => "1")
  (check (query-ref "a=1&b=2" "b") => "2")
  ;; 不存在的变量返回空字符串
  (check (query-ref "a=1&b=2" "c") => "")
  ;; 空查询字符串
  (check (query-ref "" "a") => "")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for strip-colon
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-strip-colon)
  ;; Windows 盘符路径：去掉冒号，保留斜杠
  (check (strip-colon "C:/foo") => "C/foo")
  (check (strip-colon "D:/bar/baz") => "D/bar/baz")
  ;; Unix 路径不受影响
  (check (strip-colon "/foo") => "/foo")
  ;; 普通字符串不受影响
  (check (strip-colon "foo") => "foo")
  ;; 不以字母开头的不受影响
  (check (strip-colon ":/foo") => ":/foo")
  ;; 长度不足的不受影响
  (check (strip-colon "C:") => "C:")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for tmfs-handler and tmfs-load
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-tmfs-handler)
  ;; 注册测试加载处理器
  (tmfs-handler "testload" 'load (lambda (name) (string-append "loaded: " name)))
  (check (tmfs-load "tmfs://testload/doc") => "loaded: doc")
  ;; 注册返回非字符串的处理器（自动通过 object->tmstring 转换）
  (tmfs-handler "testobj" 'load (lambda (name) '(a b c)))
  (check (string? (tmfs-load "tmfs://testobj/any")) => #t)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for lazy-tmfs-handler and lazy-tmfs-force
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-lazy-tmfs)
  ;; 注册延迟处理器（使用符号作为类别名）
  (lazy-tmfs-handler (kernel texmacs tm-file-system) lazytest)
  ;; 首次调用前表中应存在该条目
  (check (ahash-ref lazy-tmfs-table 'lazytest)
    =>
    '(kernel texmacs tm-file-system)
  ) ;check
  ;; 调用 lazy-tmfs-force 后应从表中移除
  (lazy-tmfs-force "lazytest")
  (check (ahash-ref lazy-tmfs-table 'lazytest) => #f)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for tmfs-master, tmfs-format, tmfs-remote?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-tmfs-auxiliary)
  ;; 无 master 处理器时返回原 URL
  (check (tmfs-master "tmfs://unknown/path") => "tmfs://unknown/path")
  ;; 默认格式为 "stm"
  (check (tmfs-format "tmfs://any/file") => "stm")
  ;; 注册自定义格式处理器
  (tmfs-handler "fmtclass" 'format (lambda (name) "scm"))
  (check (tmfs-format "tmfs://fmtclass/test") => "scm")
  ;; 无 load 处理器时认为是远程的
  (check (tmfs-remote? "tmfs://remote/path") => #t)
  ;; 有 load 处理器时不是远程的
  (tmfs-handler "localclass" 'load (lambda (name) name))
  (check (tmfs-remote? "tmfs://localclass/path") => #f)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for aux-name
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-aux-name)
  ;; 辅助缓冲区名称转换为 tmfs URL
  (check (url->system (aux-name "test")) => "tmfs://aux/test")
  (check (url->system (aux-name "my-buffer")) => "tmfs://aux/my-buffer")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for object->tmstring
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-object-tmstring)
  ;; 列表序列化为字符串
  (check (string? (object->tmstring '(1 2 3))) => #t)
  ;; 字符串序列化后会加上引号
  (check (string? (object->tmstring "hello")) => #t)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-tm-filesystem)
  (test-tmfs-decompose-name)
  (test-tmfs-pair)
  (test-tmfs-list)
  (test-query)
  (test-strip-colon)
  (test-tmfs-handler)
  (test-lazy-tmfs)
  (test-tmfs-auxiliary)
  (test-aux-name)
  (test-object-tmstring)
  (check-report)
) ;tm-define
