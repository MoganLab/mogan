;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0827.scm
;; DESCRIPTION : Tests for UTF-8 plugin I/O protocol adaptation
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (scheme base)
        (liii check)
        (liii string))

(check-set-mode! 'report-failed)

;;; ========== Helpers ==========

(define (utf8-verbatim-opts)
  (acons "verbatim->texmacs:encoding" "utf-8" '()))

(define (utf8-verbatim-out-opts)
  (acons "texmacs->verbatim:encoding" "utf-8" '()))

(define (roundtrip-utf8-verbatim s)
  (texmacs->verbatim-snippet
    (verbatim-snippet->texmacs s (utf8-verbatim-opts))
    (utf8-verbatim-out-opts)))

(define (scheme-snippet->string s)
  (texmacs->stm (stm-snippet->texmacs s)))

;;; ========== Verbatim UTF-8 roundtrip ==========

(define (test-verbatim-utf8-roundtrip)
  ;; CJK characters
  (check (roundtrip-utf8-verbatim "中文测试") => "中文测试")
  ;; Greek letters
  (check (roundtrip-utf8-verbatim "αβγδ") => "αβγδ")
  ;; Mixed ASCII and CJK
  (check (roundtrip-utf8-verbatim "Hello 世界") => "Hello 世界")
  ;; Math symbols
  (check (roundtrip-utf8-verbatim "∑∏∫√") => "∑∏∫√")
) ;define

;;; ========== scheme: channel with UTF-8 strings ==========

(define (test-scheme-utf8-string)
  ;; CJK inside Scheme string literal
  (check (string-contains? (scheme-snippet->string "(frac \"分子\" \"分母\")")
                           "分子") => #t)
  (check (string-contains? (scheme-snippet->string "(frac \"分子\" \"分母\")")
                           "分母") => #t)

  ;; Greek letters inside Scheme string literal
  (check (string-contains? (scheme-snippet->string "(greek \"αβγ\")")
                           "αβγ") => #t)

  ;; Emoji inside Scheme string literal
  (check (string-contains? (scheme-snippet->string "(emoji \"🎉\")")
                           "🎉") => #t)

  ;; Mixed ASCII and CJK
  (check (string-contains? (scheme-snippet->string "(document \"Hello 世界\")")
                           "Hello 世界") => #t)
) ;define

;;; ========== scheme: channel with UTF-8 unquoted symbols ==========

(define (test-scheme-utf8-symbol)
  ;; Unquoted UTF-8 symbols should also survive parsing
  (check (string-contains? (scheme-snippet->string "(chinese 汉语 条目)")
                           "汉语") => #t)
  (check (string-contains? (scheme-snippet->string "(chinese 汉语 条目)")
                           "条目") => #t)
) ;define

;;; ========== utf8-tree -> herk-tree conversion ==========

(define (test-utf8-tree-to-herk-tree)
  (define utf8-tree (stm-snippet->texmacs "(frac \"分子\" \"分母\")"))
  (define herk-tree (utf8-tree->herk-tree utf8-tree))
  (define herk-str (texmacs->stm herk-tree))

  ;; CJK codepoints should become <#XXXX> escapes in Herk
  (check (string-contains? herk-str "<#5206>") => #t)
  (check (string-contains? herk-str "<#5B50>") => #t)
  (check (string-contains? herk-str "<#6BCD>") => #t)

  ;; Greek letters that are in Herk table should become single bytes,
  ;; but the original UTF-8 should no longer be present
  (define greek-utf8 (stm-snippet->texmacs "(greek \"αβγ\")"))
  (define greek-herk (utf8-tree->herk-tree greek-utf8))
  (define greek-herk-str (texmacs->stm greek-herk))
  (check (string-contains? greek-herk-str "αβγ") => #f)
) ;define

;;; ========== <#XXXX> literal preservation ==========

(define (test-cork-escape-preservation)
  ;; Plugins may send literal <#XXXX> instead of raw UTF-8 bytes.
  ;; These should be preserved through scheme_to_tree and tree_utf8_to_herk.
  (define t (stm-snippet->texmacs "(frac <#5206><#5B50> <#5206><#6BCD>)"))
  (define herk-t (utf8-tree->herk-tree t))
  (define herk-str (texmacs->stm herk-t))

  (check (string-contains? herk-str "<#5206>") => #t)
  (check (string-contains? herk-str "<#5B50>") => #t)
  (check (string-contains? herk-str "<#6BCD>") => #t)
) ;define

;;; ========== Backward compatibility: old scheme_u8 payload ==========

(define (test-scheme-u8-backward-compat)
  ;; The protocol still accepts scheme_u8: blocks; they are handled
  ;; exactly like scheme: blocks.  We cannot send a raw block from
  ;; Scheme easily, but we can verify the conversion function path
  ;; that scheme_u8: would have used.
  (define utf8-tree (stm-snippet->texmacs "(frac \"分子\" \"分母\")"))
  (define herk-tree (utf8-tree->herk-tree utf8-tree))
  (check (tree? herk-tree) => #t)
) ;define

;;; ========== Test entry ==========

(tm-define (test_0827)
  (test-verbatim-utf8-roundtrip)
  (test-scheme-utf8-string)
  (test-scheme-utf8-symbol)
  (test-utf8-tree-to-herk-tree)
  (test-cork-escape-preservation)
  (test-scheme-u8-backward-compat)
  (check-report))
