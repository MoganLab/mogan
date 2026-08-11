;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-collab-test.scm
;; DESCRIPTION : 纯逻辑单元测试：协作文档显示名的校验（collab-valid-doc-name?）、
;;               /docs 交替列表解构（collab-docs-pairs）与菜单标签
;;               （collab-doc-label）。规则与服务端 tools/loro-server/validate.js
;;               保持一致。不弹任何 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026  Jim Zhou
;;
;; USAGE
;;   xmake b stem
;;   xmake r tm-collab-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check) (liii unicode))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/texmacs/tm-collab.scm")

;; collab-valid-doc-name?：合法输入——CJK、空格、连字符、下划线、边界长度
;; （长度按字符计：utf8-string-length，与服务端 code point 计数一致）。

(define (test-valid-names)
  (check (collab-valid-doc-name? "文档") => #t)
  (check (collab-valid-doc-name? "会议纪要 2026-08") => #t)
  (check (collab-valid-doc-name? "my doc_v2-final") => #t)
  (check (collab-valid-doc-name? "a") => #t)
  (check (collab-valid-doc-name? (make-string 64 #\a)) => #t)
  (check (collab-valid-doc-name? (utf8-make-string 64 #\中)) => #t)
) ;define

;; collab-valid-doc-name?：非法输入——空、超长、禁字符、控制字符、非字符串。

(define (test-invalid-names)
  (check (collab-valid-doc-name? "") => #f)
  (check (collab-valid-doc-name? (make-string 65 #\a)) => #f)
  (check (collab-valid-doc-name? (utf8-make-string 65 #\中)) => #f)
  (for-each (lambda (bad)
              (check (collab-valid-doc-name? (string-append "a" bad "b")) => #f)
            ) ;lambda
    (list "\\" "/" ":" "*" "?" "\"" "<" ">" "|" "\t" (string (integer->char 127)))
  ) ;for-each
  (check (collab-valid-doc-name? 42) => #f)
) ;define

;; collab-docs-pairs：扁平交替列表 → (uuid . name) 对。

(define (test-docs-pairs)
  (check (collab-docs-pairs '()) => '())
  (check (collab-docs-pairs '("u1" "名字" "u2" ""))
    =>
    '(("u1" . "名字") ("u2" . ""))
  ) ;check
  ;; 防御奇数长度：末尾落单的忽略
  (check (collab-docs-pairs '("u1" "n1" "u3")) => '(("u1" . "n1")))
) ;define

;; collab-doc-label：唯一有名 → (verbatim name)；重名 → 追加灰色 uuid 前 4 位；
;; 无名/空名 → uuid 全文。

(define (test-doc-label)
  (check (collab-doc-label "abcd1234-0000" "周报" #f) => '(verbatim "周报"))
  (check (collab-doc-label "abcd1234-0000" "周报" #t)
    =>
    '(concat (verbatim "周报")
       " "
       (with "color" "dark grey" (verbatim "(abcd)")))
  ) ;check
  (check (collab-doc-label "abcd1234-0000" "" #f) => "abcd1234-0000")
  ;; 防御：uuid 不足 4 位不越界
  (check (collab-doc-label "ab" "n" #t)
    =>
    '(concat (verbatim "n") " " (with "color" "dark grey" (verbatim "(ab)")))
  ) ;check
) ;define

;; collab-fields->url：地址+端口 → 完整 URL。地址为完整 ws(s):// URL 原样保留；
;; 地址空 → 清除（空串）；仅 host/缺端口 → 不附 ":port"。

(define (test-fields->url)
  (check (collab-fields->url "1.2.3.4" "8765") => "ws://1.2.3.4:8765")
  (check (collab-fields->url "host" "") => "ws://host")
  (check (collab-fields->url "wss://x.com:443" "") => "wss://x.com:443")
  (check (collab-fields->url "" "8765") => "")
  ;; 完整 URL 优先，端口忽略
  (check (collab-fields->url "ws://[::1]:8765" "9999") => "ws://[::1]:8765")
) ;define

;; collab-url->fields：完整 URL → (address . port) 回填两框。仅常见 ws(s)://host:port
;; 才拆；含路径 / 多冒号（IPv6）/ 非 ws(s) scheme → 整串塞进 address（端口空）。

(define (test-url->fields)
  (check (collab-url->fields "ws://1.2.3.4:8765") => '("1.2.3.4" . "8765"))
  (check (collab-url->fields "wss://x.com:443") => '("x.com" . "443"))
  (check (collab-url->fields "ws://h") => '("h" . ""))
  ;; 含路径 → 整串回填
  (check (collab-url->fields "ws://h:1/p") => '("ws://h:1/p" . ""))
  ;; 多冒号（IPv6）→ 整串回填
  (check (collab-url->fields "ws://[::1]:8765") => '("ws://[::1]:8765" . ""))
  ;; 非 ws(s) scheme → 整串回填
  (check (collab-url->fields "http://x") => '("http://x" . ""))
) ;define

;; collab-doc-name-duplicates：仅非空 name 参与统计。

(define (test-doc-name-duplicates)
  (let ((counts (collab-doc-name-duplicates '(("u1" . "周报")
                                              ("u2" . "周报")
                                              ("u3" "")
                                              ("u4" . "月报"))
                ) ;collab-doc-name-duplicates
        ) ;counts
       ) ;
    (check (assoc "周报" counts) => '("周报" . 2))
    (check (assoc "月报" counts) => '("月报" . 1))
    (check (assoc "" counts) => #f)
  ) ;let
) ;define

;; cork->utf8：还原 Qt 输入框 Cork 化后的 <#XXXX> 转义。GUI 输入中文经
;; from_qstring→utf8_to_cork 变成 <#XXXX>，协作入口需 cork->utf8 还原为 UTF-8
;; 才能校验/传输。此用例覆盖修复核心——现有 UTF-8 字面量用例测不到该路径
;; （正是原 bug 漏网的原因）。

(define (test-cork-decode)
  (check (cork->utf8 "<#6D4B><#8BD5>") => "测试")
  (check (cork->utf8 "<#6D4B><#8BD5><#6587><#6863>") => "测试文档")
  ;; ASCII 透明（英文文档名不受 Cork 影响）
  (check (cork->utf8 "abc") => "abc")
  ;; 混合：ASCII + CJK 转义
  (check (cork->utf8 "a<#6D4B>b") => "a测b")
  ;; 协作入口视角：Cork 化的中文文档名还原为 UTF-8 后应通过校验
  (check (collab-valid-doc-name? (cork->utf8 "<#6D4B><#8BD5><#6587><#6863>"))
    =>
    #t
  ) ;check
) ;define

;; collab-buffer-url->tmfs / collab-buffer? / collab-placeholder-doc-id：
;; 协作 buffer 的 tmfs URL 构造与谓词（取代旧的 URL 外单值变量标识）。

(define (test-collab-url)
  ;; URL 构造：doc_id（含 UUID 连字符）→ tmfs://collab/<doc_id>
  (check (url->unix (collab-buffer-url->tmfs "abc-123"))
    =>
    "tmfs://collab/abc-123"
  ) ;check
  (check (url->unix (collab-buffer-url->tmfs "550e8400-e29b-41d4-a716-446655440000"))
    =>
    "tmfs://collab/550e8400-e29b-41d4-a716-446655440000"
  ) ;check
  ;; doc_id 提取（collab-url->doc-id）：与 collab-buffer-url->tmfs 互逆；含 UUID 连字符
  (check (collab-url->doc-id (collab-buffer-url->tmfs "550e8400-e29b-41d4-a716-446655440000")
         ) ;collab-url->doc-id
    =>
    "550e8400-e29b-41d4-a716-446655440000"
  ) ;check
  ;; 占位 doc_id 也被正确切出（是否跳过由调用方 collab-do-silent-backup 决定）
  (check (collab-url->doc-id (unix->url "tmfs://collab/pending-1-2"))
    =>
    "pending-1-2"
  ) ;check
  ;; 谓词：仅 collab 协议为 #t（含 pending 占位），chat / 文件路径为 #f
  (check (collab-buffer? (unix->url "tmfs://collab/abc")) => #t)
  (check (collab-buffer? (unix->url "tmfs://collab/pending-1-2")) => #t)
  (check (collab-buffer? (unix->url "tmfs://chat/x")) => #f)
  (check (collab-buffer? (unix->url "/tmp/x.tmu")) => #f)
  ;; 占位 id 唯一：连续两次调用 counter 递增，结果不同
  (check (string=? (collab-placeholder-doc-id) (collab-placeholder-doc-id)) => #f)
) ;define

(tm-define (regtest-tm-collab)
  (test-valid-names)
  (test-invalid-names)
  (test-docs-pairs)
  (test-doc-label)
  (test-doc-name-duplicates)
  (test-cork-decode)
  (test-fields->url)
  (test-url->fields)
  (test-collab-url)
  (check-report)
) ;tm-define
