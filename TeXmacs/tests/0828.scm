;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0828.scm
;; DESCRIPTION : Tests for UTF-8 raw plugin I/O serializers
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

(define (utf8-tree label)
  ;; Build a UTF-8 tree directly from an stree with UTF-8 string labels.
  (stree->tree `(document ,label)))

(define (herk-tree label)
  ;; Build an internal Herk tree from an stree; string labels go through
  ;; scheme_tree_to_tree which stores them as-is (Herk in the editor).
  (stree->tree `(document ,label)))

;;; ========== texmacs->utf8raw / utf8raw->texmacs roundtrip ==========

(define (test-utf8raw-roundtrip)
  (define (roundtrip s)
    (texmacs->utf8raw (utf8raw->texmacs s)))
  ;; CJK characters
  (check (roundtrip "中文测试") => "中文测试")
  ;; Greek letters
  (check (roundtrip "αβγδ") => "αβγδ")
  ;; Mixed ASCII and CJK
  (check (roundtrip "Hello 世界") => "Hello 世界")
  ;; Math symbols
  (check (roundtrip "∑∏∫√") => "∑∏∫√")
  ;; Emoji
  (check (roundtrip "🎉🎊") => "🎉🎊")
  ;; Newlines become document children and back
  (check (roundtrip "line1\nline2") => "line1\nline2")
  ;; Multiple newlines
  (check (roundtrip "a\n\nb") => "a\n\nb")
  ;; Empty string
  (check (roundtrip "") => "")
  ;; Only whitespace
  (check (roundtrip "  ") => "  ")
) ;define

;;; ========== texmacs->utf8raw from UTF-8 tree ==========

(define (test-utf8raw-from-tree)
  (check (texmacs->utf8raw (utf8-tree "中文测试")) => "中文测试")
  (check (texmacs->utf8raw (utf8-tree "αβγδ")) => "αβγδ")
  (check (texmacs->utf8raw (utf8-tree "Hello 世界")) => "Hello 世界")
  (check (texmacs->utf8raw (utf8-tree "∑∏∫√")) => "∑∏∫√")
  ;; Multi-line document
  (check (texmacs->utf8raw (stree->tree '(document "line1" "line2"))) => "line1\nline2")
) ;define

;;; ========== utf8raw->texmacs preserves raw bytes ==========

(define (test-utf8raw-to-tree)
  ;; Literal <#XXXX> in a utf8: block must stay literal, not be decoded.
  (define t (utf8raw->texmacs "<#5206><#5B50>"))
  (check (texmacs->utf8raw t) => "<#5206><#5B50>")
  ;; CR/LF normalization
  (check (texmacs->utf8raw (utf8raw->texmacs "a\r\nb")) => "a\nb")
  (check (texmacs->utf8raw (utf8raw->texmacs "a\rb")) => "a\nb")
) ;define

;;; ========== Default plugin serializer uses UTF-8 raw ==========

(define (test-utf8raw-serialize-utf8)
  ;; utf8raw-serialize must emit raw UTF-8 bytes, not cork/SourceCode.
  (define s (utf8raw-serialize "python" (utf8-tree "print('中文')")))
  (check (string-contains? s "中文") => #t)
  (check (string-contains? s "<#4E2D>") => #f)
) ;define

(define (test-generic-serialize-utf8)
  ;; generic-serialize must emit a utf8: block with raw UTF-8 content.
  (define s (generic-serialize "python" (utf8-tree "print('中文')")))
  (check (string-starts? s (char->string #\x02)) => #t)
  (check (string-contains? s "utf8:") => #t)
  (check (string-contains? s "中文") => #t)
  (check (string-contains? s "<#4E2D>") => #f)
  (check (string-ends? s (char->string #\x05)) => #t)
) ;define

;;; ========== Plugin custom serializers use UTF-8 raw ==========

(define (load-plugin-init name)
  (load (string-append (url->system (get-texmacs-path))
                       "/plugins/" name "/progs/init-" name ".scm")))

(define (plugin-serialize-utf8 lan label)
  ;; plugin-serialize expects an stree; build a UTF-8 stree directly.
  (plugin-serialize lan `(document ,label)))

(define (test-python-serialize-utf8)
  (load-plugin-init "python")
  (with s (plugin-serialize-utf8 "python" "print('中文')")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-julia-serialize-utf8)
  (load-plugin-init "julia")
  (with s (plugin-serialize-utf8 "julia" "println(\"中文\")")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-goldfish-serialize-utf8)
  (load-plugin-init "goldfish")
  (with s (plugin-serialize-utf8 "goldfish" "(display \"中文\")")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-gnuplot-serialize-utf8)
  (load-plugin-init "gnuplot")
  (with s (plugin-serialize-utf8 "gnuplot" "set title \"中文\"")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-autosave-serialize-utf8)
  (load-plugin-init "autosave")
  (with s (plugin-serialize-utf8 "autosave" "(display \"中文\")")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-tikz-serialize-utf8)
  (load-plugin-init "tikz")
  (with s (plugin-serialize-utf8 "tikz" "\\node {中文};")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

(define (test-quiver-serialize-utf8)
  (load-plugin-init "quiver")
  (with s (plugin-serialize-utf8 "quiver" "\\node {中文};")
    (check (string-contains? s "中文") => #t)
    (check (string-contains? s "<#4E2D>") => #f)
    (when (string-contains? s "<EOF>")
      (check (string-ends? s "\n<EOF>\n") => #t))))

;;; ========== Input side: utf8raw->texmacs then Herk conversion ==========

(define (test-input-utf8-to-herk)
  ;; Simulate what input.cpp does: take a utf8: payload, parse to UTF-8 tree,
  ;; then convert to internal Herk tree.
  (define utf8-tree (utf8raw->texmacs "print('中文')"))
  (define herk-tree (utf8-tree->herk-tree utf8-tree))
  (define herk-str (texmacs->stm herk-tree))
  (check (string-contains? herk-str "<#4E2D>") => #t)
  (check (string-contains? herk-str "<#6587>") => #t)
  (check (string-contains? herk-str "中文") => #f)
) ;define

;;; ========== Herk tree -> UTF-8 tree -> raw UTF-8 string ==========

(define (test-write-path-herk-to-utf8raw)
  ;; Simulate the main-process-to-plugin path: internal Herk tree is first
  ;; converted to UTF-8 tree, then serialized by texmacs->utf8raw.
  (define herk-tree (stm-snippet->texmacs "(frac \"<#5206><#5B50>\" \"<#5206><#6BCD>\")"))
  (define utf8-tree (herk-tree->utf8-tree herk-tree))
  (define s (texmacs->utf8raw utf8-tree))
  (check (string-contains? s "分子") => #t)
  (check (string-contains? s "分母") => #t)
  (check (string-contains? s "<#5206>") => #f)
) ;define

;;; ========== Test entry ==========

(tm-define (test_0828)
  (test-utf8raw-roundtrip)
  (test-utf8raw-from-tree)
  (test-utf8raw-to-tree)
  (test-utf8raw-serialize-utf8)
  (test-generic-serialize-utf8)
  (test-python-serialize-utf8)
  (test-julia-serialize-utf8)
  (test-goldfish-serialize-utf8)
  (test-gnuplot-serialize-utf8)
  (test-autosave-serialize-utf8)
  (test-tikz-serialize-utf8)
  (test-quiver-serialize-utf8)
  (test-input-utf8-to-herk)
  (test-write-path-herk-to-utf8raw)
  (check-report))
