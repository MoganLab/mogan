
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : autosave-prune-test.scm
;; DESCRIPTION : Unit tests for (liii autosave) autosave-prune-dir
;; COPYRIGHT   : (C) 2026 Mogan Developers
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; ; 让 (liii autosave) 能被解析:
;; ; 运行入口是 `cd TeXmacs/plugins/autosave/goldfish && goldfish tests/autosave-prune-test.scm`
;; ; 所以 (getcwd) = 模块根目录,liii/autosave.scm 在其下
(import (liii os) (scheme base))
(set! *load-path* (cons (getcwd) *load-path*))

(import (liii check)
  (liii autosave)
  (liii list)
  (liii path)
  (liii os)
  (liii base)
  (srfi srfi-13)
) ;import

(check-set-mode! 'report-failed)

;; 辅助:在 dir 下生成名为 name 的空 .tmu 文件

(define (touch-tmu dir name)
  (path-touch (path-join dir (string-append name ".tmu")))
) ;define

;; 辅助:统计 dir 下 .tmu 文件数

(define (count-tmu dir)
  (let ((entries (vector->list (path-list dir))))
    (length (filter (lambda (n) (string-suffix? ".tmu" n)) entries))
  ) ;let
) ;define

;; 辅助:清空 dir 下所有 .tmu,然后 rmdir

(define (wipe dir)
  (for-each (lambda (n) (when (string-suffix? ".tmu" n) (path-unlink (path-join dir n) #t)))
    (vector->list (path-list dir))
  ) ;for-each
  (path-rmdir dir)
) ;define

;; 场景 1:目录有 11 份,prune 后保留 keep-max-1 = 9 份(为复制后 +1 让位)
(let ((base (path-join (os-temp-dir) "autosave-prune-11")))
  (when (path-exists? base)
    (wipe base)
  ) ;when
  (mkdir (path->string base))
  (for-each (lambda (n)
              (touch-tmu base (string-append "2026010" (number->string n) "_120000"))
            ) ;lambda
    (list 0 1 2 3 4 5 6 7 8 9 10)
  ) ;for-each
  (check (count-tmu base) => 11)

  (autosave-prune-dir base)

  (check (count-tmu base) => (- autosave-keep-max 1))
  (wipe base)
) ;let

;; 场景 2:目录恰好 keep-max 份,prune 后保留 keep-max-1 = 9 份
(let ((base (path-join (os-temp-dir) "autosave-prune-exact")))
  (when (path-exists? base)
    (wipe base)
  ) ;when
  (mkdir (path->string base))
  (for-each (lambda (n)
              (touch-tmu base (string-append "2026010" (number->string n) "_120000"))
            ) ;lambda
    (list 0 1 2 3 4 5 6 7 8 9)
  ) ;for-each
  (check (count-tmu base) => autosave-keep-max)

  (autosave-prune-dir base)

  (check (count-tmu base) => (- autosave-keep-max 1))
  (wipe base)
) ;let

;; 场景 3:目录不足 keep-max 份,prune 后不动
(let ((base (path-join (os-temp-dir) "autosave-prune-few")))
  (when (path-exists? base)
    (wipe base)
  ) ;when
  (mkdir (path->string base))
  (for-each (lambda (n)
              (touch-tmu base (string-append "2026010" (number->string n) "_120000"))
            ) ;lambda
    (list 0 1 2)
  ) ;for-each
  (check (count-tmu base) => 3)

  (autosave-prune-dir base)

  (check (count-tmu base) => 3)
  (wipe base)
) ;let

;; 场景 4:目录远超 keep-max(20 份),prune 后保留 9 份
(let ((base (path-join (os-temp-dir) "autosave-prune-many")))
  (when (path-exists? base)
    (wipe base)
  ) ;when
  (mkdir (path->string base))
  (for-each (lambda (n)
              (touch-tmu base
                (string-append "20260"
                  (if (< n 10) "10" "11")
                  (if (< n 10) (number->string n) (number->string (- n 10)))
                  "_120000"
                ) ;string-append
              ) ;touch-tmu
            ) ;lambda
    (list 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19)
  ) ;for-each
  (check (count-tmu base) => 20)

  (autosave-prune-dir base)

  (check (count-tmu base) => (- autosave-keep-max 1))
  (wipe base)
) ;let

;; 场景 5:目录不存在,prune 不报错
(check-true (autosave-prune-dir (path-join (os-temp-dir) "autosave-prune-nope-xyz"))
) ;check-true

(check-report)
