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

(define-library (liii rich-path)
  (export
    path-dir? path-file? path-exists?
    path-getsize path-read-text path-read-bytes path-write-text path-append-text path-touch
    path
  ) ;export
  (import (liii base) (liii lang) (liii error) (liii vector) (liii string) (liii list)
          (liii os)
  ) ;import
  (begin

    (define (path-dir? path)
      (g_isdir path)
    ) ;define

    (define (path-file? path)
      (g_isfile path)
    ) ;define

    (define (path-exists? path)
      (file-exists? path)
    ) ;define

    (define path-getsize
      (typed-lambda ((path string?))
        (if (not (file-exists? path))
          (file-not-found-error
            (string-append "No such file or directory: '" path "'")
          ) ;file-not-found-error
          (g_path-getsize path)
        ) ;if
      ) ;typed-lambda
    ) ;define

    (define path-read-text
      (typed-lambda ((path string?))
        (if (not (file-exists? path))
          (file-not-found-error
            (string-append "No such file or directory: '" path "'")
          ) ;file-not-found-error
          (g_path-read-text path)
        ) ;if
      ) ;typed-lambda
    ) ;define

    (define path-read-bytes
      (typed-lambda ((path string?))
        (if (not (file-exists? path))
          (file-not-found-error
            (string-append "No such file or directory: '" path "'")
          ) ;file-not-found-error
          (g_path-read-bytes path)
        ) ;if
      ) ;typed-lambda
    ) ;define

    (define path-write-text
      (typed-lambda ((path string?) (content string?))
        (g_path-write-text path content)
      ) ;typed-lambda
    ) ;define

    (define path-append-text
      (typed-lambda ((path string?) (content string?))
        (g_path-append-text path content)
      ) ;typed-lambda
    ) ;define

    (define (path-touch path)
      (g_path-touch path)
    ) ;define

    (define-case-class path ()
      (define parts #("."))
      (define type 'posix)
      (define drive "")
  
      (define (%set-parts! v)
        (if (rich-vector :is-type-of v)
            (set! parts (v :collect))
            (set! parts v)
        ) ;if
      ) ;define
  
      (define (%set-type! s)
        (set! type s)
      ) ;define
  
      (define (%set-drive! s)
        (set! drive s)
      ) ;define
  
      (define (%get-parts) parts)
      (define (%get-type) type)
      (define (%get-drive) drive)
  
      (define (%copy)
        (let ((p (path)))
          (p :set-parts! parts)
          (p :set-type! type)
          (p :set-drive! drive)
          p
        ) ;let
      ) ;define


      (chained-define (@of-drive ch)
        (when (not (char? ch))
          (type-error "path@of-drive must take char? as input")
        ) ;when
        (let ((r (path)))
          (r :set-type! 'windows)
          (r :set-drive! ($ ch :to-upper :make-string))
          (r :set-parts! #())
          r
        ) ;let
      ) ;chained-define

      (chained-define (@root)
        (let ((r (path)))
              (r :set-parts! #("/"))
              r
        ) ;let
      ) ;chained-define

      (chained-define (@from-parts x)
        (let ((r (path)))
          (r :set-parts! x)
          r
        ) ;let
      ) ;chained-define

      (chained-define (@/ x) 
        (if (path :is-type-of x)
            (path :root :/ x)
            (cond ((and (string-ends? x ":") (= (string-length x) 2))
                   (path :of-drive (x 0)))
            
                  ((string=? x "/") (path :root))
            
                  (else
                    (path :from-parts (vector-append (vector (string (os-sep))) (vector x)))
                  ) ;else
            ) ;cond
        ) ;if
      ) ;chained-define

      (chained-define (@apply s)
        (cond ((and (or (os-linux?) (os-macos?))
                    (string-starts? s "/"))
               (path :/ (@apply ($ s :drop 1 :get))))
              ((and (os-windows?)
                    (= (string-length s) 2)
                    (char=? (s 1) #\:))
               (path :of-drive (s 0))
              ) ;
              ((and (os-windows?) (>= (string-length s) 3)
                    (char=? (s 1) #\:)
                    (char=? (s 2) #\\))
               (path :of-drive (s 0)
                     :/ (@apply ($ s :drop 3 :get))
               ) ;path
              ) ;
              (else
               (let loop ((iter s))
                 (cond ((or (string-null? iter) (string=? iter "."))
                        (path))
                 
                       ((not (char=? (iter 0) (os-sep)))
                        (path :from-parts ($ iter :split (string (os-sep))))
                       ) ;
                 
                       (else
                        (loop ($ iter :drop 1 :get))
                       ) ;else
                 ) ;cond
               ) ;let
              ) ;else
        ) ;cond
      ) ;chained-define

      (chained-define (@from-env name)
        (path (getenv name))
      ) ;chained-define

      (define (%name)
        (if (string=? "." ($ parts :last))
            ""
            ($ parts :last)
        ) ;if
      ) ;define

      (define (%stem)
        (define last-part-str 
          (if (> (vector-length parts) 0)
              (vector-ref parts (- (vector-length parts) 1))
              ""
          ) ;if
        ) ;define
  
        (define (drop-suffix str)
          (let* ((rich-str ($ str))
                 (rich-splits (rich-str :split "."))  ; 按点分割
                 (count (rich-splits :count)))  ; 获取分割数量
            (cond ((<= count 1) str)  ; 无后缀或单一部分
                  ((string=? str ".") "")  ; 当前目录特殊处理
                  ((string=? str "..") "..") ; 上级目录特殊处理
                  ((and (string=? (rich-splits 0) "")  ; 以点开头
                        (= count 2))  ; 且只有一个点（纯隐藏文件）
                   str  ; 保留完整文件名
                  ) ;
                  (else  ; 正常多后缀情况
                   (rich-splits :take (- count 1) :make-string ".")
                  ) ;else
            ) ;cond
          ) ;let*
        ) ;define
  
        (drop-suffix (%name))
      ) ;define

      (define (%suffix)
        (let* ((name (%name))
               (rich-str ($ name))
               (rich-splits (rich-str :split "."))
               (count (rich-splits :count)))
          (cond ((<= count 1) "")  ; 无后缀
                ((string=? name ".") "")  ; 当前目录
                ((string=? name "..") "") ; 上级目录
                ((and (string=? (rich-splits 0) "")  ; 以点开头
                      (= count 2))  ; 且只有一个点（纯隐藏文件）
                 ""
                ) ;
                (else 
                 (string-append "." (rich-splits :last))  ; 返回最后一部分
                ) ;else
          ) ;cond
        ) ;let*
      ) ;define

      (define (%equals that)
        (if (path :is-type-of that)
            (string=? (%to-string) (that :to-string))
            #f
        ) ;if
      ) ;define

      (define (%file?)
        (path-file? (%to-string))
      ) ;define

      (define (%dir?)
        (path-dir? (%to-string)) 
      ) ;define

      (define (%absolute?)
        (case type
          ((posix) (string-starts? (parts 0) "/"))
    
          ((windows) (not ($ drive :empty?)))
    
          (else
            (value-error
              (string-append "path%absolute?: unknown type" (symbol->string type))
            ) ;value-error
          ) ;else
        ) ;case
      ) ;define

      (define (%relative)
        (not (%absolute?))
      ) ;define

      (define (%exists?)
        (path-exists? (%to-string))
      ) ;define

      (define (%to-string)
        (case type
          ((posix)
           (let ((s ($ parts :make-string (string (os-sep)))))
             (if (and (> ($ s :length) 1) (string-starts? s (string (os-sep))))
                 (string-drop s 1)
                 s
             ) ;if
           ) ;let
          ) ;
          ((windows)
           (let ((s ($ parts :make-string "\\")))
             (if (string-null? drive)
                 s
                 (string-append drive ":\\" s)
             ) ;if
           ) ;let
          ) ;
          (else (value-error "path%to-string: unknown type" type))
        ) ;case
      ) ;define

      (define (%read-text)
        (path-read-text (%to-string))
      ) ;define

      (typed-define (%write-text (content string?))
        (path-write-text (%to-string) content)
      ) ;typed-define

      (typed-define (%append-text (content string?))
        (path-append-text (%to-string) content)
      ) ;typed-define

      (define (%list)
        (box (listdir (%to-string)))
      ) ;define

      (define (%list-path)
        ((box (listdir (%to-string)))
         :map (lambda (x) ((%this) :/ x))
        ) ;
      ) ;define

      (define (%touch)
        (path-touch (%to-string))
      ) ;define

      (chained-define (%/ x)
        (cond ((string? x)
               (let ((new-path (%copy)))
                 (new-path :set-parts! (vector-append parts (vector x)))
                 new-path)
               ) ;let
        
              ((path :is-type-of x)
               (cond ((x :absolute?)
                      (value-error "path to append must not be absolute path: " (x :to-string)))
                     ((string=? (x :to-string) ".")
                      (%this)
                     ) ;
                     (else (let ((new-path (%copy))
                                 (x-parts (x :get-parts)))
                             (if (os-windows?)
                                 (new-path :set-parts! x-parts)
                                 (new-path :set-parts! (vector-append (vector (string (os-sep))) x-parts))
                             ) ;if
                             new-path)
                     ) ;else
               ) ;cond
              ) ;
        
              (else (type-error "only string?, path is allowed"))
        ) ;cond
      ) ;chained-define

      (chained-define (%parent)
        (define (parts-drop-right parts x)
          (let ((path-vec ($ parts :drop-right x)))
            (let ((new-path (%copy)))
              (if (path-vec :empty?)
                  (if (os-windows?)
                      (new-path :set-parts! #(""))
                      (new-path :set-parts! #("."))
                  ) ;if
                  (new-path :set-parts! (path-vec :append #("")))
              ) ;if
              new-path
            ) ;let
          ) ;let
        ) ;define
                
        (cond
          ((or (equal? #("/") parts) (equal? #(".") parts))
           (%this)
          ) ;
          ((or (os-macos?) (os-linux?))
           (let ((last-part (($ parts) :take-right 1 :collect)))
                 (if (equal? last-part #(""))
                     (parts-drop-right parts 2)
                     (parts-drop-right parts 1)
                 ) ;if
           ) ;let
          ) ;
          ((os-windows?)
           (if ($ parts :empty?)
               (%this)
               (let ((last-part (($ parts) :take-right 1 :collect)))
                 (if (equal? last-part #(""))
                     (parts-drop-right parts 2)
                     (parts-drop-right parts 1)
                 ) ;if
               ) ;let
           ) ;if
          ) ;
    
          (else (??? "Unsupported platform"))
        ) ;cond
      ) ;chained-define

      (define (%rmdir)
        (rmdir (%to-string))
      ) ;define

      (define* (%unlink (missing-ok #f))  ; 使用define*定义可选参数
        (let ((path-str (%to-string)))
          (cond
            ((file-exists? path-str)  ; 文件存在时总是删除
             (remove path-str)
            ) ;
            (missing-ok  ; 文件不存在时根据missing-ok决定
             #t         ; missing-ok为#t时静默返回#t
            ) ;missing-ok
            (else        ; missing-ok为#f时抛出错误
             (error 'file-not-found-error 
                    (string-append "File not found: " path-str)
             ) ;error
            ) ;else
          ) ;cond
        ) ;let
      ) ;define*


      (chained-define (@./ x)
        (let ((p (path x)))
              (if (p :absolute?)
                  (value-error "path@./: only accecpt relative path")
                  (path x)
              ) ;if
        ) ;let
      ) ;chained-define

      (chained-define (@cwd)
        (path (getcwd))
      ) ;chained-define

      (chained-define (@home)
        (cond ((or (os-linux?) (os-macos?))
               (path (getenv "HOME")))
              ((os-windows?)
               (path :of-drive ((getenv "HOMEDRIVE") 0)
                     :/ (path (getenv "HOMEPATH"))
               ) ;path
              ) ;
              (else (value-error "path@home: unknown type"))
        ) ;cond
      ) ;chained-define

      (chained-define (@temp-dir)
        (path (os-temp-dir)))

      ) ;chained-define

    ) ;define-case-class
  ) ;begin
