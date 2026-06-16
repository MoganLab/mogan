
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-autosave.scm
;; DESCRIPTION : Autosave plugin entry for goldfish
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
;; ; tm-autosave.scm 与 liii/ 同目录,把当前文件所在目录加入 load-path
(import (liii base) (liii path))
(set! *load-path*
  (cons (path->string (path-parent (port-filename))) *load-path*)
) ;set!

(import (texmacs protocol))
(import (liii autosave))
(import (liii json))
(import (liii path))
(import (liii os))

;; 插件进程与主进程的握手:启动时必须 flush 插件名 "autosave"，
;; 否则主进程不会进入 DATA_COMMAND 状态、无法投递后续负载。

(define (welcome)
  (flush-verbatim "autosave")
) ;define

(define (autosave-log message)
  (path-append-text "/tmp/debug.log" (string-append message "\n"))
) ;define

(define (handle-copy payload)
  (catch #t
    (lambda ()
      (let* ((json (string->json payload))
             (source (json-ref json "path"))
             (doc-id (json-ref json "id"))
             (target (if (string? doc-id) (autosave-target-path doc-id) ""))
            ) ;
        (if (or (not (string? source))
              (not (string? doc-id))
              (string=? source "")
              (string=? doc-id "")
            ) ;or
          (autosave-log "autosave copy skipped: missing source/id")
          (if (not (file-exists? source))
            (autosave-log (string-append "autosave copy skipped: source missing " source))
            (when (ensure-parent-dir doc-id)
              (autosave-prune-dir (autosave-dir doc-id))
              (if (path-copy source target)
                (autosave-log (string-append "autosave copied " source " -> " target))
                (autosave-log (string-append "autosave copy failed " source " -> " target))
              ) ;if
            ) ;when
          ) ;if
        ) ;if
      ) ;let*
    ) ;lambda
    (lambda args
      (autosave-log (string-append "autosave copy exception " (object->string args)))
    ) ;lambda
  ) ;catch
) ;define

(define (read-eval-print)
  (let ((code (read-paragraph-by-visible-eof)))
    (let ((payload (document->string code)))
      (if (string=? payload "")
        #t
        (begin
          (handle-copy payload)
          (flush-verbatim payload)
        ) ;begin
      ) ;if
    ) ;let
  ) ;let
) ;define

(welcome)
(read-eval-print)
