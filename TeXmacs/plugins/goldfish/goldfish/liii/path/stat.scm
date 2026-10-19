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

(define-library (liii path stat)
  (export make-path-stat path-stat path-stat? path-stat-size path-stat-mtime
    path-stat-atime path-stat-ctime path-stat-mode path-stat-file?
    path-stat-dir? path-stat-symlink?
  ) ;export
  (import (scheme base) (liii base) (liii bitwise) (liii error))
  (begin

    ;; POSIX st_mode 文件类型位（S_IFMT/S_IFREG/S_IFDIR/S_IFLNK）
    (define %s-ifmt 61440)
    (define %s-ifreg 32768)
    (define %s-ifdir 16384)
    (define %s-iflnk 40960)

    (define-record-type <path-stat>
      (%make-path-stat size mtime atime ctime mode)
      path-stat?
      (size %path-stat-size)
      (mtime %path-stat-mtime)
      (atime %path-stat-atime)
      (ctime %path-stat-ctime)
      (mode %path-stat-mode)
    ) ;define-record-type

    (define (make-path-stat size mtime atime ctime mode)
      (%make-path-stat size mtime atime ctime mode)
    ) ;define

    (define (%check-path-stat who st)
      (unless (path-stat? st)
        (type-error (string-append who ": expected <path-stat> record"))
      ) ;unless
    ) ;define

    (define (path-stat-size st)
      (%check-path-stat "path-stat-size" st)
      (%path-stat-size st)
    ) ;define

    (define (path-stat-mtime st)
      (%check-path-stat "path-stat-mtime" st)
      (%path-stat-mtime st)
    ) ;define

    (define (path-stat-atime st)
      (%check-path-stat "path-stat-atime" st)
      (%path-stat-atime st)
    ) ;define

    (define (path-stat-ctime st)
      (%check-path-stat "path-stat-ctime" st)
      (%path-stat-ctime st)
    ) ;define

    (define (path-stat-mode st)
      (%check-path-stat "path-stat-mode" st)
      (%path-stat-mode st)
    ) ;define

    (define (path-stat p)
      (if (not (string? p))
        (type-error "path-stat: path must be a string")
        (let ((v (g_path-stat p)))
          (make-path-stat (vector-ref v 0)
            (vector-ref v 1)
            (vector-ref v 2)
            (vector-ref v 3)
            (vector-ref v 4)
          ) ;make-path-stat
        ) ;let
      ) ;if
    ) ;define

    (define (%path-stat-type? who st type-bits)
      (%check-path-stat who st)
      (= (logand (%path-stat-mode st) %s-ifmt) type-bits)
    ) ;define

    (define (path-stat-file? st)
      (%path-stat-type? "path-stat-file?" st %s-ifreg)
    ) ;define

    (define (path-stat-dir? st)
      (%path-stat-type? "path-stat-dir?" st %s-ifdir)
    ) ;define

    (define (path-stat-symlink? st)
      (%path-stat-type? "path-stat-symlink?" st %s-iflnk)
    ) ;define

  ) ;begin
) ;define-library
