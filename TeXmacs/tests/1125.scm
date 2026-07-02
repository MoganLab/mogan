;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1125.scm
;; DESCRIPTION : Performance baseline for utf8->cork / cork->utf8 /
;;               strict-cork->utf8. Run with `xmake b 1125 && xmake r 1125`.
;;               Same script is used both before and after the migration to
;;               lolly/Data/String/cork.cpp, so the speedup is directly
;;               comparable without any C++ test-code changes.
;;
;;               In TeXmacs Scheme, `string` is a byte sequence. Source
;;               literals containing non-ASCII characters are read as raw
;;               UTF-8 bytes, which is exactly what utf8->cork consumes.
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii timeit))

;; --- Test corpora ------------------------------------------------------------
;; Each is a moderately sized string chosen to stress one path:
;;   - ascii    : pure ASCII, hits the fast passthrough path
;;   - latin1   : every non-ASCII Latin-1 codepoint (U+0080..U+00FF), each
;;                resolves to a single Cork byte
;;   - cjk      : a block of CJK ideographs, none have a Cork mapping and all
;;                escape to <#XXXX>
;;   - named    : math/named symbols (<alpha>, <beta>, ...) which map to/from
;;                multi-byte ASCII sequences in cork_to_utf8 / utf8_to_cork
;;   - mixed    : interleaved ASCII + CJK + accented Latin, realistic document

(define ascii-text
  (string-append
    "The quick brown fox jumps over the lazy dog. "
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."))

(define latin1-text
  (string-append
    "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞß"
    "àáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ"
    "¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿"))

(define cjk-text
  (string-append
    "春江潮水连海平，海上明月共潮生。"
    "滟滟随波千万里，何处春江无月明。"
    "江流宛转绕芳甸，月照花林皆似霰。"
    "空里流霜不觉飞，汀上白沙看不见。"
    "江天一色无纤尘，皎皎空中孤月轮。"
    "江畔何人初见月？江月何年初照人？"
    "人生代代无穷已，江月年年望相似。"))

(define named-text
  (string-append
    "<alpha><beta><gamma><delta><epsilon><zeta><eta><theta>"
    "<iota><kappa><lambda><mu><nu><xi><omicron><pi>"
    "<rho><sigma><tau><upsilon><phi><chi><psi><omega>"
    "<Alpha><Beta><Gamma><Delta><Epsilon><Zeta><Eta><Theta>"
    "<sum><prod><int><oint><bigcup><bigcap><bigsqcup>"
    "<leq><geq><neq><equiv><subset><supset><in><ni>"
    "<rightarrow><leftarrow><Rightarrow><Leftarrow><mapsto>"
    "<infty><partial><nabla><forall><exists><neg><wedge><vee>"))

(define mixed-text
  (string-append
    "In Björk we trust: "
    "<alpha> + <beta> = <gamma>，而 中文字符 与 Latin-1 àéîõü 混排。"))

;; Precompute cork-encoded variants of the corpora for the reverse-direction
;; benchmarks so that the timing loop only measures the conversion itself.
(define latin1-cork (utf8->cork latin1-text))
(define cjk-cork    (utf8->cork cjk-text))
(define named-cork  (utf8->cork named-text))
(define mixed-cork  (utf8->cork mixed-text))

;; --- Driver ------------------------------------------------------------------

(define (report label thunk iterations)
  (let ((total (timeit thunk '() iterations)))
    (display (string-append label
                            ": "
                            (number->string total)
                            "s for "
                            (number->string iterations)
                            " iterations\n"))))

(define (utf8->cork-bench text label iterations)
  (report (string-append "utf8->cork          " label)
          (lambda () (utf8->cork text))
          iterations))

(define (cork->utf8-bench text label iterations)
  (report (string-append "cork->utf8          " label)
          (lambda () (cork->utf8 text))
          iterations))

(define (strict-cork->utf8-bench text label iterations)
  (report (string-append "strict-cork->utf8   " label)
          (lambda () (strict-cork->utf8 text))
          iterations))

(tm-define (test_1125)
  (display "=== cork conversion performance baseline ===\n")
  (let ((n 50000))
    ;; forward direction (utf8 -> cork)
    (utf8->cork-bench ascii-text   "ascii  " n)
    (utf8->cork-bench latin1-text  "latin1 " n)
    (utf8->cork-bench cjk-text     "cjk    " n)
    (utf8->cork-bench named-text   "named  " n)
    (utf8->cork-bench mixed-text   "mixed  " n)
    ;; reverse direction (cork -> utf8)
    (cork->utf8-bench latin1-cork "latin1 " n)
    (cork->utf8-bench cjk-cork    "cjk    " n)
    (cork->utf8-bench named-cork  "named  " n)
    (cork->utf8-bench mixed-cork  "mixed  " n)
    ;; strict variant (no symbol-unicode-fallback)
    (strict-cork->utf8-bench latin1-cork "latin1 " n)
    (strict-cork->utf8-bench cjk-cork    "cjk    " n)
    (strict-cork->utf8-bench named-cork  "named  " n)
    (strict-cork->utf8-bench mixed-cork  "mixed  " n)
  ) ;let
) ;tm-define
