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

(define-library (liii subprocess process-error)
  (export make-process-error process-error? process-error-code
    process-error-stderr process-error-stdout process-error-command
    process-error-message
  ) ;export
  (import (scheme base) (liii error) (liii string))
  (begin

    (define-record-type <process-error>
      (make-process-error code stderr stdout command)
      process-error?
      (code process-error-code)
      (stderr process-error-stderr)
      (stdout process-error-stdout)
      (command process-error-command)
    ) ;define-record-type

    (define (process-error-message err)
      (unless (process-error? err)
        (type-error "process-error-message: expected a <process-error> record")
      ) ;unless
      (let ((stderr (process-error-stderr err)))
        (if (and (string? stderr) (not (string-null? stderr)))
          stderr
          (let ((stdout (process-error-stdout err)))
            (if (and (string? stdout) (not (string-null? stdout)))
              stdout
              (string-append "Command failed with exit code "
                (number->string (process-error-code err))
              ) ;string-append
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
