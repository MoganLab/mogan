;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ghost-api.scm
;; DESCRIPTION : Ghost 自动补全大模型（DeepSeek FIM）接口调用模块
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (liii ghost-api)
  (export ghost-deepseek-complete)
  (import (liii json)
          (liii http))
  (begin
    ;; 直连 DeepSeek FIM 接口（/beta/completions），返回 (tokens . logprobs)
    ;; tokens/logprobs 是等长向量，分别存放生成的 token 及其对数概率。
    ;; 截断/拼接/置信度阈值由调用方（ghost-text.scm）处理。
    (define (ghost-deepseek-complete prefix suffix)
      (let* ((url "https://api.deepseek.com/beta/completions")
             (api-key "sk-f28fc253092841639b32ca1fbb05b4f7")
             (headers `(("Authorization" . ,(string-append "Bearer " api-key))
                        ("Content-Type" . "application/json")))
             (req-data (json->string
                         (list (cons "model" "deepseek-v4-flash")
                               (cons "prompt" prefix)
                               (cons "suffix" suffix)
                               (cons "max_tokens" 16)
                               (cons "stop" #("\n"))
                               (cons "thinking" (list (cons "type" "disabled")))
                               (cons "logprobs" 10)
                               (cons "stream" #f))))
             (r (http-post url :data req-data :headers headers))
             (lp-obj (json-ref (string->json (r 'text)) "choices" 0 "logprobs"))
             (toks (json-ref lp-obj "tokens"))
             (lps (json-ref lp-obj "token_logprobs")))
        (if (and (vector? toks) (vector? lps) (> (vector-length toks) 0))
          (cons toks lps)
          (cons #() #()))))
     ;define
  ) ;begin
) ;define-library
