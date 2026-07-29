;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ghost-worker.scm
;; DESCRIPTION : Ghost 自动补全后台协议处理器
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (liii ghost-worker)
  (export ghost-worker-handle)
  (import (scheme base)
          (liii json)
          (liii ghost-api)
          (texmacs protocol)
  ) ;import
  (begin
    ;; 接收主进程消息，请求 DeepSeek，回传 tokens/logprobs
    (define (ghost-worker-handle payload)
      (let* ((j (string->json payload))
             (res (ghost-deepseek-complete (json-ref-string j "prefix" "")
                                           (json-ref-string j "suffix" "")))
             (out-json (json->string
                         (list (cons "tokens" (car res))
                               (cons "logprobs" (cdr res))))))
        (flush-scheme `(document ,out-json)))
    ) ;define
  ) ;begin
) ;define-library
