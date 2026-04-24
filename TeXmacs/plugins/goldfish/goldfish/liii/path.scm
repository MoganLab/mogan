(define-library (liii path)
  (export path
    path?
    path-copy
    path-dir?
    path-file?
    path-exists?
    path-getsize
    path-read-text
    path-read-bytes
    path-write-text
    path-append-text
    path-touch
    path-root
    path-of-drive
    path-from-parts
    path-from-env
    path-cwd
    path-home
    path-temp-dir
    path-parts
    path-type
    path-drive
    path->string
    path-from-string
    path-name
    path-stem
    path-suffix
    path-equals?
    path=?
    path-absolute?
    path-relative?
    path-join
    path-parent
    path-list
    path-list-path
    path-rmdir
    path-unlink
    path-rename
  ) ;export
  (import (liii base)
    (liii error)
    (liii os)
    (liii string)
    (liii vector)
    (scheme base)
  ) ;import
  (begin

    ;; ; Path record type
    (define-record-type <path>
      (make-path-record parts type drive)
      path?
      (parts path-record-parts
        path-record-set-parts!
      ) ;parts
      (type path-record-type
        path-record-set-type!
      ) ;type
      (drive path-record-drive
        path-record-set-drive!
      ) ;drive
    ) ;define-record-type

    (define (string-split-vec str sep)
      (let loop
        ((chars (string->list str))
         (current '())
         (result '())
        ) ;
        (cond ((null? chars)
               (list->vector (reverse (cons (list->string (reverse current))
                                        result
                                      ) ;cons
                             ) ;reverse
               ) ;list->vector
              ) ;
              ((char=? (car chars) sep)
               (loop (cdr chars)
                 '()
                 (cons (list->string (reverse current))
                   result
                 ) ;cons
               ) ;loop
              ) ;
              (else (loop (cdr chars)
                      (cons (car chars) current)
                      result
                    ) ;loop
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    ;; ; Parse string path into parts
    ;; For absolute paths like "/home/da", the first part is "" to indicate leading /
    ;; On Windows, also handles backslash as separator
    (define (parse-path-string s)
      (cond ((string-null? s) #("."))
            ((string=? s ".") #("."))
            ((string=? s "/") #("/"))
            ((string=? s "\\") #("\\"))
            (else (let ((sep (os-sep)))
                    ;; Normalize path: replace / with \ on Windows, then split
                    (let ((normalized (if (os-windows?)
                                        (string-replace s "/" "\\")
                                        s
                                      ) ;if
                          ) ;normalized
                         ) ;
                      (if (and (> (string-length normalized) 0)
                            (char=? (string-ref normalized 0) sep)
                          ) ;and
                        ;; Absolute path: start with empty string part
                        (let ((parts (string-split-vec normalized sep)
                              ) ;parts
                             ) ;
                          (if (or (vector-empty? parts)
                                (not (string-null? (vector-ref parts 0))
                                ) ;not
                              ) ;or
                            (vector-append #("") parts)
                            parts
                          ) ;if
                        ) ;let
                        ;; Relative path
                        (string-split-vec normalized sep)
                      ) ;if
                    ) ;let
                  ) ;let
            ) ;else
      ) ;cond
    ) ;define

    ;; ; Check if string is a Windows absolute path with drive letter
    (define (windows-path-with-drive? s)
      (and (>= (string-length s) 2)
        (char-alphabetic? (string-ref s 0))
        (char=? (string-ref s 1) #\:)
      ) ;and
    ) ;define

    ;; ; Extract drive letter from Windows path string
    (define (extract-drive s)
      (string (char-upcase (string-ref s 0)))
    ) ;define

    ;; ; Parse Windows path string into parts
    (define (parse-windows-path s)
      (let ((sep (os-sep)))
        (if (and (> (string-length s) 2)
              (or (char=? (string-ref s 2) #\\)
                (char=? (string-ref s 2) #\/)
              ) ;or
            ) ;and
          ;; Absolute Windows path like "C:\Users\..."
          (let* ((rest (substring s 3 (string-length s)))
                 (parts (if (string-null? rest)
                          #()
                          (string-split-vec rest sep)
                        ) ;if
                 ) ;parts
                ) ;
            parts
          ) ;let*
          ;; Relative to drive like "C:file.txt"
          (string-split-vec s sep)
        ) ;if
      ) ;let
    ) ;define

    ;; ; Create a path object
    (define (path . args)
      (if (null? args)
        (make-path-record #(".") 'posix "")
        (let ((arg (car args)))
          (cond ((string? arg)
                 (if (windows-path-with-drive? arg)
                   ;; Windows path with drive letter like "C:\Users"
                   (let ((parts (parse-windows-path arg))
                         (drive (extract-drive arg))
                        ) ;
                     (make-path-record parts 'windows drive)
                   ) ;let
                   ;; Regular path - use platform-specific type
                   (let ((parts (parse-path-string arg))
                         (type (if (os-windows?) 'windows 'posix)
                         ) ;type
                        ) ;
                     (make-path-record parts type "")
                   ) ;let
                 ) ;if
                ) ;
                ((path? arg) (path-copy arg))
                (else (type-error "path: argument must be string or path"
                      ) ;type-error
                ) ;else
          ) ;cond
        ) ;let
      ) ;if
    ) ;define

    ;; ; Copy a path object
    (define (path-copy p)
      (if (path? p)
        (make-path-record (vector-copy (path-record-parts p))
          (path-record-type p)
          (path-record-drive p)
        ) ;make-path-record
        (type-error "path-copy: argument must be path"
        ) ;type-error
      ) ;if
    ) ;define

    ;; ; Get parts as vector
    (define (path-parts p)
      (if (path? p)
        (vector-copy (path-record-parts p))
        (type-error "path-parts: argument must be path"
        ) ;type-error
      ) ;if
    ) ;define

    ;; ; Get type ('posix or 'windows)
    (define (path-type p)
      (if (path? p)
        (path-record-type p)
        (type-error "path-type: argument must be path"
        ) ;type-error
      ) ;if
    ) ;define

    ;; ; Get drive letter (for Windows paths)
    (define (path-drive p)
      (if (path? p)
        (path-record-drive p)
        (type-error "path-drive: argument must be path"
        ) ;type-error
      ) ;if
    ) ;define

    ;; ; Convert path to string
    (define (path->string p)
      (cond ((path? p)
             (let ((parts (path-record-parts p))
                   (type (path-record-type p))
                   (drive (path-record-drive p))
                  ) ;
               (case type
                ((posix)
                 (if (vector-empty? parts)
                   ""
                   (let ((first (vector-ref parts 0)))
                     ;; POSIX type paths always use forward slash
                     (parts->string parts "/")
                   ) ;let
                 ) ;if
                ) ;
                ((windows)
                 (let ((s (parts->string parts "\\")))
                   (if (string-null? drive)
                     s
                     (string-append drive ":\\" s)
                   ) ;if
                 ) ;let
                ) ;
                (else (value-error "path->string: unknown type"
                      ) ;value-error
                ) ;else
               ) ;case
             ) ;let
            ) ;
            ((string? p) p)
            (else (type-error "path->string: argument must be path or string"
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define (path-from-string s)
      (path s)
    ) ;define

    ;; ; Helper: convert parts vector to string
    ;; ; For absolute paths, first part is "" or "/" which should result in leading /
    (define (parts->string parts sep)
      (let ((len (vector-length parts)))
        (if (= len 0)
          ""
          (let ((first (vector-ref parts 0)))
            (cond
              ;; Absolute path indicated by empty first part
              ((string-null? first)
               (if (= len 1)
                 sep
                 (let loop
                   ((i 1) (result ""))
                   (if (>= i len)
                     result
                     (let ((part (vector-ref parts i)))
                       (if (string-null? result)
                         (loop (+ i 1) (string-append sep part))
                         (loop (+ i 1)
                           (string-append result sep part)
                         ) ;loop
                       ) ;if
                     ) ;let
                   ) ;if
                 ) ;let
               ) ;if
              ) ;
              ;; Absolute path indicated by "/" as first part (from path-from-parts)
              ((string=? first "/")
               (if (= len 1)
                 sep
                 ;; Join remaining parts with sep, then prepend /
                 (let loop
                   ((i 1) (result ""))
                   (if (>= i len)
                     (string-append sep result)
                     (let ((part (vector-ref parts i)))
                       (if (string-null? result)
                         (loop (+ i 1) part)
                         (loop (+ i 1)
                           (string-append result sep part)
                         ) ;loop
                       ) ;if
                     ) ;let
                   ) ;if
                 ) ;let
               ) ;if
              ) ;
              ;; Relative path
              (else (let loop
                      ((i 0) (result ""))
                      (if (>= i len)
                        result
                        (let ((part (vector-ref parts i)))
                          (if (string-null? result)
                            (loop (+ i 1) part)
                            (loop (+ i 1)
                              (string-append result sep part)
                            ) ;loop
                          ) ;if
                        ) ;let
                      ) ;if
                    ) ;let
              ) ;else
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; ; Check if two paths are equal
    (define (path-equals? p1 p2)
      (let ((s1 (path->string (path p1)))
            (s2 (path->string (path p2)))
           ) ;
        (string=? s1 s2)
      ) ;let
    ) ;define

    (define path=? path-equals?)

    ;; ; Check if path is absolute
    (define (path-absolute? p)
      (if (path? p)
        (let ((type (path-record-type p))
              (drive (path-record-drive p))
              (parts (path-record-parts p))
             ) ;
          (case type
           ((windows)
            ;; Windows absolute path has a drive letter
            (not (string-null? drive))
           ) ;
           ((posix)
            ;; POSIX absolute path starts with empty part (leading /) or is just "/"
            (and (> (vector-length parts) 0)
              (let ((first (vector-ref parts 0)))
                (or (string-null? first)
                  (string=? first "/")
                ) ;or
              ) ;let
            ) ;and
           ) ;
           (else #f)
          ) ;case
        ) ;let
        (let ((s (path->string p)))
          (cond ((os-windows?)
                 (and (>= (string-length s) 2)
                   (char=? (string-ref s 1) #\:)
                 ) ;and
                ) ;
                (else (and (> (string-length s) 0)
                        (char=? (string-ref s 0) (os-sep))
                      ) ;and
                ) ;else
          ) ;cond
        ) ;let
      ) ;if
    ) ;define

    ;; ; Check if path is relative
    (define (path-relative? p)
      (not (path-absolute? p))
    ) ;define

    ;; ; Get the last component of path (filename)
    (define (path-name p)
      (let ((s (path->string p)))
        ;; Handle special cases: empty string and "." both represent current dir
        (if (or (string-null? s) (string=? s "."))
          ""
          (let ((sep (os-sep)))
            (let loop
              ((i (- (string-length s) 1)))
              (cond ((< i 0) s)
                    ((char=? (string-ref s i) sep)
                     (substring s (+ i 1) (string-length s))
                    ) ;
                    (else (loop (- i 1)))
              ) ;cond
            ) ;let
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; ; Get the stem (filename without extension)
    (define (path-stem p)
      (let ((name (path-name p)))
        (let ((splits (string-split name #\.)))
          (let ((count (length splits)))
            (cond ((<= count 1) name)
                  ((string=? name ".") "")
                  ((string=? name "..") "..")
                  ((and (string=? (car splits) "")
                     (= count 2)
                   ) ;and
                   name
                  ) ;
                  (else
                    ;; Take all parts except the last one and join with "."
                    (let loop
                      ((i 0) (result ""))
                      (if (>= i (- count 1))
                        result
                        (let ((part (list-ref splits i)))
                          (if (string-null? result)
                            (loop (+ i 1) part)
                            (loop (+ i 1)
                              (string-append result "." part)
                            ) ;loop
                          ) ;if
                        ) ;let
                      ) ;if
                    ) ;let
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    ;; ; Get the suffix (file extension)
    (define (path-suffix p)
      (let ((name (path-name p)))
        (let ((splits (string-split name #\.)))
          (let ((count (length splits)))
            (cond ((<= count 1) "")
                  ((string=? name ".") "")
                  ((string=? name "..") "")
                  ((and (string=? (car splits) "")
                     (= count 2)
                   ) ;and
                   ""
                  ) ;
                  (else (string-append "."
                          (list-ref splits (- count 1))
                        ) ;string-append
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    ;; ; Join paths
    (define (path-join base . segments)
      (let ((sep (string (os-sep))))
        (let loop
          ((result (path->string base))
           (rest segments)
          ) ;
          (if (null? rest)
            result
            (let ((part (path->string (car rest))))
              (if (or (string-null? result)
                    (string-ends? result sep)
                  ) ;or
                (loop (string-append result part)
                  (cdr rest)
                ) ;loop
                (loop (string-append result sep part)
                  (cdr rest)
                ) ;loop
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    ;; ; Get parent directory
    (define (path-parent p)
      (let ((s (path->string p)))
        (let ((sep (os-sep)))
          ;; First, remove trailing separator if present (except for root)
          (let ((s-trimmed (if (and (> (string-length s) 1)
                                 (char=? (string-ref s (- (string-length s) 1))
                                   sep
                                 ) ;char=?
                               ) ;and
                             (substring s 0 (- (string-length s) 1))
                             s
                           ) ;if
                ) ;s-trimmed
               ) ;
            (let loop
              ((i (- (string-length s-trimmed) 1)))
              (cond ((< i 0)
                     (if (os-windows?) (path "") (path "."))
                    ) ;
                    ((char=? (string-ref s-trimmed i) sep)
                     (if (= i 0)
                       (path-root)
                       ;; Keep the trailing separator for the parent path
                       (path (substring s-trimmed 0 (+ i 1)))
                     ) ;if
                    ) ;
                    (else (loop (- i 1)))
              ) ;cond
            ) ;let
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    ;; ; Path predicates and operations (work with strings or paths)
    (define (path-dir? p)
      (g_isdir (path->string p))
    ) ;define

    (define (path-file? p)
      (g_isfile (path->string p))
    ) ;define

    (define (path-exists? p)
      (file-exists? (path->string p))
    ) ;define

    (define (path-getsize p)
      (let ((s (path->string p)))
        (if (not (file-exists? s))
          (file-not-found-error (string-append "No such file or directory: '"
                                  s
                                  "'"
                                ) ;string-append
          ) ;file-not-found-error
          (g_path-getsize s)
        ) ;if
      ) ;let
    ) ;define

    (define (path-read-text p)
      (let ((s (path->string p)))
        (if (not (file-exists? s))
          (file-not-found-error (string-append "No such file or directory: '"
                                  s
                                  "'"
                                ) ;string-append
          ) ;file-not-found-error
          (g_path-read-text s)
        ) ;if
      ) ;let
    ) ;define

    (define (path-read-bytes p)
      (let ((s (path->string p)))
        (if (not (file-exists? s))
          (file-not-found-error (string-append "No such file or directory: '"
                                  s
                                  "'"
                                ) ;string-append
          ) ;file-not-found-error
          (g_path-read-bytes s)
        ) ;if
      ) ;let
    ) ;define

    (define (path-write-text p content)
      (if (not (string? content))
        (type-error "path-write-text: content must be string"
        ) ;type-error
        (g_path-write-text (path->string p)
          content
        ) ;g_path-write-text
      ) ;if
    ) ;define

    (define (path-append-text p content)
      (g_path-append-text (path->string p)
        content
      ) ;g_path-append-text
    ) ;define

    (define (path-touch p)
      (g_path-touch (path->string p))
    ) ;define

    ;; ; Static path constructors
    (define (path-root)
      (make-path-record #("/") 'posix "")
    ) ;define

    (define (path-of-drive ch)
      (if (char? ch)
        (make-path-record #()
          'windows
          (string (char-upcase ch))
        ) ;make-path-record
        (type-error "path-of-drive: argument must be char"
        ) ;type-error
      ) ;if
    ) ;define

    (define (path-from-parts parts)
      (if (vector? parts)
        (if (and (> (vector-length parts) 0)
              (string? (vector-ref parts 0))
              (windows-path-with-drive? (vector-ref parts 0)
              ) ;windows-path-with-drive?
            ) ;and
          ;; Windows path with drive letter like "C:"
          (let* ((drive-str (vector-ref parts 0))
                 (drive (extract-drive drive-str))
                 ;; Build result parts without drive part
                 (clean-parts (let loop
                                ((i 1) (result '()))
                                (if (>= i (vector-length parts))
                                  (list->vector (reverse result))
                                  (let ((part (vector-ref parts i)))
                                    ;; Skip empty parts and separator parts
                                    (if (or (string-null? part)
                                          (string=? part "/")
                                          (string=? part "\\")
                                        ) ;or
                                      (loop (+ i 1) result)
                                      (loop (+ i 1) (cons part result))
                                    ) ;if
                                  ) ;let
                                ) ;if
                              ) ;let
                 ) ;clean-parts
                ) ;
            (make-path-record clean-parts
              'windows
              drive
            ) ;make-path-record
          ) ;let*
          ;; Regular POSIX-style path
          (make-path-record (vector-copy parts)
            'posix
            ""
          ) ;make-path-record
        ) ;if
        (type-error "path-from-parts: argument must be vector"
        ) ;type-error
      ) ;if
    ) ;define

    (define (path-from-env name)
      (path (getenv name))
    ) ;define

    (define (path-cwd)
      (path (getcwd))
    ) ;define

    (define (path-home)
      (cond ((or (os-linux?) (os-macos?))
             (path (getenv "HOME"))
            ) ;
            ((os-windows?)
             (path (string-append (getenv "HOMEDRIVE")
                     (getenv "HOMEPATH")
                   ) ;string-append
             ) ;path
            ) ;
            (else (value-error "path-home: unknown platform"
                  ) ;value-error
            ) ;else
      ) ;cond
    ) ;define

    (define (path-temp-dir)
      (path (os-temp-dir))
    ) ;define

    ;; ; List directory contents
    (define (path-list p)
      (listdir (path->string p))
    ) ;define

    ;; ; List directory contents as path objects
    (define (path-list-path p)
      (let ((base (path->string p)))
        (let ((entries (listdir base)))
          (vector-map (lambda (entry) (path-join base entry))
            entries
          ) ;vector-map
        ) ;let
      ) ;let
    ) ;define

    ;; ; Remove directory
    (define (path-rmdir p)
      (rmdir (path->string p))
    ) ;define

    ;; ; Remove file
    (define* (path-unlink p (missing-ok #f))
      (let ((s (path->string p)))
        (cond ((file-exists? s) (remove s))
              (missing-ok #t)
              (else (error 'file-not-found-error
                      (string-append "File not found: " s)
                    ) ;error
              ) ;else
        ) ;cond
      ) ;let
    ) ;define*

    ;; ; Rename file or directory
    (define (path-rename src dst)
      (rename (path->string src)
        (path->string dst)
      ) ;rename
    ) ;define

  ) ;begin
) ;define-library
