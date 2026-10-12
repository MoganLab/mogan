;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : overseas-test.scm
;; DESCRIPTION : 海外版编译条件与默认语言纯逻辑单元测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r overseas-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/texmacs/tm-server.scm")

(define (test-overseas-predicate)
  (check (boolean? (overseas-stem?)) => #t)
) ;define

(define (test-default-language)
  (check (get-default-language)
    =>
    (if (overseas-stem?) "english" (get-locale-language))
  ) ;check
) ;define

(tm-define (regtest-overseas)
  (test-overseas-predicate)
  (test-default-language)
  (check-report)
) ;tm-define
