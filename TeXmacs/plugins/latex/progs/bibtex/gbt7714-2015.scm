
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : gbt7714-2015.scm
;; DESCRIPTION : GBT 7714-2015 style for BibTeX files
;; COPYRIGHT   : (C) 2025 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (bibtex gbt7714-2015)
  (:use (bibtex bib-utils) (bibtex plain)))

(bib-define-style "gbt7714-2015" "gbt7714-2015")

(tm-define (bib-sorted-entries l)
  (:mode bib-gbt7714-2015?)
  l)

;; 重写条目格式函数以支持所有文献类型
(tm-define (bib-format-entry n x)
  (:mode bib-gbt7714-2015?)
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
;; 辅助函数
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 将URL里rsub表达式转换回下划线：将(rsub "x")转换为_x
(define (convert-rsub-to-underscore expr)
  (cond
    ((and (list? expr) (= (length expr) 2) (equal? (car expr) 'rsub))
     (let ((text (cadr expr)))
       (if (string? text)
           (string-append "_" text)
           (string-append "_" (tm->string text)))))
    ((and (list? expr) (>= (length expr) 1) (equal? (car expr) 'concat))
     `(concat ,@(map convert-rsub-to-underscore (cdr expr))))
    ((list? expr)
     (map convert-rsub-to-underscore expr))
    ((string? expr) expr)
    (else expr)))

;; 检查是否有URL/DOI/urldate
(tm-define (gbt-has-url-doi? x)
  (:mode bib-gbt7714-2015?)
  (let* ((url (bib-field x "url"))
         (doi (bib-field x "doi"))
         (urldate (bib-field x "urldate")))
    (or (not (bib-null? url)) (not (bib-null? doi)) (not (bib-null? urldate)))))

;; 智能句子函数：如果有URL/DOI/urldate则不添加句点，否则添加句点
(tm-define (gbt-new-smart-sentence x ref)
  (:mode bib-gbt7714-2015?)
  (if (gbt-has-url-doi? ref)
      (bib-upcase-first (bib-new-list ", " x))
      (bib-add-period (bib-upcase-first (bib-new-list ", " x)))))

;; 智能块函数：包装智能句子，包含URL/DOI，不添加额外空格
(tm-define (gbt-new-smart-block-with-url x ref)
  (:mode bib-gbt7714-2015?)
  (if (bib-null? x) ""
      `(concat ,(gbt-new-smart-sentence x ref)
               ,(bib-new-case-preserved-block (bib-format-url-doi ref)))))

;; 通用版本格式化函数
(tm-define (gbt-format-edition x chinese?)
  (:mode bib-gbt7714-2015?)
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
  (:mode bib-gbt7714-2015?)
  (if (or (bib-null? a) (nlist? a))
      ""
      (let* ((n (length a))
             (chinese? (authors-contain-chinese? a))
             ;; GBT 7714-2015: 最多显示3个作者
             (max-authors 3)
             (author-count (- n 1))
             (show-count (min author-count max-authors))
             ;; 逗号分隔符：中文和英文都加空格
             (comma-sep (if chinese? ", " ", ")))
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
  (:mode bib-gbt7714-2015?)
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
  (:mode bib-gbt7714-2015?)
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
  (:mode bib-gbt7714-2015?)
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
  (:mode bib-gbt7714-2015?)
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

;; 日期格式
(tm-define (bib-format-date x)
  (:mode bib-gbt7714-2015?)
  ;; 日期处理函数，支持date字段和year字段
  ;; date字段加括号，year字段不加括号
  ;; 如果有pages字段，则格式为"年份:页码"
  (let* ((d (bib-field x "date"))
         (y (bib-field x "year"))
         (p (bib-field x "pages")))
    (cond
      ((not (bib-null? d)) `(concat "(" ,d ")"))
      ((not (bib-null? y))
       (let* ((year y)
              (pag (if (or (bib-null? p) (nlist? p))
                       ""
                       (cond
                         ((equal? 1 (length p)) "")
                         ((equal? 2 (length p)) `(concat ": " ,(list-ref p 1)))
                         (else
                          `(concat ": " ,(list-ref p 1)
                                   ,bib-range-symbol ,(list-ref p 2)))))))
         (if (== pag "")
             year
             `(concat ,year ,pag))))
      (else ""))))

;; 书名格式
(tm-define (bib-format-in-ed-booktitle x)
  (:mode bib-gbt7714-2015?)
  (let* ((b (bib-default-field x "booktitle"))
         (e (bib-field x "editor")))
    (if (bib-null? b)
        ""
        `(concat ,(bib-translate "in ") ,b))))

;; 年份,卷(期):页码格式
(tm-define (bib-format-vol-num-pages x)
  (:mode bib-gbt7714-2015?)
  ;; GBT 7714-2015 格式：年份,卷(期):页码
  (let* ((y (bib-field x "year"))
         (v (bib-field x "volume"))
         (n (bib-field x "number"))
         (p (bib-field x "pages"))
         (year (if (bib-null? y) "" y))
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
    (if (and (== vol "") (== num "") (== pag ""))
        year
        (if (bib-null? year)
            `(concat ,vol ,num ,pag)
            (if (not (== vol ""))
                `(concat ,year ", " ,vol ,num ,pag)
                `(concat ,year ,num ,pag))))))

;; 数字标签格式
(tm-define (bib-format-bibitem n x)
  (:mode bib-gbt7714-2015?)
  ;; 使用数字标签，如 [1], [2], ...
  `(bibitem* ,(number->string n)))

;; URL/DOI 信息格式
(tm-define (bib-format-url-doi x)
  (:mode bib-gbt7714-2015?)
  (let* ((url-raw (bib-field x "url"))
         (doi-raw (bib-field x "doi"))
         (urldate (bib-field x "urldate"))
         ;; 转换rsub表达式为下划线
         (url (convert-rsub-to-underscore url-raw))
         (doi (convert-rsub-to-underscore doi-raw))
         (has-url (not (bib-null? url-raw)))
         (has-doi (not (bib-null? doi-raw)))
         (has-urldate (not (bib-null? urldate))))
    (cond
      (has-doi
       ;; 有 DOI（优先使用，忽略 URL）：添加https://doi.org/前缀
       (let ((doi-url `(concat "https://doi.org/" ,doi)))
         (if has-urldate
             `(concat "[" ,urldate "]. " ,doi-url)
             doi-url)))
      (has-url
       ;; 只有 URL（没有 DOI）
       (if has-urldate
           `(concat "[" ,urldate "]. " ,url)
           `(concat ,url)))
      (else ""))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 类型格式函数
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 重写文章格式以添加文献类型标识符 [J]
(tm-define (bib-format-article n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "article")))
         ,(gbt-new-smart-block-with-url
           `(,(bib-format-field x "journal")
             ,(bib-format-vol-num-pages x)) x)))))

;; 重写图书格式以添加文献类型标识符 [M]
(tm-define (bib-format-book n x)
  (:mode bib-gbt7714-2015?)
  (let ((chinese? (authors-contain-chinese?
                   (if (bib-empty? x "editor")
                       (bib-field x "author")
                       (bib-field x "editor")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             (if (bib-empty? x "editor")
                 (bib-format-author x)
                 (bib-format-editor x)))
           ,(bib-new-block
             (bib-new-sentence
              `((concat ,(bib-format-field x "title")
                        ,(bib-document-type-identifier x "book")))))
           ,(bib-new-block
             (let ((edition-str (gbt-format-edition x chinese?)))
               (if (equal? edition-str "") "" edition-str)))
           ,(gbt-new-smart-block-with-url
             `(,(bib-format-number-series x)
               ,(bib-format-address-institution x)
               ,(bib-format-date x)) x))))))

;; 重写析出图书格式以添加文献类型标识符 [M]
(tm-define (bib-format-inbook n x)
  (:mode bib-gbt7714-2015?)
  (let ((chinese? (authors-contain-chinese?
                   (if (bib-empty? x "editor")
                       (bib-field x "author")
                       (bib-field x "editor")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             (if (bib-empty? x "editor")
                 (bib-format-author x)
                 (bib-format-editor x)))
           ,(bib-new-block
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
           ,(gbt-new-smart-block-with-url
             `(,(bib-format-number-series x)
               ,(bib-format-address-institution x)
               ,(bib-format-date x)) x))))))

;; 重写会议论文格式以添加文献类型标识符 [C]
(tm-define (bib-format-inproceedings n x)
  (:mode bib-gbt7714-2015?)
  `(concat
    ,(bib-format-bibitem n x)
    ,(bib-label (list-ref x 2))
    ,(bib-new-list-spc
      `(,(bib-new-block (bib-format-author x))
        ,(bib-new-block
              `(concat ,(bib-format-field-preserve-case x "title")
                       ,(bib-document-type-identifier x "inproceedings")
                       "//"
                       ,(bib-format-field-preserve-case x "booktitle")
                       "."))
        ,(gbt-new-smart-block-with-url
          `(,(bib-format-address-institution x)
            ,(bib-format-date x)) x)))))

;; 重写会议录格式以添加文献类型标识符 [C]
(tm-define (bib-format-proceedings n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-editor x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "proceedings")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-address-institution x)
                 ,(bib-format-date x)) x)))))

;; 重写手册格式以添加文献类型标识符 [M]
(tm-define (bib-format-manual n x)
  (:mode bib-gbt7714-2015?)
  (let ((chinese? (authors-contain-chinese?
                   (if (bib-empty? x "author")
                       '()
                       (bib-field x "author")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block
             (if (bib-empty? x "author")
                 (bib-new-sentence `(,(bib-format-address-institution x)))
                 (bib-format-author x)))
           ,(bib-new-block
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
           ,(gbt-new-smart-block-with-url
               (if (bib-empty? x "author")
                   (let ((address-institution (bib-format-address-institution x))
                         (date (bib-format-date x)))
                     (cond
                       ((and (equal? address-institution "") (bib-null? date)) '())
                       ((equal? address-institution "") `(,date))
                       ((bib-null? date) `(,address-institution))
                       (else `(,address-institution ,date))))
                   `(,(bib-format-address-institution x)
                     ,(bib-format-date x))) x))))))

;; 重写博士论文格式以添加文献类型标识符 [D]
(tm-define (bib-format-phdthesis n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "phdthesis")))
         ,(gbt-new-smart-block-with-url
               `(,(if (bib-empty? x "type")
                      (bib-format-field-Locase x "type"))
                 ,(bib-format-address-institution x)
                 ,(bib-format-date x)) x)))))

;; 重写硕士论文格式以添加文献类型标识符 [D]
(tm-define (bib-format-mastersthesis n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "mastersthesis")))
         ,(gbt-new-smart-block-with-url
               `(,(if (bib-empty? x "type")
                      (bib-format-field-Locase x "type"))
                 ,(bib-format-address-institution x)
                 ,(bib-format-date x)) x)))))

;; 重写报告格式以添加文献类型标识符 [R]
(tm-define (bib-format-techreport n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "techreport")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-address-institution x)
                 ,(let ((date-str (bib-format-date x))
                        (number (bib-field x "number")))
                    (if (bib-null? number)
                        date-str
                        (if (equal? date-str "")
                            number
                            `(concat ,date-str ": " ,number))))) x)))))

;; 重写杂项格式以添加文献类型标识符 [Z]
(tm-define (bib-format-misc n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "misc")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-date x)) x)))))

;; 重写专利格式以添加文献类型标识符 [P]
(tm-define (bib-format-patent n x)
  (:mode bib-gbt7714-2015?)
  (let ((date-str (let ((d (bib-field x "date"))
                        (y (bib-field x "year")))
                    (cond
                      ((not (bib-null? d)) d)
                      ((not (bib-null? y)) y)
                      (else "")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block (bib-format-author x))
           ,(bib-new-block
             (let* ((title (bib-format-field-preserve-case x "title"))
                    (number (bib-field x "number"))
                    (identifier (bib-document-type-identifier x "patent")))
               (if (bib-null? number)
                   `(concat ,title ,identifier)
                   `(concat ,title ": " ,number ,identifier))))
           ,(gbt-new-smart-block-with-url
                 `(,date-str) x))))))

;; 重写标准格式以添加文献类型标识符 [S]
(tm-define (bib-format-standard n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
           (let* ((title (bib-format-field-preserve-case x "title"))
                  (key (bib-field x "key"))
                  (number (bib-field x "number"))
                  (std-num (if (bib-null? key) number key))
                  (identifier (bib-document-type-identifier x "standard")))
             (if (bib-null? std-num)
                 `(concat ,title ,identifier)
                 `(concat ,std-num " " ,title ,identifier))))
         ,(gbt-new-smart-block-with-url
               (let ((address-institution (bib-format-address-institution x))
                     (date (bib-format-date x)))
                 (cond
                   ((and (equal? address-institution "") (bib-null? date)) '())
                   ((equal? address-institution "") `(,date))
                   ((bib-null? date) `(,address-institution))
                   (else `(,address-institution ,date)))) x)))))

;; 重写数据库格式以添加文献类型标识符 [DB]
(tm-define (bib-format-database n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "database")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-date x)) x)))))

;; 重写电子公告格式以添加文献类型标识符 [EB]
(tm-define (bib-format-electronic n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "electronic")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-date x)) x)))))

;; 重写在线网页格式以添加文献类型标识符 [EB]
(tm-define (bib-format-online n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "online")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-date x)) x)))))

;; 重写报纸格式以添加文献类型标识符 [N]
(tm-define (bib-format-newspaper n x)
  (:mode bib-gbt7714-2015?)
  (let* ((date-str (let ((d (bib-field x "date"))
                         (y (bib-field x "year")))
                     (cond
                       ((not (bib-null? d)) d)
                       ((not (bib-null? y)) y)
                       (else ""))))
         (p (bib-field x "pages"))
         (pag (if (or (bib-null? p) (nlist? p))
                  ""
                  (cond
                    ((equal? 1 (length p)) "")
                    ((equal? 2 (length p)) (list-ref p 1))
                    (else `(concat ,(list-ref p 1)
                                   ,bib-range-symbol ,(list-ref p 2))))))
         (date-pages-str (if (== pag "")
                             date-str
                             `(concat ,date-str "(" ,pag ")"))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block (bib-format-author x))
           ,(bib-new-block
             `(concat ,(bib-format-field-preserve-case x "title")
                      ,(bib-document-type-identifier x "newspaper")))
           ,(gbt-new-smart-block-with-url
                 `(,(bib-format-field x "journal")
                   ,date-pages-str) x))))))

;; 重写汇编格式以添加文献类型标识符 [G]
(tm-define (bib-format-collection n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-editor x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "collection")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-address-institution x)
                 ,(bib-format-date x)) x)))))

;; 重写析出汇编格式以添加文献类型标识符 [G]
(tm-define (bib-format-incollection n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block
            (if (bib-empty? x "author")
                (bib-format-editor x)
                (bib-format-author x)))
         ,(bib-new-block
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
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-address-institution x)
                 ,(bib-format-date x)) x)))))

;; 重写软件格式以添加文献类型标识符 [CP]
(tm-define (bib-format-software n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "software")))
         ,(gbt-new-smart-block-with-url
               (let ((version (bib-field x "version"))
                     (date (bib-format-date x)))
                 (cond
                   ((and (bib-null? version) (bib-null? date)) '())
                   ((bib-null? version) `(,date))
                   (else `((concat "Version " ,version) ,date)))) x)))))

;; 重写程序格式以添加文献类型标识符 [CP]
(tm-define (bib-format-program n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "program")))
         ,(gbt-new-smart-block-with-url
               (let ((version (bib-field x "version"))
                     (date (bib-format-date x)))
                 (cond
                   ((and (bib-null? version) (bib-null? date)) '())
                   ((bib-null? version) `(,date))
                   (else `((concat "Version " ,version) ,date)))) x)))))

;; 重写档案格式以添加文献类型标识符 [A]
(tm-define (bib-format-archive n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           (let* ((title (bib-format-field-preserve-case x "title"))
                  (number (bib-field x "number"))
                  (identifier (bib-document-type-identifier x "archive")))
             (if (bib-null? number)
                 `(concat ,title ,identifier)
                 `(concat ,title ": " ,number ,identifier))))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-address-institution x)
                 ,(bib-format-date x)) x)))))

;; 重写舆图格式以添加文献类型标识符 [CM]
(tm-define (bib-format-map n x)
  (:mode bib-gbt7714-2015?)
  (let ((chinese? (authors-contain-chinese?
                   (if (bib-empty? x "editor")
                       (bib-field x "author")
                       (bib-field x "editor")))))
    `(concat
       ,(bib-format-bibitem n x)
       ,(bib-label (list-ref x 2))
       ,(bib-new-list-spc
         `(,(bib-new-block (bib-format-author x))
           ,(bib-new-block
             `(concat ,(bib-format-field-preserve-case x "title")
                      ,(bib-document-type-identifier x "map")))
           ,(bib-new-block
             (let ((edition-str (gbt-format-edition x chinese?)))
               (if (equal? edition-str "") "" edition-str)))
           ,(gbt-new-smart-block-with-url
                 `(,(bib-format-address-institution x)
                   ,(bib-format-date x)) x))))))

;; 重写数据集格式以添加文献类型标识符 [DS]
(tm-define (bib-format-dataset n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "dataset")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-date x)) x)))))

;; 重写其他格式以添加文献类型标识符 [Z]
(tm-define (bib-format-other n x)
  (:mode bib-gbt7714-2015?)
  `(concat
     ,(bib-format-bibitem n x)
     ,(bib-label (list-ref x 2))
     ,(bib-new-list-spc
       `(,(bib-new-block (bib-format-author x))
         ,(bib-new-block
           `(concat ,(bib-format-field-preserve-case x "title")
                    ,(bib-document-type-identifier x "other")))
         ,(gbt-new-smart-block-with-url
               `(,(bib-format-address-institution x)
                 ,(bib-format-date x)) x)))))
