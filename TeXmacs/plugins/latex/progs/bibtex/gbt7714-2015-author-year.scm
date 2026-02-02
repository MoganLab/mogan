
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : gbt7714-2015-author-year.scm
;; DESCRIPTION : GBT 7714-2015-author-year style for BibTeX files
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (bibtex gbt7714-2015-author-year)
  (:use (bibtex bib-utils) (bibtex plain)))

(bib-define-style "gbt7714-2015-author-year" "gbt7714-2015-author-year")

;; 哈希表用于存储年份后缀
(define gbt-year-suffix-table #f)
(define gbt-author-year-count-table #f)

;; 安全字符串转换：确保值转换为字符串，如果为#f则返回空字符串
(define (safe-tm->string x)
  (cond
    ((string? x) x)
    ((not x) "")
    (else (let ((result (tm->string x)))
            (if result result "")))))

;; 获取作者键字符串（用于生成哈希表键）
(define (gbt-get-author-key-string x)
  (let* ((field-info (gbt-get-author-field x))
         (field-type (car field-info))
         (field-list (cdr field-info))
         (has-author (not (equal? field-type 'empty)))
         (chinese? (if has-author (authors-contain-chinese? field-list) #f)))
    (if has-author
        (let* ((n (length field-list))
               (author-count (- n 1)))
          ;; 辅助函数：提取作者姓氏（英文）或完整姓名（中文）
          (define (get-author-key i)
            (let ((author (list-ref field-list i)))
              (if chinese?
                  ;; 中文：使用bib-format-name获取完整姓名
                  (let ((formatted-name (bib-format-name author)))
                    (cond
                      ((string? formatted-name) formatted-name)
                      ((not formatted-name) "")
                      (else (safe-tm->string formatted-name))))
                  ;; 英文：只提取姓氏
                  (let ((last-name-raw (list-ref author 3)))
                    (if (bib-null? last-name-raw)
                        ""
                        (let ((last-name (bib-purify last-name-raw)))
                          (cond
                            ((string? last-name) last-name)
                            ((not last-name) "")
                            (else (safe-tm->string last-name)))))))))
          (cond
            ;; 作者数 ≥ 3：显示第一位 + et al/等
            ((>= author-count 3)
             (let ((first (get-author-key 1)))
               (if chinese?
                   (string-append first "<#7b49>")
                   (string-append first " et al."))))
            ;; 作者数 = 2：显示两位，用"and"或"和"连接
            ((= author-count 2)
             (let ((first (get-author-key 1))
                   (second (get-author-key 2)))
               (if chinese?
                   (string-append first "<#548C>" second)
                   (string-append first " & " second))))
            ;; 作者数 = 1：显示一位
            ((= author-count 1)
             (get-author-key 1))
            ;; 其他情况
            (else "")))
        "")))

;; 预处理函数：计算同一年同一作者文献的年份后缀
(tm-define (bib-preprocessing t)
  (:mode bib-gbt7714-2015-author-year?)
  ;; 初始化哈希表
  (set! gbt-year-suffix-table (make-hash-table 100))
  (set! gbt-author-year-count-table (make-hash-table 100))
  ;; 第一遍：统计每个作者-年份组合的文献数量（所有条目）
  (for-each (lambda (entry)
              (when (func? entry 'bib-entry)
                (let* ((author-short (gbt-get-author-key-string entry))
                       (year-raw (bib-field entry "year"))
                       (year-expr (cond ((bib-null? year-raw) "?")
                                        ((string? year-raw) year-raw)
                                        ((not year-raw) "?")
                                        (else (safe-tm->string year-raw))))
                       (year (cond ((string? year-expr) year-expr)
                                   ((not year-expr) "?")
                                   (else (safe-tm->string year-expr))))
                       (author-year-key (string-append author-short ":" year)))
                  (ahash-set! gbt-author-year-count-table
                             author-year-key
                             (+ 1 (or (ahash-ref gbt-author-year-count-table author-year-key) 0))))))
            t)
  ;; 第二遍：按排序顺序分配后缀
  (let ((sorted-entries (bib-sorted-entries t)))
    (for-each (lambda (entry)
                (when (func? entry 'bib-entry)
                  (let* ((key (list-ref entry 2))
                         (author-short (gbt-get-author-key-string entry))
                         (year-raw (bib-field entry "year"))
                         (year-expr (cond ((bib-null? year-raw) "?")
                                          ((string? year-raw) year-raw)
                                          ((not year-raw) "?")
                                          (else (safe-tm->string year-raw))))
                         (year (cond ((string? year-expr) year-expr)
                                     ((not year-expr) "?")
                                     (else (safe-tm->string year-expr))))
                         (author-year-key (string-append author-short ":" year))
                         (count (ahash-ref gbt-author-year-count-table author-year-key)))
                    (if (> count 1)
                        ;; 需要后缀：分配a, b, c...
                        ;; 使用哈希表记录已分配的后缀索引（按作者-年份组合）
                        (let* ((suffix-index-key (string-append author-year-key ":index"))
                               (next-index (or (ahash-ref gbt-year-suffix-table suffix-index-key) 0))
                               (suffix-char (integer->char (+ 97 next-index))) ; 97 = 'a'
                               (suffix (string suffix-char)))
                          (ahash-set! gbt-year-suffix-table key suffix)
                          (ahash-set! gbt-year-suffix-table suffix-index-key (+ next-index 1)))
                        ;; 单一文献，无后缀
                        (ahash-set! gbt-year-suffix-table key "")))))
              sorted-entries)))

;; 重写条目格式函数以支持所有文献类型
(tm-define (bib-format-entry n x)
  (:mode bib-gbt7714-2015-author-year?)
  (if (and (list? x) (func? x 'bib-entry)
           (= (length x) 4) (func? (list-ref x 3) 'document))
      (with doctype (list-ref x 1)
        (gbt-remove-keepcase
         (cond
           ((equal? doctype "article") (bib-format-article n x))
           ((equal? doctype "book") (bib-format-book n x))
           ((equal? doctype "booklet") (bib-format-booklet n x))
           ((equal? doctype "inbook") (bib-format-inbook n x))
           ((equal? doctype "incollection") (bib-format-incollection n x))
           ((equal? doctype "inproceedings") (bib-format-inproceedings n x))
           ((equal? doctype "conference") (bib-format-inproceedings n x))
           ((equal? doctype "manual") (bib-format-manual n x))
           ((equal? doctype "mastersthesis") (bib-format-mastersthesis n x))
           ((equal? doctype "misc") (bib-format-misc n x))
           ((equal? doctype "phdthesis") (bib-format-phdthesis n x))
           ((equal? doctype "proceedings") (bib-format-proceedings n x))
           ((equal? doctype "techreport") (bib-format-techreport n x))
           ((equal? doctype "unpublished") (bib-format-unpublished n x))
           ;; GBT 7714-2015 新增类型
           ((equal? doctype "standard") (bib-format-standard n x))
           ((equal? doctype "database") (bib-format-database n x))
           ((equal? doctype "software") (bib-format-software n x))
           ((equal? doctype "program") (bib-format-program n x))
           ((equal? doctype "archive") (bib-format-archive n x))
           ((equal? doctype "map") (bib-format-map n x))
           ((equal? doctype "dataset") (bib-format-dataset n x))
           ((equal? doctype "electronic") (bib-format-electronic n x))
           ((equal? doctype "online") (bib-format-online n x))
           ((equal? doctype "newspaper") (bib-format-newspaper n x))
           ((equal? doctype "collection") (bib-format-collection n x))
           ((equal? doctype "patent") (bib-format-patent n x))
           ((equal? doctype "other") (bib-format-other n x))
           (else (bib-format-misc n x)))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; cite相关
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 获取作者字段（优先使用author，如果没有则使用editor）
(define (gbt-get-author-field x)
  ;; editor和author同逻辑：优先使用author，没有author才使用editor
  (let ((author-field (bib-field x "author"))
        (editor-field (bib-field x "editor")))
    (cond
      ((not (or (bib-null? author-field) (nlist? author-field))) (cons 'author author-field))
      ((not (or (bib-null? editor-field) (nlist? editor-field))) (cons 'editor editor-field))
      (else (cons 'empty '())))))

;; 为作者字符串添加后缀（如果是editor）
(define (gbt-add-suffix author-str field-type chinese? count)
  ;; editor和author同逻辑：都不加后缀
  author-str)

;; 获取作者字符串（用于natbib-triple的author字段 - 完整格式，用于参考文献表）
(tm-define (gbt-get-author-string x)
  (:mode bib-gbt7714-2015-author-year?)
  (let* ((field-info (gbt-get-author-field x))
         (field-type (car field-info))
         (field-list (cdr field-info))
         (has-author (not (equal? field-type 'empty)))
         (chinese? (if has-author (authors-contain-chinese? field-list) #f)))
    (if has-author
        (let* ((n (length field-list))
               (author-count (- n 1))
               ;; 参考文献表阈值：GB/T 7714-2015规定：≤3人全部列出，≥4人写"前3人+等"
               (max-authors 3)
               (show-count (min author-count max-authors))
               (has-more (> author-count max-authors))
               ;; 分隔符：中文不加空格，英文加空格
               (separator (if chinese? "," ", ")))
          (cond
            ((= author-count 1)
             (let ((author-name (bib-format-name (list-ref field-list 1))))
               (gbt-add-suffix author-name field-type chinese? 1)))
            (else
             (let* ((first (bib-format-name (list-ref field-list 1)))
                    ;; 收集中间作者
                    ;; 如果有更多作者（>3）：收集第2到第3个作者（共2个中间作者）
                    ;; 如果没有更多作者（<=3）：收集第2到第author-count-1个作者
                    (middle (let loop ((i 2) (result ""))
                              (if (or (>= i n)
                                      (if has-more
                                          (> i max-authors)
                                          (>= i author-count)))
                                  result
                                  (loop (+ i 1)
                                        (if (equal? result "")
                                            (bib-format-name (list-ref field-list i))
                                            `(concat ,result ,separator ,(bib-format-name (list-ref field-list i))))))))
                    (last-part (if has-more
                                   (if chinese? "<#7b49>" "et al") ;;等
                                   (if (>= author-count 2)
                                       (bib-format-name (list-ref field-list (- n 1)))
                                       "")))
                    ;; 构建作者字符串
                    (author-str (cond
                                  ((and (equal? middle "") (equal? last-part "")) first)
                                  ((equal? middle "") `(concat ,first ,separator ,last-part))
                                  ((equal? last-part "") `(concat ,first ,separator ,middle))
                                  (else `(concat ,first ,separator ,middle ,separator ,last-part)))))
               (gbt-add-suffix author-str field-type chinese? author-count)))))
        "")))

;; 获取短作者字符串（用于natbib-triple的author*字段 - 引用标签格式）
(tm-define (gbt-get-author*-string x)
  (:mode bib-gbt7714-2015-author-year?)
  (let* ((field-info (gbt-get-author-field x))
         (field-type (car field-info))
         (field-list (cdr field-info))
         (has-author (not (equal? field-type 'empty)))
         (chinese? (if has-author (authors-contain-chinese? field-list) #f)))
    (if has-author
        (let* ((n (length field-list))
               (author-count (- n 1)))
          ;; 辅助函数：提取作者姓氏（英文）或完整姓名（中文）
          (define (get-author-display i)
            (let ((author (list-ref field-list i)))
              (if chinese?
                  (bib-format-name author)  ;; 中文：完整姓名
                  ;; 英文：只提取姓氏，不大写
                  (let ((last-name-raw (list-ref author 3)))
                    (if (bib-null? last-name-raw)
                        ""
                        (bib-purify last-name-raw))))))
          (cond
            ;; 作者数 ≥ 3：显示第一位 + et al/等
            ((>= author-count 3)
             (let ((first (get-author-display 1)))
               (if chinese?
                   (gbt-add-suffix `(concat ,first "<#7b49>") field-type chinese? author-count)
                   (gbt-add-suffix `(concat ,first " et al.") field-type chinese? author-count))))
            ;; 作者数 = 2：显示两位，用"and"或"和"连接
            ((= author-count 2)
             (let ((first (get-author-display 1))
                   (second (get-author-display 2)))
               (if chinese?
                   (gbt-add-suffix `(concat ,first "<#548C>" ,second) field-type chinese? author-count)
                   (gbt-add-suffix `(concat ,first " & " ,second) field-type chinese? author-count))))
            ;; 作者数 = 1：显示一位
            ((= author-count 1)
             (gbt-add-suffix (get-author-display 1) field-type chinese? author-count))
            ;; 其他情况（应该不会发生）
            (else "")))
        "")))

;; 获取年份字符串（带后缀）
(tm-define (gbt-get-year-string x)
  (:mode bib-gbt7714-2015-author-year?)
  (let* ((key (list-ref x 2))
         (y (bib-field x "year"))
         (year-base-expr (cond ((bib-null? y) "?")
                               ((string? y) y)
                               ((not y) "?")
                               (else (safe-tm->string y))))
         (year-base (cond ((string? year-base-expr) year-base-expr)
                          ((not year-base-expr) "?")
                          (else (safe-tm->string year-base-expr))))
         (suffix (if gbt-year-suffix-table
                     (or (ahash-ref gbt-year-suffix-table key) "")
                     "")))
    (if (equal? suffix "")
        year-base
        (string-append year-base suffix))))

;; 标签格式
(tm-define (bib-format-bibitem n x)
  (:mode bib-gbt7714-2015-author-year?)
  ;; 返回作者(年份)格式用于cite-author-year包
  (let ((author-str (gbt-get-author*-string x))
        (year-str (gbt-get-year-string x)))
    `(bibitem* (natbib-triple ,(gbt-get-author-string x)
                              ,author-str
                              ,year-str))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 辅助函数
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 将URL里rsub表达式转换回下划线：将(rsub "x")转换为_x
(define (convert-rsub-to-underscore expr)
  (cond
    ((and (list? expr) (= (length expr) 2) (equal? (car expr) 'rsub))
     (let ((text (cadr expr)))
       (if (string? text)
           (string-append "_" text)
           (string-append "_" (safe-tm->string text)))))
    ((and (list? expr) (>= (length expr) 1) (equal? (car expr) 'concat))
     `(concat ,@(map convert-rsub-to-underscore (cdr expr))))
    ((list? expr)
     (map convert-rsub-to-underscore expr))
    ((string? expr) expr)
    (else expr)))


;; 通用版本格式化函数
(tm-define (gbt-format-edition x chinese?)
  (:mode bib-gbt7714-2015-author-year?)
  (let ((edition-field (bib-field x "edition")))
    (if (or (bib-null? edition-field) (equal? edition-field ""))
        ""
        (if chinese?
            `(concat "<#7B2C>" ,edition-field "<#7248>")  ;; 第X版
            `(concat ,edition-field " edition")))))

;; 子字符串检查函数
(define (substring? pat text)
  (let ((pat-len (string-length pat))
        (text-len (string-length text)))
    (let outer ((i 0))
      (cond
        ((> (+ i pat-len) text-len) #f)
        (else
         (let inner ((j 0))
           (cond
             ((>= j pat-len) #t)
             ((char=? (string-ref pat j)
                      (string-ref text (+ i j)))
              (inner (+ j 1)))
             (else (outer (+ i 1))))))))))

;; 检测字符串是否包含中文字符的辅助函数
(define (contains-chinese? str)
  (if (bib-null? str) #f
      (let ((s (if (string? str) str "")))
        ;; 检查是否包含中文 Unicode 字符的内部表示模式
        (or (substring? "<#4" s)
            (substring? "<#5" s)
            (substring? "<#6" s)
            (substring? "<#7" s)
            (substring? "<#8" s)
            (substring? "<#9" s)
            ;; 检查实际的中文字符（基本中文范围 U+4E00-U+9FFF）
            (let loop ((i 0) (len (string-length s)))
              (if (>= i len) #f
                  (let* ((c (string-ref s i))
                         (code (char->integer c)))
                    (if (and (>= code #x4e00) (<= code #x9fff))
                        #t
                        (loop (+ i 1) len)))))))))

;; 检测作者列表是否包含中文作者
(define (authors-contain-chinese? a)
  (if (or (bib-null? a) (nlist? a)) #f
      (let loop ((i 1) (n (length a)))
        (if (>= i n) #f
            (let* ((author-raw (list-ref a i))
                   (author (gbt-remove-keepcase author-raw))
                   ;; 将作者的所有字符串部分连接成一个字符串，然后检查是否包含中文
                   (author-str (cond
                                 ((string? author) author)
                                 ((list? author)
                                  (let part-loop ((j 1) (m (length author)) (result ""))
                                    (if (>= j m) result
                                        (let ((part (list-ref author j)))
                                          (part-loop (+ j 1) m
                                                     (if (string? part)
                                                         (string-append result part)
                                                         result))))))
                                 (else ""))))
              (if (contains-chinese? author-str)
                  #t
                  (loop (+ i 1) n)))))))

;; 移除keepcase标签的辅助函数
(define (gbt-remove-keepcase x)
  (cond
    ((list? x)
     (if (not (null? x))
         (if (equal? (car x) 'keepcase)
             (gbt-remove-keepcase (cadr x))
             (cons (car x) (map gbt-remove-keepcase (cdr x))))
         '()))
    ((string? x) x)
    (else x)))

;; 作者列表格式
(tm-define (bib-format-names a)
  (:mode bib-gbt7714-2015-author-year?)
  (if (or (bib-null? a) (nlist? a))
      ""
      (let* ((n (length a))
             (chinese? (authors-contain-chinese? a))
             ;; GBT 7714-2015: 最多显示3个作者
             (max-authors 3)
             (author-count (- n 1))
             (show-count (min author-count max-authors))
             ;; 逗号分隔符：中文不加空格，英文加空格
             (comma-sep (if chinese? "," ", ")))
        (cond
          ((equal? author-count 1)
           (bib-format-name (list-ref a 1)))
          (else
           (let* ((first (bib-format-name (list-ref a 1)))
                  (has-more (> author-count max-authors))
                  ;; 收集中间作者
                  ;; 如果有更多作者（>3）：收集第2到第3个作者（共2个中间作者）
                  ;; 如果没有更多作者（<=3）：收集第2到第author-count-1个作者
                  (middle (let loop ((i 2) (result ""))
                            (if (or (>= i n)
                                    (if has-more
                                        (> i max-authors)
                                        (>= i author-count)))
                                result
                                (loop (+ i 1)
                                      (if (equal? result "")
                                          (bib-format-name (list-ref a i))
                                          `(concat ,result ,comma-sep ,(bib-format-name (list-ref a i))))))))
                  (last-part (if has-more
                                 (if chinese? "<#7b49>" "et al") ;;等
                                 (if (>= author-count 2)
                                     (bib-format-name (list-ref a (- n 1)))
                                     "")))
                  ;; 分隔符：无论是否有更多作者，都使用逗号分隔符
                  ;; 中文：作者1,作者2,作者3,等
                  ;; 英文：Author1, Author2, Author3, et al
                  (separator comma-sep))
             (cond
               ((and (equal? middle "") (equal? last-part "")) first)
               ((equal? middle "") `(concat ,first ,separator ,last-part))
               ((equal? last-part "") `(concat ,first ,comma-sep ,middle))
               (else `(concat ,first ,comma-sep ,middle ,separator ,last-part)))))))))

;; 编者格式
(tm-define (bib-format-editor x)
  (:mode bib-gbt7714-2015-author-year?)
  ;; 格式化编者字段，添加 ed. 或 eds. 后缀（中文为“编”或“主编”）
  (let* ((e (bib-field x "editor"))
         (chinese? (if (or (bib-null? e) (nlist? e)) #f (authors-contain-chinese? e))))
    (if (or (bib-null? e) (nlist? e))
        ""
        (let* ((names (bib-format-names e))
               (count (- (length e) 1)))
          (if chinese?
              (if (= count 1)
                  `(concat ,names ",<#7F16>") ;;编
                  `(concat ,names ",<#4E3B><#7F16>")) ;;主编
              (if (= count 1)
                  `(concat ,names ", ed.")
                  `(concat ,names ", eds.")))))))

;; 文献类型标识符函数
(tm-define (bib-document-type-identifier x type)
  (:mode bib-gbt7714-2015-author-year?)
  ;; 根据文献类型和是否有在线访问信息返回标识符
  ;; 优先使用note字段，如果note字段包含标识符
  ;; 否则检查是否有url或doi字段来判断是否为在线文献
  (let* ((note (bib-field x "note"))
         (has-url (not (bib-null? (bib-field x "url"))))
         (has-doi (not (bib-null? (bib-field x "doi"))))
         (online (or has-url has-doi)))
    (cond
      ((and (not (bib-null? note)) (not (equal? note ""))) note)  ;; 优先使用note字段
      ((equal? type "article") (if online "[J/OL]" "[J]"))         ;; 期刊!
      ((equal? type "book") (if online "[M/OL]" "[M]"))            ;; 普通图书!
      ((equal? type "inbook") (if online "[M/OL]" "[M]"))          ;; 析出图书!
      ((equal? type "inproceedings") (if online "[C/OL]" "[C]"))   ;; 会议录!
      ((equal? type "proceedings") (if online "[C/OL]" "[C]"))     ;; 会议录!
      ((equal? type "phdthesis") (if online "[D/OL]" "[D]"))       ;; 学位论文-博士!
      ((equal? type "mastersthesis") (if online "[D/OL]" "[D]"))   ;; 学位论文-硕士!
      ((equal? type "techreport") (if online "[R/OL]" "[R]"))      ;; 报告!
      ((equal? type "collection") (if online "[G/OL]" "[G]"))      ;; 汇编!
      ((equal? type "incollection") (if online "[G/OL]" "[G]"))      ;; 析出汇编!
      ((equal? type "manual") (if online "[M/OL]" "[M]"))          ;; 手册/说明书!
      ((equal? type "standard") (if online "[S/OL]" "[S]"))        ;; 标准!
      ((equal? type "patent") (if online "[P/OL]" "[P]"))          ;; 专利!
      ((equal? type "database") (if online "[DB/OL]" "[DB]"))      ;; 数据库!
      ((equal? type "software") (if online "[CP/OL]" "[CP]"))      ;; 计算机程序!
      ((equal? type "program") (if online "[CP/OL]" "[CP]"))       ;; 计算机程序!
      ((equal? type "online") (if online "[EB/OL]" "[EB]"))        ;; 电子公告!
      ((equal? type "electronic") (if online "[EB/OL]" "[EB]"))    ;; 电子公告!
      ((equal? type "archive") (if online "[A/OL]" "[A]"))         ;; 档案!
      ((equal? type "map") (if online "[CM/OL]" "[CM]"))           ;; 舆图!
      ((equal? type "dataset") (if online "[DS/OL]" "[DS]"))       ;; 数据集!
      ((equal? type "newspaper") (if online "[N/OL]" "[N]"))       ;; 报纸!
      ((equal? type "misc") (if online "[Z/OL]" "[Z]"))            ;; 其他!
      ((equal? type "other") (if online "[Z/OL]" "[Z]"))           ;; 其他!
      (else ""))))

;; 地址:机构格式
(tm-define (bib-format-address-institution x)
  (:mode bib-gbt7714-2015-author-year?)
  (let* ((addr (cond
                 ((not (bib-empty? x "address")) (bib-field x "address"))
                 ((not (bib-empty? x "location")) (bib-field x "location"))
                 (else "")))
         (inst-field (cond
                       ((not (bib-empty? x "school")) (bib-field x "school"))
                       ((not (bib-empty? x "organization")) (bib-field x "organization"))
                       ((not (bib-empty? x "publisher")) (bib-field x "publisher"))
                       ((not (bib-empty? x "institution")) (bib-field x "institution"))
                       (else "")))
         (inst-val (if (or (bib-null? inst-field) (equal? inst-field "")) "" inst-field)))
    (cond
      ((and (not (equal? addr "")) (not (equal? inst-val "")))
       `(concat ,addr ": " ,inst-val))
      ((not (equal? addr "")) addr)
      ((not (equal? inst-val "")) inst-val)
      (else ""))))

;; 作者姓名格式
(tm-define (bib-format-name x)
  (:mode bib-gbt7714-2015-author-year?)
  ;; 西文作者：姓在前（全大写），名缩写（如 "YU H B"）
  ;; 中文作者：姓在前，名在后（全称）
  ;; 使用双层花括号的机构名：不进行格式化处理
  (let* ((first-name-raw (if (bib-null? (list-ref x 1)) "" (list-ref x 1)))
         (last-name-raw (if (bib-null? (list-ref x 3)) "" (list-ref x 3)))
         (chinese? (or (contains-chinese? first-name-raw) (contains-chinese? last-name-raw)))
         (first-name (if chinese?
                         first-name-raw  ;; 中文名不缩写
                         (if (bib-null? first-name-raw) "" (bib-abbreviate first-name-raw "" " "))))
         (last-name (if (bib-null? last-name-raw) "" (if chinese? (bib-purify last-name-raw) (string-upcase (bib-purify last-name-raw))))))
    ;; 西文姓名处理：如果没有逗号分隔，保持原样
    (if (not chinese?)
        (cond
          ((bib-null? last-name-raw) first-name-raw)   ;; 没有姓，返回名
          ((bib-null? first-name-raw) last-name-raw)   ;; 没有名，返回姓
          (else `(concat ,last-name " " ,first-name))) ;; 正常格式化
        ;; 中文姓名处理：保持原有逻辑
        (if (bib-null? last-name)
            first-name
            (if (bib-null? first-name)
                last-name
                `(concat ,last-name " " ,first-name))))))

;; 书名格式
(tm-define (bib-format-in-ed-booktitle x)
  (:mode bib-gbt7714-2015-author-year?)
  (let* ((b (bib-default-field x "booktitle"))
         (e (bib-field x "editor")))
    (if (bib-null? b)
        ""
        `(concat ,(bib-translate "in ") ,b))))

;; 卷(期):页码格式
(tm-define (bib-format-vol-num-pages x)
  (:mode bib-gbt7714-2015-author-year?)
  ;; GBT 7714-2015-author-year 格式：卷(期):页码（年份在标签中）
  (let* ((v (bib-field x "volume"))
         (n (bib-field x "number"))
         (p (bib-field x "pages"))
         (vol (if (bib-null? v) "" v))
         (num (if (bib-null? n) "" `(concat "(" ,n ")")))
         (pag (if (or (bib-null? p) (nlist? p))
                  ""
                  (cond
                    ((equal? 1 (length p)) "")
                    ((equal? 2 (length p)) `(concat ": " ,(list-ref p 1)))
                    (else
                     `(concat ": " ,(list-ref p 1)
                              ,bib-range-symbol ,(list-ref p 2)))))))
    `(concat ,vol ,num ,pag)))

;; URL/DOI 信息格式
(tm-define (bib-format-url-doi x)
  (:mode bib-gbt7714-2015-author-year?)
  (let* ((url-raw (bib-field x "url"))
         (doi-raw (bib-field x "doi"))
         ;; 转换rsub表达式为下划线
         (url (convert-rsub-to-underscore url-raw))
         (doi (convert-rsub-to-underscore doi-raw))
         (has-url (not (bib-null? url-raw)))
         (has-doi (not (bib-null? doi-raw))))
    (cond
      (has-doi
       ;; 有 DOI（优先使用，忽略 URL）：添加https://doi.org/前缀
       `(concat "https://doi.org/" ,doi))
      (has-url
       ;; 只有 URL（没有 DOI）
       `(concat ,url))
      (else ""))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 类型格式函数
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 重写文章格式以添加文献类型标识符 [J]
(tm-define (bib-format-article n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "article")))
         ,(bib-new-block
            (bib-new-sentence
              `(,(bib-format-field x "journal")
                ,(bib-format-vol-num-pages x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写图书格式以添加文献类型标识符 [M]
(tm-define (bib-format-book n x)
  (:mode bib-gbt7714-2015-author-year?)
  (let ((chinese? (authors-contain-chinese?
                   (if (bib-empty? x "editor")
                       (bib-field x "author")
                       (bib-field x "editor")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             (bib-new-sentence
              `((concat ,(bib-format-field x "title")
                        ,(bib-document-type-identifier x "book")))))
           ,(bib-new-block
             (let ((edition-str (gbt-format-edition x chinese?)))
               (if (equal? edition-str "") "" edition-str)))
           ,(bib-new-block
             (bib-new-sentence
              `(,(bib-format-number-series x)
                ,(bib-format-address-institution x))))
           ,(bib-new-case-preserved-block (bib-format-url-doi x)))))))

;; 重写析出图书格式以添加文献类型标识符 [M]
(tm-define (bib-format-inbook n x)
  (:mode bib-gbt7714-2015-author-year?)
  (let ((chinese? (authors-contain-chinese?
                   (if (bib-empty? x "editor")
                       (bib-field x "author")
                       (bib-field x "editor")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             (let* ((bookauthor-field (bib-field x "bookauthor"))
                    (editor-field (bib-field x "editor"))
                    (booktitle-field (bib-field x "booktitle"))
                    (edition-str (gbt-format-edition x chinese?))
                    (has-bookauthor (not (bib-null? bookauthor-field)))
                    (has-editor (not (bib-null? editor-field))))
               (cond
                 (has-bookauthor
                  (let ((bookauthor-names (cond
                                            ((bib-null? bookauthor-field) "")
                                            ((nlist? bookauthor-field) bookauthor-field)
                                            (else (bib-format-names bookauthor-field)))))
                    `(concat ,(bib-format-field-preserve-case x "title")
                             ,(bib-document-type-identifier x "book")
                             "//"
                             ,bookauthor-names ". "
                             ,(bib-format-field-preserve-case x "booktitle")
                             ,(if (equal? edition-str "") "" `(concat ": " ,edition-str))
                             ".")))
                 (has-editor
                  (let ((editor-names (cond
                                        ((bib-null? editor-field) "")
                                        ((nlist? editor-field) editor-field)
                                        (else (bib-format-names editor-field)))))
                    `(concat ,(bib-format-field-preserve-case x "title")
                             ,(bib-document-type-identifier x "book")
                             "//"
                             ,editor-names ". "
                             ,(bib-format-field-preserve-case x "booktitle")
                             ,(if (equal? edition-str "") "" `(concat ": " ,edition-str))
                             ".")))
                 (else
                  `(concat ,(bib-format-field-preserve-case x "title")
                           ,(bib-document-type-identifier x "book")
                           "//. "
                           ,(bib-format-field-preserve-case x "booktitle")
                           ,(if (equal? edition-str "") "" `(concat ": " ,edition-str))
                           ".")))))
           ,(bib-new-block
             (bib-new-sentence
              `(,(bib-format-number-series x)
                ,(bib-format-address-institution x))))
           ,(bib-new-case-preserved-block (bib-format-url-doi x)))))))

;; 重写会议论文格式以添加文献类型标识符 [C]
(tm-define (bib-format-inproceedings n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
    ,(bib-format-bibitem n x)
    ,(bib-label (list-ref x 2))
    ,(bib-new-list-spc
      `(,(bib-new-block
              `(concat ,(bib-format-field-preserve-case x "title")
                       ,(bib-document-type-identifier x "inproceedings")
                       "//"
                       ,(bib-format-field-preserve-case x "booktitle")
                       "."))
        ,(bib-new-block
          (bib-new-sentence
           `(,(bib-format-address-institution x))))
        ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写会议录格式以添加文献类型标识符 [C]
(tm-define (bib-format-proceedings n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "proceedings")))
         ,(bib-new-block
           (bib-new-sentence
            `(,(bib-format-address-institution x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写手册格式以添加文献类型标识符 [S]
(tm-define (bib-format-manual n x)
  (:mode bib-gbt7714-2015-author-year?)
  (let ((chinese? (authors-contain-chinese?
                   (if (bib-empty? x "author")
                       '()
                       (bib-field x "author")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             (let* ((title (bib-format-field-preserve-case x "title"))
                    (number (bib-field x "number"))
                    (edition-str (gbt-format-edition x chinese?))
                    (identifier (bib-document-type-identifier x "manual"))
                    (parts (list title)))
               (if (not (equal? edition-str ""))
                   (set! parts (append parts (list ": " edition-str))))
               (if (not (bib-null? number))
                   (set! parts (append parts (list ": " number))))
               (set! parts (append parts (list identifier)))
               (apply tmconcat parts)))
           ,(bib-new-block
             (bib-new-sentence
              (if (bib-empty? x "author")
                  (let ((address-institution (bib-format-address-institution x)))
                    (if (equal? address-institution "") '() `(,address-institution)))
                  `(,(bib-format-address-institution x)))))
           ,(bib-new-case-preserved-block (bib-format-url-doi x)))))))

;; 重写博士论文格式以添加文献类型标识符 [D]
(tm-define (bib-format-phdthesis n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "phdthesis")))
         ,(bib-new-block
           (bib-new-sentence
            `(,(if (bib-empty? x "type")
                   (bib-format-field-Locase x "type"))
              ,(bib-format-address-institution x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写硕士论文格式以添加文献类型标识符 [D]
(tm-define (bib-format-mastersthesis n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "mastersthesis")))
         ,(bib-new-block
           (bib-new-sentence
            `(,(if (bib-empty? x "type")
                   (bib-format-field-Locase x "type"))
              ,(bib-format-address-institution x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写报告格式以添加文献类型标识符 [R]
(tm-define (bib-format-techreport n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "techreport")))
         ,(bib-new-block
           (bib-new-sentence
            `(,(bib-format-address-institution x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写杂项格式以添加文献类型标识符 [Z]
(tm-define (bib-format-misc n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "misc")))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写专利格式以添加文献类型标识符 [P]
(tm-define (bib-format-patent n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             (let* ((title (bib-format-field-preserve-case x "title"))
                    (number (bib-field x "number"))
                    (identifier (bib-document-type-identifier x "patent")))
               (if (bib-null? number)
                   `(concat ,title ,identifier)
                   `(concat ,title ": " ,number ,identifier))))
           ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写标准格式以添加文献类型标识符 [S]
(tm-define (bib-format-standard n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           (let* ((title (bib-format-field-preserve-case x "title"))
                  (number (bib-field x "number"))
                  (identifier (bib-document-type-identifier x "standard")))
             (if (bib-null? number)
                 `(concat ,title ,identifier)
                 `(concat ,title ": " ,number ,identifier))))
         ,(bib-new-block
           (bib-new-sentence
            (let ((address-institution (bib-format-address-institution x)))
              (if (equal? address-institution "") '() `(,address-institution)))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写数据库格式以添加文献类型标识符 [DB]
(tm-define (bib-format-database n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "database")))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写电子公告格式以添加文献类型标识符 [EB]
(tm-define (bib-format-electronic n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "electronic")))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写在线网页格式以添加文献类型标识符 [EB]
(tm-define (bib-format-online n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             `(concat ,(bib-format-field-preserve-case x "title")
                      ,(bib-document-type-identifier x "online")))
           ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写报纸格式以添加文献类型标识符 [N]
(tm-define (bib-format-newspaper n x)
  (:mode bib-gbt7714-2015-author-year?)
  (let* ((p (bib-field x "pages"))
         (pag (if (or (bib-null? p) (nlist? p))
                  ""
                  (cond
                    ((equal? 1 (length p)) "")
                    ((equal? 2 (length p)) (list-ref p 1))
                    (else `(concat ,(list-ref p 1)
                                   ,bib-range-symbol ,(list-ref p 2))))))
         (date-pages-str (if (== pag "")
                             ""
                             `(concat "(" ,pag ")"))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             `(concat ,(bib-format-field-preserve-case x "title")
                      ,(bib-document-type-identifier x "newspaper")))
           ,(bib-new-block
             (bib-new-sentence
              `(,(bib-format-field x "journal")
                ,date-pages-str)))
           ,(bib-new-case-preserved-block (bib-format-url-doi x)))))))

;; 重写汇编格式以添加文献类型标识符 [G]
(tm-define (bib-format-collection n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "collection")))
         ,(bib-new-block
           (bib-new-sentence
            `(,(bib-format-address-institution x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写析出汇编格式以添加文献类型标识符 [G]
(tm-define (bib-format-incollection n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
               (let* ((editor-field (bib-field x "editor"))
                      (booktitle-field (bib-field x "booktitle"))
                      (has-editor (not (bib-null? editor-field))))
                 (if has-editor
                     (let ((editor-names (cond
                                           ((bib-null? editor-field) "")
                                           ((nlist? editor-field) editor-field)
                                           (else (bib-format-names editor-field)))))
                       `(concat ,(bib-format-field-preserve-case x "title")
                                ,(bib-document-type-identifier x "collection")
                                "//"
                                ,editor-names ". "
                                ,(bib-format-field-preserve-case x "booktitle")
                                "."))
                     `(concat ,(bib-format-field-preserve-case x "title")
                              ,(bib-document-type-identifier x "collection")
                              "//"
                              ,(bib-format-field-preserve-case x "booktitle")
                              "."))))
         ,(bib-new-block
           (bib-new-sentence
            `(,(bib-format-address-institution x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写软件格式以添加文献类型标识符 [CP]
(tm-define (bib-format-software n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "software")))
         ,(bib-new-block
           (bib-new-sentence
            (let ((version (bib-field x "version")))
              (if (bib-null? version) '() `((concat "Version " ,version))))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写程序格式以添加文献类型标识符 [CP]
(tm-define (bib-format-program n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "program")))
         ,(bib-new-block
           (bib-new-sentence
            (let ((version (bib-field x "version")))
              (if (bib-null? version) '() `((concat "Version " ,version))))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写档案格式以添加文献类型标识符 [A]
(tm-define (bib-format-archive n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           (let* ((title (bib-format-field-preserve-case x "title"))
                  (number (bib-field x "number"))
                  (identifier (bib-document-type-identifier x "archive")))
             (if (bib-null? number)
                 `(concat ,title ,identifier)
                 `(concat ,title ": " ,number ,identifier))))
         ,(bib-new-block
           (bib-new-sentence
            `(,(bib-format-address-institution x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写舆图格式以添加文献类型标识符 [CM]
(tm-define (bib-format-map n x)
  (:mode bib-gbt7714-2015-author-year?)
  (let ((chinese? (authors-contain-chinese?
                   (if (bib-empty? x "editor")
                       (bib-field x "author")
                       (bib-field x "editor")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             `(concat ,(bib-format-field-preserve-case x "title")
                      ,(bib-document-type-identifier x "map")))
           ,(bib-new-block
             (let ((edition-str (gbt-format-edition x chinese?)))
               (if (equal? edition-str "") "" edition-str)))
           ,(bib-new-block
             (bib-new-sentence
              `(,(bib-format-address-institution x))))
           ,(bib-new-case-preserved-block (bib-format-url-doi x)))))))

;; 重写数据集格式以添加文献类型标识符 [DS]
(tm-define (bib-format-dataset n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "dataset")))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))

;; 重写其他格式以添加文献类型标识符 [Z]
(tm-define (bib-format-other n x)
  (:mode bib-gbt7714-2015-author-year?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "other")))
         ,(bib-new-block
           (bib-new-sentence
            `(,(bib-format-address-institution x))))
         ,(bib-new-case-preserved-block (bib-format-url-doi x))))))
