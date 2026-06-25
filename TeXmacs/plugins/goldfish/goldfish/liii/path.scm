(define-library (liii path)
  (export path
    path?
    path-copy
    path-copy-into
    path-dir?
    path-file?
    path-exists?
    path-getsize
    path-read-text
    path-read-bytes
    path-write-text
    path-write-bytes
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
    path-suffixes
    path-with-name
    path-with-stem
    path-with-suffix
    path-relative-to
    path-starts-with?
    path-equals?
    path=?
    path-absolute?
    path-relative?
    path-join
    path-parent
    path-parents
    path-list
    path-list-path
    path-rmdir
    path-unlink
    path-rename
    path-mkdir
    path-absolute
    path-expanduser
    path-match
    path-as-posix
    path-resolve
  ) ;export
  (import (liii base)
    (liii error)
    (liii os)
    (liii string)
    (liii vector)
    (scheme base)
    (scheme char)
    (liii ascii)
  ) ;import
  (begin

    ;; ;============================================================
    ;; ; Path record 类型
    ;; ;============================================================
    ;; ; root 字段：#\\ / #\/ 表示有根分隔符(绝对路径或驱动器根),
    ;; ; #f 表示无根(drive-relative 或相对路径)。用于区分 C:\foo
    ;; ; (root=#\\)和 C:foo(root=#f)的语义差异。
    (define-record-type <path>
      (make-path-record parts type drive root)
      path?
      (parts path-record-parts path-record-set-parts!)
      (type path-record-type path-record-set-type!)
      (drive path-record-drive path-record-set-drive!)
      (root path-record-root path-record-set-root!)
    ) ;define-record-type

    ;; ;============================================================
    ;; ; 辅助函数区
    ;; ;============================================================

    ;; ; 字符串按 sep 切分为 vector(保留空段)。Windows/posix 通用。
    (define (string-split-vec str sep)
      (list->vector (string-split str sep))
    ) ;define

    ;; ; 判断字符串是否以 UNC 前缀(\\)开头。仅 Windows 路径用,posix 永远 #f。
    (define (unc-prefix? s)
      (and (>= (string-length s) 2)
        (char=? (string-ref s 0) #\\)
        (char=? (string-ref s 1) #\\)
      ) ;and
    ) ;define

    ;; ; 判断字符串是否为带盘符的 Windows 路径(如 "C:" 开头)。posix 永远 #f。
    (define (windows-path-with-drive? s)
      (and (>= (string-length s) 2)
        (char-alphabetic? (string-ref s 0))
        (char=? (string-ref s 1) #\:)
      ) ;and
    ) ;define

    ;; ; 提取 Windows 路径字符串的盘符字母(大写,不含冒号)。仅 Windows 路径用。
    (define (extract-drive s)
      (string (ascii-upcase (string-ref s 0)))
    ) ;define

    ;; ; 过滤掉 "." 段和空段。pathlib 风格:构造时丢弃 "." 与连续/尾分隔符
    ;; ; 产生的空段,保留 ".." 直到 resolve() 才处理。使 /tmp/ → /tmp、
    ;; ; a//b → a/b 与 pathlib 一致。
    (define (drop-dot-parts v)
      (vector-filter (lambda (p) (not (or (string=? p ".") (string-null? p)))) v)
    ) ;define

    ;; ; 路径字符串解析为 (values parts root),适用于 posix 与 Windows
    ;; ; 普通路径(无 UNC、无盘符)。
    ;; ; 差异:Windows 平台下先把 / 规范化为 \,再按当前平台 sep 切分;
    ;; ; posix 平台直接按 / 切分。起始分隔符由 root 字段表达,不混入 parts。
    (define (parse-path-string s)
      (cond ((string-null? s) (values #(".") #f))
            ((string=? s ".") (values #(".") #f))
            ((string=? s "/") (values #() #\/))
            ((string=? s "\\") (values #() #\\))
            (else (let ((sep (os-sep)))
                    (let ((normalized (if (os-windows?) (string-replace s "/" "\\") s)))
                      (if (and (> (string-length normalized) 0) (char=? (string-ref normalized 0) sep))
                        ;; 绝对路径:丢弃 string-split-vec 产生的起始空 stub。
                        (let ((raw (string-split-vec normalized sep)))
                          (values (drop-dot-parts (vector-drop raw 1)) sep)
                        ) ;let
                        ;; 相对路径
                        (values (drop-dot-parts (string-split-vec normalized sep)) #f)
                      ) ;if
                    ) ;let
                  ) ;let
            ) ;else
      ) ;cond
    ) ;define

    ;; ; 解析 UNC 路径 \\server[\share][\path...] 为 (values parts drive root)。
    ;; ; 仅 Windows 用。差异:仅 \\server(无 share)时 root=#f(对齐 pathlib);
    ;; ; 含 share 时 root=#\\。
    (define (parse-unc normalized len)
      (let* ((after-slash (substring normalized 2 len))
             (first-slash (string-index after-slash #\\))
            ) ;
        (if (not first-slash)
          ;; 仅 \\server（无 share）：drive 是 \\\\server 整体，但 root 为 #f
          ;; (对齐 pathlib: PureWindowsPath('\\\\srv').root == '')
          (values #() (string-append "\\\\" after-slash) #f)
          (let* ((server (substring after-slash 0 first-slash))
                 (rest (substring after-slash (+ first-slash 1) (string-length after-slash)))
                 (second-slash (string-index rest #\\))
                ) ;
            (if (not second-slash)
              ;; \\server\share（无后续路径）
              (values #() (string-append "\\\\" server "\\" rest) #\\)
              ;; \\server\share\path...
              (let* ((share (substring rest 0 second-slash))
                     (path-rest (substring rest (+ second-slash 1) (string-length rest)))
                     (parts (if (string-null? path-rest)
                              #()
                              (drop-dot-parts (string-split-vec path-rest #\\))
                            ) ;if
                     ) ;parts
                    ) ;
                (values parts (string-append "\\\\" server "\\" share) #\\)
              ) ;let*
            ) ;if
          ) ;let*
        ) ;if
      ) ;let*
    ) ;define

    ;; ; Windows 路径字符串解析为 (values parts drive root)。仅 Windows 用。
    ;; ; 同时识别 \ 和 / 作为分隔符(Windows API 两种都接受)。涵盖四种形式:
    ;; ;   \\server\share\a\b → drive="\\server\share" root=#\\ parts=#("a" "b")
    ;; ;   C:\a\b             → drive="C"              root=#\\ parts=#("a" "b")
    ;; ;   C:foo              → drive="C"              root=#f  parts=#("foo")
    ;; ;   \foo               → drive=""               root=#\\ parts=#("foo")
    ;; ;   foo\bar            → drive=""               root=#f  parts=#("foo" "bar")
    (define (parse-windows-path s)
      (let* ((normalized (string-replace s "/" "\\")) (len (string-length normalized)))
        (cond
          ;; UNC 路径: \\server\share[\path...]
          ((unc-prefix? normalized) (parse-unc normalized len))

          ;; 盘符绝对路径: C:\...
          ((and (>= len 3)
             (char-alphabetic? (string-ref normalized 0))
             (char=? (string-ref normalized 1) #\:)
             (char=? (string-ref normalized 2) #\\)
           ) ;and
           (let* ((drive (extract-drive normalized))
                  (rest (substring normalized 3 len))
                  (parts (if (string-null? rest) #() (drop-dot-parts (string-split-vec rest #\\)))
                  ) ;parts
                 ) ;
             (values parts drive #\\)
           ) ;let*
          ) ;

          ;; 盘符相对路径: C:foo
          ((windows-path-with-drive? normalized)
           (let* ((drive (extract-drive normalized))
                  (rest (substring normalized 2 len))
                  (parts (if (string-null? rest) #() (drop-dot-parts (string-split-vec rest #\\)))
                  ) ;parts
                 ) ;
             (values parts drive #f)
           ) ;let*
          ) ;

          ;; 当前盘根路径: \foo
          ((and (> len 0) (char=? (string-ref normalized 0) #\\))
           (let* ((rest (substring normalized 1 len))
                  (parts (if (string-null? rest) #() (drop-dot-parts (string-split-vec rest #\\)))
                  ) ;parts
                 ) ;
             (values parts "" #\\)
           ) ;let*
          ) ;

          ;; 相对路径: foo\bar
          (else (values (drop-dot-parts (string-split-vec normalized #\\)) "" #f))
        ) ;cond
      ) ;let*
    ) ;define

    ;; ; 构造路径 anchor 首元素字符串(对齐 pathlib.parts 的首元素)。
    ;; ; 差异:posix 绝对返回 "/";Windows 返回 "C:\"/"C:"/"\\server\share\"/"\\";
    ;; ; 纯相对路径(无 drive 无 root)返回 #f。
    (define (anchor-string type drive root)
      (cond ((eq? type 'posix) (and root "/"))
            ((unc-prefix? drive) (string-append drive "\\"))
            ((not (string-null? drive)) (string-append drive ":" (if root "\\" "")))
            (root "\\")
            (else #f)
      ) ;cond
    ) ;define

    ;; ; 将纯净 parts 段用 sep 拼接成字符串(不含 drive/anchor/stub)。
    ;; ; 调用方根据路径类型传 "/"(posix)或 "\\"(Windows)。
    (define (parts->string parts sep)
      (string-join (vector->list parts) sep)
    ) ;define

    ;; ; 解析末段的点分隔结构,返回 (values stem-list suffix-list)。
    ;; ; 隐藏文件(.bashrc)、无点、"."/".." 整体作 stem 无后缀。
    ;; ; 末尾点(foo.)当作空后缀:stem="foo", suffix=".", 对齐 Python 3.14+ pathlib。
    (define (split-name-dots name)
      (cond ((or (string=? name ".") (string=? name "..")) (values (list name) '()))
            (else (let ((splits (string-split name #\.)))
                    (if (or (<= (length splits) 1) (string=? (car splits) ""))
                      (values (list name) '())
                      (let* ((rev (reverse splits)) (suffix-seg (car rev)) (stem-segs (reverse (cdr rev))))
                        (values stem-segs (list (string-append "." suffix-seg)))
                      ) ;let*
                    ) ;if
                  ) ;let
            ) ;else
      ) ;cond
    ) ;define

    ;; ; 返回末段替换为 new-name 后的新 path(保留 drive/root/其他段)。
    ;; ; 差异:替换后 Windows drive/anchor 保留(如 C:\a\b.txt → C:\a\c.md),
    ;; ; posix 同理(/a/b.txt → /a/c.md)。
    ;; ; 空路径(parts 为空或仅 ".")直接返回单段 new-name 的相对路径。
    (define (replace-last-segment p new-name)
      (let* ((pp (path p))
             (parts (path-record-parts pp))
             (n (vector-length parts))
             (type (path-record-type pp))
             (drive (path-record-drive pp))
             (root (path-record-root pp))
            ) ;
        (cond ((or (= n 0) (and (= n 1) (string=? (vector-ref parts 0) ".")))
               (make-path-record (vector new-name) 'posix "" #f)
              ) ;
              (else (let ((new-parts (vector-copy parts)))
                      (vector-set! new-parts (- n 1) new-name)
                      (make-path-record new-parts type drive root)
                    ) ;let
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define

    ;; ; 过滤 path-from-parts 中首段 anchor 之后的空段/分隔符 stub。
    ;; ; 同时识别 posix "/" 与 Windows "\\" 作为分隔符(避免污染)。
    (define (clean-tail parts)
      (vector-filter (lambda (part)
                       (not (or (string-null? part) (string=? part "/") (string=? part "\\")))
                     ) ;lambda
        (vector-drop parts 1)
      ) ;vector-filter
    ) ;define

    ;; ; 字符类 [..] 匹配单字符 ch。pattern[j] 是 '['。
    ;; ; 返回 (values matched? next-index-after-])。
    (define (charclass-match-one pattern plen j ch)
      (let ((negate (and (< (+ j 1) plen) (char=? (string-ref pattern (+ j 1)) #\^))))
        (let scan
          ((k (if negate (+ j 2) (+ j 1))) (matched #f))
          (cond ((or (>= k plen) (char=? (string-ref pattern k) #\]))
                 (values (if negate (not matched) matched) (if (< k plen) (+ k 1) k))
                ) ;
                ((and (< (+ k 2) plen) (char=? (string-ref pattern (+ k 1)) #\-))
                 (let ((lo (string-ref pattern k)) (hi (string-ref pattern (+ k 2))))
                   (let ((hit (if (char<=? lo hi)
                                (and (char>=? ch lo) (char<=? ch hi))
                                (and (char>=? ch hi) (char<=? ch lo))
                              ) ;if
                         ) ;hit
                        ) ;
                     (scan (+ k 3) (or matched hit))
                   ) ;let
                 ) ;let
                ) ;
                (else (scan (+ k 1) (or matched (char=? (string-ref pattern k) ch))))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    ;; ; glob 单层匹配(支持 * / ? / [seq],不跨分隔符)。平台无关
    ;; ; (大小写敏感性由 path-match 在 Windows 类型下统一 downcase 处理)。
    (define (glob-match? pattern str)
      (let ((plen (string-length pattern)) (slen (string-length str)))
        (letrec ((match-at (lambda (p0 s0)
                             (cond ((= p0 plen) (= s0 slen))
                                   ((char=? (string-ref pattern p0) #\*)
                                    ;; * 尝试匹配 0..(slen-s0) 个字符
                                    (let try-star
                                      ((n 0))
                                      (cond ((match-at (+ p0 1) (+ s0 n)) #t)
                                            ((< (+ s0 n) slen) (try-star (+ n 1)))
                                            (else #f)
                                      ) ;cond
                                    ) ;let
                                   ) ;
                                   ((= s0 slen) #f)
                                   ((char=? (string-ref pattern p0) #\?) (match-at (+ p0 1) (+ s0 1)))
                                   ((char=? (string-ref pattern p0) #\[)
                                    (let-values (((hit next) (charclass-match-one pattern plen p0 (string-ref str s0))))
                                      (and hit (match-at next (+ s0 1)))
                                    ) ;let-values
                                   ) ;
                                   ((char=? (string-ref pattern p0) (string-ref str s0))
                                    (match-at (+ p0 1) (+ s0 1))
                                   ) ;
                                   (else #f)
                             ) ;cond
                           ) ;lambda
                 ) ;match-at
                ) ;
          (match-at 0 0)
        ) ;letrec
      ) ;let
    ) ;define

    ;; ; 规范化绝对路径:消除 . 段、折叠 .. 段。
    ;; ; 不解析符号链接(无 realpath 原语,与 pathlib strict 语义有差异)。
    (define (normalize-absolute p)
      (let* ((pp (path-absolute p))
             (segs (vector->list (path-record-parts pp)))
             (root (path-record-root pp))
             (type (path-record-type pp))
             (drive (path-record-drive pp))
            ) ;
        (let loop
          ((rest segs) (acc '()))
          (cond ((null? rest) (make-path-record (list->vector (reverse acc)) type drive root))
                ((string=? (car rest) "..")
                 (if (null? acc)
                   ;; .. 在根之上:丢弃(不能越过根)
                   (loop (cdr rest) acc)
                   (loop (cdr rest) (cdr acc))
                 ) ;if
                ) ;
                (else (loop (cdr rest) (cons (car rest) acc)))
          ) ;cond
        ) ;let
      ) ;let*
    ) ;define

    ;; ;============================================================
    ;; ; 公共函数区
    ;; ;============================================================

    ;; ; 对齐 pathlib.PurePath(*pathsegment)
    ;; ; 差异:Windows 路径形式(UNC、C:\、C:foo)只在 Windows 平台识别,
    ;; ; 否则一律当 posix;同一字符串 "/" 在 Windows 被规范化为 windows "\"。
    (define (path . args)
      (if (null? args)
        (make-path-record #(".") 'posix "" #f)
        (let ((arg (car args)))
          (cond ((string? arg)
                 (if (and (os-windows?) (or (unc-prefix? arg) (windows-path-with-drive? arg)))
                   (receive (parts drive root)
                     (parse-windows-path arg)
                     (make-path-record parts 'windows drive root)
                   ) ;receive
                   (receive (parts root)
                     (parse-path-string arg)
                     (let ((type (if (os-windows?) 'windows 'posix)))
                       (make-path-record parts type "" root)
                     ) ;let
                   ) ;receive
                 ) ;if
                ) ;
                ((path? arg) (copy arg))
                (else (type-error "path: argument must be string or path"))
          ) ;cond
        ) ;let
      ) ;if
    ) ;define

    ;; ; 复制文件(跨平台行为:依赖底层 g_path-copy 原语)。
    (define (path-copy source target)
      (let ((src (path->string source)) (dst (path->string target)))
        (if (not (file-exists? src))
          (file-not-found-error (string-append "No such file or directory: '" src "'"))
          (g_path-copy src dst)
        ) ;if
      ) ;let
    ) ;define

    ;; ; 复制文件到目标目录(自动取源文件名作为目标名)。
    (define (path-copy-into source target-dir)
      (let ((filename (path-name (path-from-string source))))
        (path-copy source
          (path->string (path-join (path-from-string target-dir) (path filename)))
        ) ;path-copy
      ) ;let
    ) ;define

    ;; ; 对齐 pathlib.PurePath.parts
    ;; ; 差异:绝对路径首元素是 anchor — posix "/", Windows "C:\"/"\\srv\sh\\"/"\\";
    ;; ; drive-relative(C:foo)首元素 "C:";相对路径无 anchor。
    (define (path-parts p)
      (if (not (path? p))
        (type-error "path-parts: argument must be path")
        (let ((parts (path-record-parts p))
              (root (path-record-root p))
              (drive (path-record-drive p))
              (type (path-record-type p))
             ) ;
          (let ((anchor (anchor-string type drive root)))
            (cond (anchor (vector-append (vector anchor) parts))
                  ((and (= (vector-length parts) 1) (string=? (vector-ref parts 0) ".")) #())
                  (else (vector-copy parts))
            ) ;cond
          ) ;let
        ) ;let
      ) ;if
    ) ;define

    ;; ; 对齐 pathlib.PurePath.parser 风格 — 返回 'posix 或 'windows。
    ;; ; 差异:类型由构造时的平台决定,Windows 上 (path "/") 是 'windows。
    (define (path-type p)
      (if (path? p)
        (path-record-type p)
        (type-error "path-type: argument must be path")
      ) ;if
    ) ;define

    ;; ; 对齐 pathlib.PurePath.drive
    ;; ; 差异:posix 恒为 "";Windows drive-absolute/drive-relative 返回 "C:"(带冒号),
    ;; ; UNC 返回 "\\server\share" 整体(无冒号),相对/current-drive root 返回 ""。
    (define (path-drive p)
      (if (path? p)
        (let* ((drive (path-record-drive p)) (type (path-record-type p)))
          (cond ((eq? type 'posix) "")
                ((unc-prefix? drive) drive)
                ((string-null? drive) "")
                (else (string-append drive ":"))
          ) ;cond
        ) ;let*
        (type-error "path-drive: argument must be path")
      ) ;if
    ) ;define

    ;; ; 对齐 pathlib.PurePath.root
    ;; ; 差异:有根时 posix 返回 "/",Windows 返回 "\";无根(相对、drive-relative、
    ;; ; 仅 server 的 \\srv)返回 ""。
    (define (path-root p)
      (if (path? p)
        (let ((root (path-record-root p)) (type (path-record-type p)))
          (cond ((not root) "")
                ((eq? type 'windows) "\\")
                (else "/")
          ) ;cond
        ) ;let
        (type-error "path-root: argument must be path")
      ) ;if
    ) ;define

    ;; ; 对齐 str(pathlib.PurePath)
    ;; ; 差异:posix 用 "/" 拼接,Windows 用 "\";Windows UNC share anchor
    ;; ; 在只剩 anchor 时带尾斜杠(对齐 pathlib: str('\\srv\sh')=='\\srv\sh\')。
    (define (path->string p)
      (cond ((path? p)
             (let ((parts (path-record-parts p))
                   (type (path-record-type p))
                   (drive (path-record-drive p))
                   (root (path-record-root p))
                  ) ;
               (case type
                ((posix)
                 (let ((body (parts->string parts "/")))
                   (cond ((and root (not (string-null? body))) (string-append "/" body))
                         (root "/")
                         ((string-null? body) ".")
                         (else body)
                   ) ;cond
                 ) ;let
                ) ;
                ((windows)
                 (let ((body (parts->string parts "\\")))
                   (cond
                     ;; UNC: drive 字段含 \\server[\share] anchor。
                     ;; ; 完整 share anchor(\\server\share)只剩 anchor 时带尾斜杠;
                     ;; ; 光秃 server(\\srv)不带尾斜杠(root 为空)。
                     ((unc-prefix? drive)
                      (if (string-null? body)
                        (if (string-index (substring drive 2 (string-length drive)) #\\)
                          (string-append drive "\\")
                          drive
                        ) ;if
                        (string-append drive "\\" body)
                      ) ;if
                     ) ;
                     ;; drive-absolute 或 drive-relative: drive 是单个盘符如 "C"
                     ((not (string-null? drive))
                      (let ((prefix (string-append drive ":" (if root "\\" ""))))
                        (if (string-null? body) prefix (string-append prefix body))
                      ) ;let
                     ) ;
                     ;; 无 drive: root=#\\ 为当前盘根 "\foo",否则为相对 "foo\bar"
                     (root (if (string-null? body) "\\" (string-append "\\" body)))
                     (else (if (string-null? body) "." body))
                   ) ;cond
                 ) ;let
                ) ;
                (else (value-error "path->string: unknown type"))
               ) ;case
             ) ;let
            ) ;
            ((string? p) p)
            (else (type-error "path->string: argument must be path or string"))
      ) ;cond
    ) ;define

    ;; ; 对齐 pathlib.PurePath(s)
    (define (path-from-string s)
      (path s)
    ) ;define

    ;; ; 对应 pathlib 的 == 操作符
    ;; ; 差异:按 path->string 字符串比对,Windows 上 "C:\a" 与 "C:/a" 不等
    ;; ; (一个反斜杠、一个正斜杠);posix 上 "/" 与 "\" 不等。
    (define (path-equals? p1 p2)
      (let ((s1 (path->string (path p1))) (s2 (path->string (path p2))))
        (string=? s1 s2)
      ) ;let
    ) ;define

    (define path=? path-equals?)

    ;; ; 对齐 pathlib.PurePath.is_absolute()
    ;; ; 差异:posix 仅看 root 字段;Windows 要求 drive 非空且 root 非空
    ;; ; (C:\foo、\\srv\sh\foo 绝对;C:foo、\foo 都不是绝对)。
    (define (path-absolute? p)
      (if (path? p)
        (let ((type (path-record-type p))
              (drive (path-record-drive p))
              (root (path-record-root p))
             ) ;
          (case type
           ((windows) (and root (not (string-null? drive))))
           ((posix) (and root #t))
           (else #f)
          ) ;case
        ) ;let
        (path-absolute? (path p))
      ) ;if
    ) ;define

    ;; ; 对应 path-absolute? 的取反。差异:同 path-absolute? 的 Windows/posix 判定。
    (define (path-relative? p)
      (not (path-absolute? p))
    ) ;define

    ;; ; 对齐 pathlib.PurePath.name
    ;; ; 差异:跨平台一致 — 直接取 record parts 末段,不依赖 path->string
    ;; ; 与平台 sep。空 parts 或仅 "." 返回 ""。
    (define (path-name p)
      (let* ((pp (path p)) (parts (path-record-parts pp)) (n (vector-length parts)))
        (cond ((= n 0) "")
              ((and (= n 1) (string=? (vector-ref parts 0) ".")) "")
              (else (vector-ref parts (- n 1)))
        ) ;cond
      ) ;let*
    ) ;define

    ;; ; 对齐 pathlib.PurePath.stem
    ;; ; 差异:基于 path-name,跨平台一致。隐藏文件(.bashrc)整体作 stem。
    (define (path-stem p)
      (let-values (((stem-segs _) (split-name-dots (path-name p))))
        (string-join stem-segs ".")
      ) ;let-values
    ) ;define

    ;; ; 对齐 pathlib.PurePath.suffix
    ;; ; 差异:基于 path-name 末段,跨平台一致。无后缀返回 ""。
    (define (path-suffix p)
      (let-values (((_ suffix-segs) (split-name-dots (path-name p))))
        (if (null? suffix-segs) "" (car suffix-segs))
      ) ;let-values
    ) ;define

    ;; ; 对齐 pathlib.PurePath.suffixes
    ;; ; 差异:基于 path-name 末段,跨平台一致。隐藏文件/无点/"."/".." 返回 #()。
    ;; ; 末尾点(foo.)返回 #("."),对齐 Python 3.14+ pathlib。
    (define (path-suffixes p)
      (let ((name (path-name p)))
        (cond ((or (string=? name ".") (string=? name "..")) #())
              (else (let ((splits (string-split name #\.)))
                      (if (or (<= (length splits) 1) (string=? (car splits) ""))
                        #()
                        (list->vector (map (lambda (s) (string-append "." s)) (cdr splits)))
                      ) ;if
                    ) ;let
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    ;; ; 对齐 pathlib.PurePath.with_name(name)
    ;; ; 差异:保留原路径的 drive/root/类型;Windows 上 C:\a\b → C:\a\new。
    (define (path-with-name p new-name)
      (if (not (string? new-name))
        (type-error "path-with-name: new-name must be string")
        (replace-last-segment p new-name)
      ) ;if
    ) ;define

    ;; ; 对齐 pathlib.PurePath.with_stem(stem)
    ;; ; 差异:保留最后一个后缀(a.tar.gz → new.gz);Windows 上保留 drive/root。
    (define (path-with-stem p new-stem)
      (if (not (string? new-stem))
        (type-error "path-with-stem: new-stem must be string")
        (let ((suffix (path-suffix p)))
          ;; 无后缀(含隐藏文件):替换整段,等价于 with-name
          (if (string-null? suffix)
            (replace-last-segment p new-stem)
            (replace-last-segment p (string-append new-stem suffix))
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    ;; ; 对齐 pathlib.PurePath.with_suffix(suffix)
    ;; ; 差异:ext="" 去后缀;ext 必须以 "." 开头;Windows 上保留 drive/root。
    (define (path-with-suffix p ext)
      (if (not (string? ext))
        (type-error "path-with-suffix: ext must be string")
        (let* ((name (path-name p)) (stem (path-stem p)))
          (cond ((string-null? ext)
                 (if (string-null? (path-suffix (path p)))
                   (path p)
                   (replace-last-segment p stem)
                 ) ;if
                ) ;
                ((not (char=? (string-ref ext 0) #\.))
                 (value-error "path-with-suffix: ext must start with '.'")
                ) ;
                (else (replace-last-segment p (string-append stem ext)))
          ) ;cond
        ) ;let*
      ) ;if
    ) ;define

    ;; ; 对齐 pathlib.PurePath.relative_to(*other)
    ;; ; 差异:要求两者 anchor(drive/root/type)完全一致 — posix 上 root 必须相同,
    ;; ; Windows 上 drive 和 root 都必须相同。否则抛 value-error。
    (define (path-relative-to p base)
      (let* ((pp (path p))
             (bp (path base))
             (p-segs (path-record-parts pp))
             (b-segs (path-record-parts bp))
             (p-drive (path-record-drive pp))
             (b-drive (path-record-drive bp))
             (p-root (path-record-root pp))
             (b-root (path-record-root bp))
             (pn (vector-length p-segs))
             (bn (vector-length b-segs))
            ) ;
        (define (same-anchor?)
          (and (string=? p-drive b-drive)
            (eq? p-root b-root)
            (eq? (path-record-type pp) (path-record-type bp))
          ) ;and
        ) ;define
        (define (prefix-match?)
          ;; 检查 b-segs 是 p-segs 的前缀
          (let loop
            ((i 0))
            (cond ((= i bn) #t)
                  ((not (string=? (vector-ref p-segs i) (vector-ref b-segs i))) #f)
                  (else (loop (+ i 1)))
            ) ;cond
          ) ;let
        ) ;define
        (define (remaining-segments)
          ;; 取 p-segs 从 bn 起的剩余段
          (let* ((rlen (- pn bn)) (rest (make-vector rlen)))
            (let fill
              ((j 0))
              (if (= j rlen)
                rest
                (begin
                  (vector-set! rest j (vector-ref p-segs (+ bn j)))
                  (fill (+ j 1))
                ) ;begin
              ) ;if
            ) ;let
          ) ;let*
        ) ;define
        (cond ((not (same-anchor?))
               (value-error "path-relative-to: paths do not share an anchor")
              ) ;
              ((> bn pn) (value-error "path-relative-to: path is not relative to base"))
              ((not (prefix-match?))
               (value-error "path-relative-to: path is not relative to base")
              ) ;
              ((= bn pn) (path "."))
              (else (make-path-record (remaining-segments) (path-record-type pp) "" #f))
        ) ;cond
      ) ;let*
    ) ;define

    ;; ; 对齐 pathlib.PurePath.is_relative_to(other)
    ;; ; 返回 #t 当且仅当 p 是 base 本身或 base 的后代路径。
    (define (path-starts-with? p base)
      (let* ((pp (path p))
             (bp (path base))
             (p-segs (path-record-parts pp))
             (b-segs (path-record-parts bp))
             (p-drive (path-record-drive pp))
             (b-drive (path-record-drive bp))
             (p-root (path-record-root pp))
             (b-root (path-record-root bp))
             (pn (vector-length p-segs))
             (bn (vector-length b-segs))
            ) ;
        (define (same-anchor?)
          (and (string=? p-drive b-drive)
            (eq? p-root b-root)
            (eq? (path-record-type pp) (path-record-type bp))
          ) ;and
        ) ;define
        (define (prefix-match?)
          (let loop
            ((i 0))
            (cond ((= i bn) #t)
                  ((not (string=? (vector-ref p-segs i) (vector-ref b-segs i))) #f)
                  (else (loop (+ i 1)))
            ) ;cond
          ) ;let
        ) ;define
        (and (same-anchor?) (>= pn bn) (prefix-match?))
      ) ;let*
    ) ;define

    ;; ; 对齐 pathlib.PurePath.joinpath(*pathsegments)
    ;; ; 差异:Windows 下 drive 继承/重置规则复杂 ——
    ;; ;   完整绝对段(C:\b 或 \\srv\sh\a)替换 acc;
    ;; ;   drive-relative 段(D:rel):drive 与 acc 不同→替换,相同→追加(C:\base.joinpath('C:rel')=>C:\base\rel);
    ;; ;   current-drive root 段(\b):继承 acc drive,重置主体(C:\base.joinpath('\b')=>C:\b)。
    ;; ; posix 退化为"绝对段替换 / 相对段追加"。
    (define (path-join base . segments)
      (let ((base-rec (path base)))
        (define (append-parts acc seg)
          (define (drop-dots v)
            (vector-filter (lambda (x) (not (string=? x "."))) v)
          ) ;define
          (let ((acc-parts (drop-dots (path-record-parts acc)))
                (seg-parts (drop-dots (path-record-parts seg)))
               ) ;
            (make-path-record (vector-append acc-parts seg-parts)
              (path-record-type acc)
              (path-record-drive acc)
              (path-record-root acc)
            ) ;make-path-record
          ) ;let
        ) ;define
        (define (join-one acc seg)
          (let ((seg-type (path-record-type seg))
                (seg-drive (path-record-drive seg))
                (seg-root (path-record-root seg))
                (acc-drive (path-record-drive acc))
               ) ;
            (cond
              ;; 完整绝对段:替换 acc
              ((path-absolute? seg) seg)
              ;; Windows drive-relative 段(有 drive 无 root)
              ((and (eq? seg-type 'windows) (not (string-null? seg-drive)) (not seg-root))
               (if (string=? seg-drive acc-drive) (append-parts acc seg) seg)
              ) ;
              ;; Windows current-drive root 段(无 drive 有 root):继承 acc drive,重置主体
              ((and (eq? seg-type 'windows) (string-null? seg-drive) seg-root)
               (make-path-record (path-record-parts seg) 'windows acc-drive seg-root)
              ) ;
              ;; 纯相对段:追加 parts
              (else (append-parts acc seg))
            ) ;cond
          ) ;let
        ) ;define
        (let loop
          ((acc base-rec) (rest segments))
          (if (null? rest)
            acc
            (if (and (string? (car rest)) (string-null? (car rest)))
              (loop acc (cdr rest))
              (loop (join-one acc (path (car rest))) (cdr rest))
            ) ;if
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    ;; ; 对齐 pathlib.PurePath.parent
    ;; ; 差异:posix 根 "/" 的 parent 是自身;Windows UNC anchor (\\srv\sh) parent 是自身,
    ;; ; drive-absolute 单段(C:\Users) parent 回到 "C:\",drive-relative(C:foo) parent 是 "C:",
    ;; ; 相对单段(a) parent 是当前目录 "."。
    (define (path-parent p)
      (let* ((pp (path p))
             (parts (path-record-parts pp))
             (type (path-record-type pp))
             (drive (path-record-drive pp))
             (root (path-record-root pp))
             (n (vector-length parts))
            ) ;
        (cond
          ;; 无段:根路径(/、C:\、\\srv\sh)或空相对路径,parent 为自身
          ((= n 0) pp)
          ;; 仅一段:取决于是否有 anchor。
          ;;  - 有 root(绝对 /a、C:\Users、\foo、\\srv\sh\a):parent 为其 anchor
          ;;  - 无 root 但有 drive(drive-relative C:foo):parent 为 "C:"(pathlib 语义)
          ;;  - 无 root 无 drive(相对 a、foo):parent 为当前目录 "."
          ((= n 1)
           (if root
             (cond ((and (eq? type 'posix) (string-null? drive))
                    (make-path-record #() 'posix "" #\/)
                   ) ;
                   ;; UNC 单段(\\srv\sh\a → \\srv\sh):drive 存的是 share anchor
                   ;; ("\\server\share" 以 \\ 开头,与单字母盘符区分),直接回到
                   ;; 空 parts + 原 drive/root。
                   ((unc-prefix? drive) (make-path-record #() 'windows drive root))
                   ((not (string-null? drive)) (path-of-drive (string-ref drive 0)))
                   ;; 无 drive 但有 root:windows current-drive root (\foo → \)
                   (else (make-path-record #() 'windows "" root))
             ) ;cond
             (if (not (string-null? drive))
               ;; drive-relative 单段:C:foo → C:(保留 drive、root 空、parts 空)
               (make-path-record #() 'windows drive #f)
               (path ".")
             ) ;if
           ) ;if
          ) ;
          ;; 多段:去掉末段,保留 drive/root
          (else (make-path-record (vector-drop-right parts 1) type drive root))
        ) ;cond
      ) ;let*
    ) ;define

    ;; ; 对齐 pathlib.PurePath.parents
    ;; ; 差异:绝对路径终止于根/anchor;相对路径终止于当前目录 "."。
    ;; ; 跨平台行为一致 — 基于 path-parent 反复回溯。
    (define (path-parents p)
      (let ((start (path p)))
        (let loop
          ((cur start) (acc '()))
          (let* ((par (path-parent cur))
                 (par-str (path->string par))
                 (cur-str (path->string cur))
                ) ;
            (cond
              ;; parent 等于自身(cur 已是根/anchor,或 cur 自身是 "."):停止,不纳入。
              ;; 覆盖 'a'(parent=.)→ 但 cur≠.,故不命中此条;覆盖 '.'(parent=.,cur=.)→ 命中,返回 ()。
              ((string=? par-str cur-str) (list->vector (reverse acc)))
              ;; 相对路径回溯到当前目录且 cur 自身不是 ".":纳入 "." 后停止。
              ((string=? par-str ".") (list->vector (reverse (cons (path ".") acc))))
              (else (loop par (cons par acc)))
            ) ;cond
          ) ;let*
        ) ;let
      ) ;let
    ) ;define

    ;; ; 判断是否为目录。跨平台行为一致。
    (define (path-dir? p)
      (g_isdir (path->string p))
    ) ;define

    ;; ; 判断是否为普通文件。跨平台行为一致。
    (define (path-file? p)
      (g_isfile (path->string p))
    ) ;define

    ;; ; 判断路径是否存在(文件或目录)。跨平台行为一致。
    (define (path-exists? p)
      (file-exists? (path->string p))
    ) ;define

    ;; ; 获取文件大小(字节)。不存在时抛 file-not-found-error。
    (define (path-getsize p)
      (let ((s (path->string p)))
        (if (not (file-exists? s))
          (file-not-found-error (string-append "No such file or directory: '" s "'"))
          (g_path-getsize s)
        ) ;if
      ) ;let
    ) ;define

    ;; ; 读取文件为文本。不存在时抛 file-not-found-error。
    (define (path-read-text p)
      (let ((s (path->string p)))
        (if (not (file-exists? s))
          (file-not-found-error (string-append "No such file or directory: '" s "'"))
          (g_path-read-text s)
        ) ;if
      ) ;let
    ) ;define

    ;; ; 读取文件为字节向量。不存在时抛 file-not-found-error。
    (define (path-read-bytes p)
      (let ((s (path->string p)))
        (if (not (file-exists? s))
          (file-not-found-error (string-append "No such file or directory: '" s "'"))
          (g_path-read-bytes s)
        ) ;if
      ) ;let
    ) ;define

    ;; ; 覆盖写入文本。content 必须为 string。
    (define (path-write-text p content)
      (if (not (string? content))
        (type-error "path-write-text: content must be string")
        (g_path-write-text (path->string p) content)
      ) ;if
    ) ;define

    ;; ; 覆盖写入字节。data 必须为 bytevector。
    (define (path-write-bytes p data)
      (if (not (byte-vector? data))
        (type-error "path-write-bytes: data must be bytevector")
        (g_path-write-bytes (path->string p) data)
      ) ;if
    ) ;define

    ;; ; 追加文本到文件末尾。
    (define (path-append-text p content)
      (g_path-append-text (path->string p) content)
    ) ;define

    ;; ; 创建空文件(若不存在),已存在则更新访问时间。
    (define (path-touch p)
      (g_path-touch (path->string p))
    ) ;define

    ;; ; goldfish 特有 — Windows 驱动器根 "C:\" 的便捷构造器。
    ;; ; drive="C", root=#\\, parts=#()。仅 Windows 路径用,posix 无对应。
    (define (path-of-drive ch)
      (if (char? ch)
        (make-path-record #() 'windows (string (ascii-upcase ch)) #\\)
        (type-error "path-of-drive: argument must be char")
      ) ;if
    ) ;define

    ;; ; goldfish 特有 — 从 parts vector 构造路径(与 path-parts 互为逆运算)。
    ;; ; 差异:识别首段 anchor(对齐 pathlib.parts 首元素形式):
    ;; ;   "C:\"   → drive-absolute(windows), root=#\\
    ;; ;   "C:"    → drive-relative(windows),  root=#f
    ;; ;   "\\server\share\" → UNC share anchor, drive="\\server\share", root=#\\
    ;; ;   "\\"    → windows 当前驱动器根, root=#\\
    ;; ;   "/"     → posix 绝对, root=#\/
    (define (path-from-parts parts)
      (if (not (vector? parts))
        (type-error "path-from-parts: argument must be vector")
        (if (= (vector-length parts) 0)
          (make-path-record #(".") 'posix "" #f)
          (let ((head (vector-ref parts 0)))
            (cond ((and (string? head) (unc-prefix? head))
                   ;; UNC share anchor: \\server\share\[...] → drive="\\server\share", root=#\\
                   (let* ((hend (string-length head))
                          (trimmed (if (char=? (string-ref head (- hend 1)) #\\)
                                     (substring head 0 (- hend 1))
                                     head
                                   ) ;if
                          ) ;trimmed
                         ) ;
                     (make-path-record (clean-tail parts) 'windows trimmed #\\)
                   ) ;let*
                  ) ;

                  ((and (string? head) (windows-path-with-drive? head))
                   (let ((drive (extract-drive head)) (hend (string-length head)))
                     ;; 首段含尾反斜杠(C:\) → drive-absolute(root=#\\);仅 C: → drive-relative(root=#f)
                     (let ((root (if (and (> hend 2) (char=? (string-ref head 2) #\\)) #\\ #f)))
                       (make-path-record (clean-tail parts) 'windows drive root)
                     ) ;let
                   ) ;let
                  ) ;

                  ((string=? head "/")
                   ;; posix 绝对：保留 caller 传的剩余 parts（不再剥离 "."，因为 caller 显式传）
                   (make-path-record (vector-copy (vector-drop parts 1)) 'posix "" #\/)
                  ) ;

                  ((string=? head "\\")
                   ;; windows 当前驱动器根
                   (make-path-record (vector-copy (vector-drop parts 1)) 'windows "" #\\)
                  ) ;

                  ((string-null? head)
                   ;; 老式空 stub 开头，按 posix 绝对处理
                   (make-path-record (vector-copy (vector-drop parts 1)) 'posix "" #\/)
                  ) ;

                  (else (make-path-record (vector-copy parts) 'posix "" #f))
            ) ;cond
          ) ;let
        ) ;if
      ) ;if
    ) ;define

    ;; ; goldfish 特有 — 从环境变量构造路径(等价于 (path (getenv name)))。
    (define (path-from-env name)
      (path (getenv name))
    ) ;define

    ;; ; 当前工作目录(pathlib.Path.cwd() 的模块函数版本)。
    (define (path-cwd)
      (path (getcwd))
    ) ;define

    ;; ; 对齐 pathlib.Path.home 的环境变量解析顺序。
    ;; ; 差异:POSIX 查 HOME;Windows 查 USERPROFILE,缺失则回退 HOMEDRIVE+HOMEPATH。
    (define (path-home)
      (cond ((or (os-linux?) (os-macos?))
             (let ((h (getenv "HOME")))
               (if h (path h) (value-error "path-home: HOME is not set"))
             ) ;let
            ) ;
            ((os-windows?)
             (let ((profile (getenv "USERPROFILE")))
               (cond (profile (path profile))
                     ((and (getenv "HOMEDRIVE") (getenv "HOMEPATH"))
                      (path (string-append (getenv "HOMEDRIVE") (getenv "HOMEPATH")))
                     ) ;
                     (else (value-error "path-home: USERPROFILE and HOMEDRIVE/HOMEPATH are not set"))
               ) ;cond
             ) ;let
            ) ;
            (else (value-error "path-home: unknown platform"))
      ) ;cond
    ) ;define

    ;; ; 系统临时目录(POSIX 通常 /tmp,Windows 通常 %TEMP%)。
    (define (path-temp-dir)
      (path (os-temp-dir))
    ) ;define

    ;; ; 列出目录内容(字符串向量)。
    (define (path-list p)
      (listdir (path->string p))
    ) ;define

    ;; ; 列出目录内容(path 对象向量,每个元素 = base join entry)。
    (define (path-list-path p)
      (let ((base (path->string p)))
        (let ((entries (listdir base)))
          (vector-map (lambda (entry) (path-join base entry)) entries)
        ) ;let
      ) ;let
    ) ;define

    ;; ; 删除目录(必须为空)。
    (define (path-rmdir p)
      (rmdir (path->string p))
    ) ;define

    ;; ; 删除文件。missing-ok=#t 时文件不存在不报错(对齐 pathlib.Path.unlink)。
    (define* (path-unlink p (missing-ok #f))
      (let ((s (path->string p)))
        (cond ((file-exists? s) (remove s))
              (missing-ok #t)
              (else (error 'file-not-found-error (string-append "File not found: " s)))
        ) ;cond
      ) ;let
    ) ;define*

    ;; ; 重命名/移动文件或目录(对齐 pathlib.Path.rename)。
    (define (path-rename src dst)
      (rename (path->string src) (path->string dst))
    ) ;define

    ;; ; 对齐 pathlib.Path.absolute()
    ;; ; 差异:相对路径拼 cwd,绝对路径原样返回。不解析符号链接(区别于 path-resolve)。
    (define (path-absolute p)
      (let ((pp (path p)))
        (if (path-absolute? pp) pp (path (path-join (path-cwd) pp)))
      ) ;let
    ) ;define

    ;; ; 对齐 pathlib.Path.expanduser()(仅 ~ 部分;不支持 ~user)
    ;; ; 差异:~ 展开为 path-home() 的结果 — POSIX 上是 HOME,Windows 上是 USERPROFILE。
    (define (path-expanduser p)
      (let* ((pp (path p)) (s (path->string pp)))
        (if (string-starts? s "~")
          (let* ((rest (substring s 1 (string-length s))) (home-str (path->string (path-home))))
            (path (cond ((string-null? rest) home-str)
                        ((string-starts? rest "/") (string-append home-str rest))
                        (else (string-append home-str "/" rest))
                  ) ;cond
            ) ;path
          ) ;let*
          pp
        ) ;if
      ) ;let*
    ) ;define

    ;; ; 对齐 pathlib.PurePath.as_posix()
    ;; ; 差异:posix 下等价于 path->string;Windows 下把 \ 转成 /。
    (define (path-as-posix p)
      (let ((s (path->string p)))
        (if (os-windows?) (string-replace s "\\" "/") s)
      ) ;let
    ) ;define

    ;; ; 对齐 pathlib.PurePath.match(pattern)
    ;; ; 差异:Windows 类型大小写不敏感(Foo.TXT 匹配 *.txt),posix 类型大小写敏感。
    ;; ; 模式可含分隔符,从右向左逐段匹配;模式以分隔符开头(绝对模式)要求段数完全匹配。
    (define (path-match p pattern)
      (let* ((pp (path p))
             (type (path-record-type pp))
             (sep (if (eq? type 'windows) #\\ #\/))
             ;; Windows match 大小写不敏感:统一转小写后再匹配。
             (ci? (eq? type 'windows))
             (down (lambda (s) (if ci? (string-downcase s) s)))
             (segs (map down (vector->list (path-record-parts pp))))
             (pat-raw (map down (string-split pattern sep)))
             ;; 绝对模式:pattern 以 sep 开头会产生首段空串
             (absolute-pattern? (and (>= (string-length pattern) 1) (char=? (string-ref pattern 0) sep))
             ) ;absolute-pattern?
             (pat-segs (if absolute-pattern? (cdr pat-raw) pat-raw))
            ) ;
        ;; 逐段从右匹配 pat-segs 与 segs 的尾部;绝对模式要求段数完全匹配
        (let loop
          ((ps (reverse pat-segs)) (ss (reverse segs)))
          (cond ((null? ps) (or (not absolute-pattern?) (null? ss)))
                ((null? ss) #f)
                ((glob-match? (car ps) (car ss)) (loop (cdr ps) (cdr ss)))
                (else #f)
          ) ;cond
        ) ;let
      ) ;let*
    ) ;define

    ;; ; 对齐 pathlib.Path.resolve(strict=False) 简化版
    ;; ; 差异:绝对化 + 折叠 . / .. 段;不解析符号链接(无 realpath 原语)。
    ;; ; Windows 与 posix 均适用,基于 path-absolute 取 cwd 作种子。
    (define (path-resolve p)
      (normalize-absolute p)
    ) ;define

    ;; ; 对齐 pathlib.Path.mkdir(parents, exist_ok)
    ;; ; 差异:parents=#t 递归创建中间目录(从根向下,跳过已存在的);
    ;; ; exist_ok=#t 时已存在不报错。跨平台行为一致。
    (define* (path-mkdir p (parents #f) (exist-ok #f))
      (define (ensure-one s)
        (if (file-exists? s) #t (mkdir s))
      ) ;define
      (let ((s (path->string p)))
        (cond ((and (not exist-ok) (file-exists? s))
               (file-exists-error (string-append "File exists: '" s "'"))
              ) ;
              ((not parents) (ensure-one s))
              (else
                ;; parents:从根向下逐级创建(跳过已存在的)
                (let loop
                  ((rest (reverse (vector->list (path-parents (path s))))))
                  (if (null? rest)
                    (ensure-one s)
                    (begin
                      (ensure-one (path->string (car rest)))
                      (loop (cdr rest))
                    ) ;begin
                  ) ;if
                ) ;let
              ) ;else
        ) ;cond
      ) ;let
    ) ;define*

  ) ;begin
) ;define-library
