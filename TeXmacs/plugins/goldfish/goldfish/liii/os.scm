(define-library (liii os)
  (export os-arch
    os-type
    os-windows?
    os-linux?
    os-macos?
    os-temp-dir
    os-sep
    pathsep
    os-call
    mkdir
    chdir
    rmdir
    remove
    rename
    getenv
    putenv
    unsetenv
    getcwd
    listdir
    access
    getlogin
    getpid
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
      (if (os-windows?) #\\ #\/)
    ) ;define

    (define (pathsep)
      (if (os-windows?) #\; #\:)
    ) ;define

    (define (%check-dir-andthen path f)
      (cond ((not (file-exists? path))
             (file-not-found-error (string-append "No such file or directory: '"
                                     path
                                     "'"
                                   ) ;string-append
             ) ;file-not-found-error
            ) ;
            ((not (g_isdir path))
             (not-a-directory-error (string-append "Not a directory: '"
                                      path
                                      "'"
                                    ) ;string-append
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
            (else (value-error "Allowed mode 'F_OK, 'X_OK,'W_OK, 'R_OK"
                  ) ;value-error
            ) ;else
      ) ;cond
    ) ;define

    (define* (getenv key (default #f))
      (let ((val (get-environment-variable key)))
        (if val val default)
      ) ;let
    ) ;define*

    (define (putenv key value)
      (if (and (string? key) (string? value))
        (g_setenv key value)
        (type-error "(putenv key value): key and value must be strings"
        ) ;type-error
      ) ;if
    ) ;define

    (define (unsetenv key)
      (g_unsetenv key)
    ) ;define

    (define (os-temp-dir)
      (let ((temp-dir (g_os-temp-dir)))
        (string-remove-suffix temp-dir
          (string (os-sep))
        ) ;string-remove-suffix
      ) ;let
    ) ;define

    (define (mkdir path)
      (if (file-exists? path)
        (file-exists-error (string-append "File exists: '"
                             path
                             "'"
                           ) ;string-append
        ) ;file-exists-error
        (g_mkdir path)
      ) ;if
    ) ;define

    (define (rmdir path)
      (%check-dir-andthen path g_rmdir)
    ) ;define

    (define (remove path)
      (cond ((not (string? path))
             (type-error "(remove path): path must be string"
             ) ;type-error
            ) ;
            ((not (file-exists? path))
             (file-not-found-error (string-append "File not found: " path)
             ) ;file-not-found-error
            ) ;
            ((g_isdir path)
             (value-error "Cannot remove a directory (use 'rmdir' instead)"
             ) ;value-error
            ) ;
            (else (g_remove-file path))
      ) ;cond
    ) ;define

    (define (rename src dst)
      (cond ((not (string? src))
             (type-error "(rename src dst): src must be string"
             ) ;type-error
            ) ;
            ((not (string? dst))
             (type-error "(rename src dst): dst must be string"
             ) ;type-error
            ) ;
            ((not (file-exists? src))
             (file-not-found-error (string-append "File not found: " src)
             ) ;file-not-found-error
            ) ;
            ((file-exists? dst)
             (file-exists-error (string-append "File exists: " dst)
             ) ;file-exists-error
            ) ;
            (else (g_rename src dst))
      ) ;cond
    ) ;define

    (define (chdir path)
      (if (file-exists? path)
        (g_chdir path)
        (file-not-found-error (string-append "No such file or directory: '"
                                path
                                "'"
                              ) ;string-append
        ) ;file-not-found-error
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
