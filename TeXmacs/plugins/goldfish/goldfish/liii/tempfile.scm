;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;

(define-library (liii tempfile)
  (export make-temp-file make-temp-dir with-temp-file with-temp-dir)
  (import (scheme base) (scheme file) (liii base) (liii error) (liii os))
  (begin

    ;; 目录不存在/无权限等错误由 C 层按 errno 映射抛出，
    ;; 此处不做 file-exists?/access 预检查（TOCTOU 反模式）
    (define (%check-temp-args caller prefix suffix dir)
      (unless (string? prefix)
        (type-error (string-append caller ": prefix must be a string"))
      ) ;unless
      (unless (string? suffix)
        (type-error (string-append caller ": suffix must be a string"))
      ) ;unless
      (unless (or (not dir) (string? dir))
        (type-error (string-append caller ": dir must be a string or #f"))
      ) ;unless
    ) ;define

    (define* (make-temp-file (prefix "tmp") (suffix "") (dir #f))
      (%check-temp-args "make-temp-file" prefix suffix dir)
      (g_mkstemp prefix suffix (if dir dir (os-temp-dir)))
    ) ;define*

    (define* (make-temp-dir (prefix "tmp") (suffix "") (dir #f))
      (%check-temp-args "make-temp-dir" prefix suffix dir)
      (g_mkdtemp prefix suffix (if dir dir (os-temp-dir)))
    ) ;define*

    (define* (with-temp-file proc (prefix "tmp") (suffix "") (dir #f))
      (unless (procedure? proc)
        (type-error "with-temp-file: proc must be a procedure")
      ) ;unless
      (let*-values (((port path) (make-temp-file prefix suffix dir)))
        (close-output-port port)
        (dynamic-wind (lambda () #f)
          (lambda () (proc path))
          ;; proc 可能已删除该文件，清理失败不应掩盖 body 的异常
          (lambda () (catch #t (lambda () (delete-file path)) (lambda args #f)))
        ) ;dynamic-wind
      ) ;let*-values
    ) ;define*

    (define* (with-temp-dir proc (prefix "tmp") (suffix "") (dir #f))
      (unless (procedure? proc)
        (type-error "with-temp-dir: proc must be a procedure")
      ) ;unless
      (let ((path (make-temp-dir prefix suffix dir)))
        (dynamic-wind (lambda () #f)
          (lambda () (proc path))
          (lambda () (g_rmtree path))
        ) ;dynamic-wind
      ) ;let
    ) ;define*

  ) ;begin
) ;define-library
