;
; Copyright (C) 2024 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (liii os)
  (export
    os-arch os-type os-windows? os-linux? os-macos? os-temp-dir
    os-sep pathsep
    os-call ; system
    mkdir chdir rmdir remove rename getenv putenv unsetenv getcwd listdir access getlogin getpid
  ) ;export
  (import (scheme process-context)
          (liii base)
          (liii error)
          (liii string)
  ) ;import
  (begin

    (define (os-arch)
      (g_os-arch)
    ) ;define

    (define (os-type)
      (g_os-type)
    ) ;define

    (define (os-linux?)
      (let ((name (os-type)))
        (and name (string=? name "Linux"))
      ) ;let
    ) ;define

    (define (os-macos?)
      (let ((name (os-type)))
        (and name (string=? name "Darwin"))
      ) ;let
    ) ;define

    (define (os-windows?)
      (let ((name (os-type)))
        (and name (string=? name "Windows"))
      ) ;let
    ) ;define

    (define (os-sep)
      (if (os-windows?)
        #\\
        #\/
      ) ;if
    ) ;define

    (define (pathsep)
      (if (os-windows?)
        #\;
        #\:
      ) ;if
    ) ;define

    (define (%check-dir-andthen path f)
      (cond
        ((not (file-exists? path))
         (file-not-found-error
           (string-append "No such file or directory: '" path "'")
         ) ;file-not-found-error
        ) ;
        ((not (g_isdir path))
         (not-a-directory-error
           (string-append "Not a directory: '" path "'")
         ) ;not-a-directory-error
        ) ;
        (else (f path))
      ) ;cond
    ) ;define

    (define (os-call command)
      (g_os-call command)
    ) ;define

    (define (system command)
      (g_system command)
    ) ;define

    (define (access path mode)
      (cond ((eq? mode 'F_OK) (g_access path 0))
            ((eq? mode 'X_OK) (g_access path 128))
            ((eq? mode 'W_OK) (g_access path 2))
            ((eq? mode 'R_OK) (g_access path 1))
            (else (value-error "Allowed mode 'F_OK, 'X_OK,'W_OK, 'R_OK"))
      ) ;cond
    ) ;define

    (define* (getenv key (default #f))
      (let ((val (get-environment-variable key)))
        (if val
            val
            default
        ) ;if
      ) ;let
    ) ;define*

    (define (putenv key value)
      (if (and (string? key) (string? value))
          (g_setenv key value)
          (type-error "(putenv key value): key and value must be strings")
      ) ;if
    ) ;define

    (define (unsetenv key)
      (g_unsetenv key)
    ) ;define

    (define (os-temp-dir)
      (let ((temp-dir (g_os-temp-dir)))
        (string-remove-suffix temp-dir (string (os-sep)))
      ) ;let
    ) ;define

    (define (mkdir path)
      (if (file-exists? path)
        (file-exists-error (string-append "File exists: '" path "'"))
        (g_mkdir path)
      ) ;if
    ) ;define

    (define (rmdir path)
      (%check-dir-andthen path g_rmdir)
    ) ;define

    (define (remove path)
      (cond
        ((not (string? path))
         (type-error "(remove path): path must be string")
        ) ;
        ((not (file-exists? path))
         (file-not-found-error (string-append "File not found: " path))
        ) ;
        ((g_isdir path)  ; 检查是否为目录
         (value-error "Cannot remove a directory (use 'rmdir' instead)")
        ) ;
        (else
         (g_remove-file path)
        ) ;else
      ) ;cond
    ) ;define

    (define (rename src dst)
      (cond
        ((not (string? src))
         (type-error "(rename src dst): src must be string")
        ) ;
        ((not (string? dst))
         (type-error "(rename src dst): dst must be string")
        ) ;
        ((not (file-exists? src))
         (file-not-found-error (string-append "File not found: " src))
        ) ;
        ((file-exists? dst)
         (file-exists-error (string-append "File exists: " dst))
        ) ;
        (else
         (g_rename src dst)
        ) ;else
      ) ;cond
    ) ;define

    (define (chdir path)
      (if (file-exists? path)
        (g_chdir path)
        (file-not-found-error (string-append "No such file or directory: '" path "'"))
      ) ;if
    ) ;define

    (define (listdir path)
      (%check-dir-andthen path g_listdir)
    ) ;define

    (define (getcwd)
      (g_getcwd)
    ) ;define

    (define (getlogin)
      (if (os-windows?)
          (getenv "USERNAME")
          (g_getlogin)
      ) ;if
    ) ;define

    (define (getpid)
      (g_getpid)
    ) ;define

  ) ;begin
) ;define-library

