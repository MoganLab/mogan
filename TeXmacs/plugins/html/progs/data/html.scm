
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-html.scm
;; DESCRIPTION : setup html converters
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data html))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Html
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define html-detected-limit 1000)

;; 按行分割文本
(define (html-string-split-lines s)
  (let ((len (if (>= (string-length s) html-detected-limit) html-detected-limit (string-length s))))
    (let loop ((i 0)
              (start 0)
              (result '()))
      (cond ((>= i len)
             (reverse (cons (substring s start i) result)))
            ((char=? (string-ref s i) #\newline)
             (loop (+ i 1)
                   (+ i 1)
                   (cons (substring s start i) result)))
            (else (loop (+ i 1) start result))))))

;; 某个字符在文本中的含量
(define (character-from-string s ch)
  (if (not (string-null? s)) 
      (let* ((len (string-length s))
             (limit (if (>= len html-detected-limit) html-detected-limit len)))
        (let loop ((ref 0)
                   (count 0))
          (if (>= ref limit)
              (/ count len)
              (loop (+ ref 1)
                    (if (char=? (string-ref s ref) ch)
                        (+ count 1)
                        count)))))
      #f))

;; 计算一个子串在文本中的含量，计算的是子串的字符数，而不是个数
(define (html-string-count-substring s sub)
    (let ((sub-len (string-length sub)))
      (if (zero? sub-len)
          0
          (let loop ((i 0)
                     (count 0))
            (if (>= i (- (string-length s) sub-len -1))
                count
                (if (string=? (substring s i (+ i sub-len)) sub)
                    (loop (+ i sub-len) (+ count 1))
                    (loop (+ i 1) count)))))))

;; < 和 > 的含量
(define (html-angle-bracket-density s)
    (if (string-null? s)
        0
        (let* ((len (string-length s))
               (limit (if (>= len html-detected-limit) html-detected-limit len))
               (substr (substring s 0 limit)))
          (/ (+ (character-from-string substr #\<)
                (character-from-string substr #\>))
             len))))

;; 完整的tag子串在文本中的字符含量
(define (html-tag-density s)
    (if (string-null? s)
        0
        (let* ((len (string-length s))
               (limit (if (>= len html-detected-limit) html-detected-limit len))
               (substr (substring s 0 limit))
               (lc-substr (string-downcase substr)))
          (let ((count (+ (html-string-count-substring lc-substr "<div")
                         (html-string-count-substring lc-substr "<span")
                         (html-string-count-substring lc-substr "<p")
                         (html-string-count-substring lc-substr "<a")
                         (html-string-count-substring lc-substr "<img")
                         (html-string-count-substring lc-substr "<ul")
                         (html-string-count-substring lc-substr "<ol")
                         (html-string-count-substring lc-substr "<li")
                         (html-string-count-substring lc-substr "<table")
                         (html-string-count-substring lc-substr "<tr")
                         (html-string-count-substring lc-substr "<td")
                         (html-string-count-substring lc-substr "<th")
                         (html-string-count-substring lc-substr "<h1")
                         (html-string-count-substring lc-substr "<h2")
                         (html-string-count-substring lc-substr "<h3")
                         (html-string-count-substring lc-substr "<h4")
                         (html-string-count-substring lc-substr "<h5")
                         (html-string-count-substring lc-substr "<h6")
                         (html-string-count-substring lc-substr "<form")
                         (html-string-count-substring lc-substr "<input")
                         (html-string-count-substring lc-substr "<button")
                         (html-string-count-substring lc-substr "<textarea")
                         (html-string-count-substring lc-substr "<select")
                         (html-string-count-substring lc-substr "<option")
                         (html-string-count-substring lc-substr "<style")
                         (html-string-count-substring lc-substr "<script")
                         (html-string-count-substring lc-substr "<meta")
                         (html-string-count-substring lc-substr "<link")
                         (html-string-count-substring lc-substr "</div")
                         (html-string-count-substring lc-substr "</ul")
                         (html-string-count-substring lc-substr "</ol")
                         (html-string-count-substring lc-substr "</table")
                         (html-string-count-substring lc-substr "</tr")
                         (html-string-count-substring lc-substr "</form")
                         (html-string-count-substring lc-substr "</style")
                         (html-string-count-substring lc-substr "</script"))))
            (/ count len)))))

;; = 和 " 的含量
(define (html-attribute-density s)
    (if (string-null? s)
        0
        (let* ((len (string-length s))
               (limit (if (>= len html-detected-limit) html-detected-limit len))
               (substr (substring s 0 limit)))
          (/ (+ (character-from-string substr #\=)
                (character-from-string substr #\"))
             len))))

;; 这一行文本是否包含html标签
(define (html-line-contains-features? line)
    (let ((lc-line (string-downcase line)))
      (or
       (> (html-string-count-substring lc-line "<div") 0)
       (> (html-string-count-substring lc-line "<span") 0)
       (> (html-string-count-substring lc-line "<p") 0)
       (> (html-string-count-substring lc-line "<a") 0)
       (> (html-string-count-substring lc-line "<img") 0)
       (> (html-string-count-substring lc-line "<ul") 0)
       (> (html-string-count-substring lc-line "<ol") 0)
       (> (html-string-count-substring lc-line "<li") 0)
       (> (html-string-count-substring lc-line "<table") 0)
       (> (html-string-count-substring lc-line "<tr") 0)
       (> (html-string-count-substring lc-line "<td") 0)
       (> (html-string-count-substring lc-line "<th") 0)
       (> (html-string-count-substring lc-line "<h1") 0)
       (> (html-string-count-substring lc-line "<h2") 0)
       (> (html-string-count-substring lc-line "<h3") 0)
       (> (html-string-count-substring lc-line "<h4") 0)
       (> (html-string-count-substring lc-line "<h5") 0)
       (> (html-string-count-substring lc-line "<h6") 0)
       (> (html-string-count-substring lc-line "</div") 0)
       (> (html-string-count-substring lc-line "</span") 0)
       (> (html-string-count-substring lc-line "</p") 0)
       (> (html-string-count-substring lc-line "</a") 0)
       (> (html-string-count-substring lc-line "/>") 0)
       (> (html-string-count-substring lc-line "<!doctype") 0)
       (> (html-string-count-substring lc-line "<?xml") 0))))

;; 计算存在html特征的行的含量
(define (html-feature-line-density s)
    (let ((lines (html-string-split-lines s)))
      (if (null? lines)
          0
          (let loop ((remaining lines)
                     (count 0)
                     (total 0))
            (if (null? remaining)
                (if (> total 0) (/ count total) 0)
                (let ((line (car remaining)))
                  (loop (cdr remaining)
                        (if (html-line-contains-features? line) (+ count 1) count)
                        (+ total 1))))))))

;; 计算div标签的平衡性
(define (html-structure-balanced? s)
    (let* ((lc-s (string-downcase s))
           (open-tags (html-string-count-substring lc-s "<div"))
           (close-tags (html-string-count-substring lc-s "</div")))
      ;; div 的开标签与闭标签数量差小于2
      (and (> open-tags 0) (> close-tags 0) (<= (abs (- open-tags close-tags)) 2))))

;; 短字符串的特殊检测
(define (determine-short-html-string s)
    (let* ((len (string-length s)))
      (cond
        ((or
          (and (> (character-from-string s #\<) 0)
               (> (character-from-string s #\>) 0)
               (> (html-string-count-substring s "</") 0))
          (> (html-string-count-substring (string-downcase s) "class=") 0)
          (> (html-string-count-substring (string-downcase s) "id=") 0)
          (> (html-string-count-substring (string-downcase s) "style=") 0)
          (> (html-string-count-substring (string-downcase s) "href=") 0)
          (> (html-string-count-substring (string-downcase s) "src=") 0))
         #t)
        ((>= (html-angle-bracket-density s) 0.03) #t)
        (else #f))))

(define (is-short-html-string? s)
  (if (<= (string-length s) 100)
      (determine-short-html-string s)
      #f))

 (define (is-html-string? s)
    (let* ((angle-density (html-angle-bracket-density s))
           (tag-density (html-tag-density s))
           (attr-density (html-attribute-density s))
           (feature-line-density (html-feature-line-density s))
           (balanced? (html-structure-balanced? s)))
      (cond
        ;; High confidence: clear HTML structure
        ;; < > 含量，标签含量，特征行含量
        ((and (>= angle-density 0.02)
              (>= tag-density 0.01)
              (>= feature-line-density 0.25))
         #t)
        ;; Medium confidence: good angle bracket density with either tags or attributes
        ;; 
        ((and (>= angle-density 0.015)
              (or (>= tag-density 0.005)
                  (>= attr-density 0.01))
              (>= feature-line-density 0.15))
         #t)
        ;; Lower confidence: balanced structure with some HTML features
        ((and balanced?
              (>= angle-density 0.01)
              (>= feature-line-density 0.10))
         #t)
        ;; Very high angle bracket density (likely HTML/XML)
        ((>= angle-density 0.03) #t)
        (else #f))))

(define (html-recognizes-at? s pos)
  (set! pos (format-skip-spaces s pos))
  (cond ((format-test? s pos "<html") #t)
        ((format-test? s pos "<xhtml") #t)
        ((format-test? s pos "<body") #t)
        ((format-test? s pos "<title") #t)
        ((format-test? s pos "<!doctype html") #t)
        ((format-test? s pos "<math") #t)
        ((format-test? s pos "<table") #t)
        ((format-test? s pos "<p>") #t)
        ((format-test? s pos "<div") #t)
        ((format-test? s pos "<span") #t)
        ((format-test? s pos "<a ") #t)
        ((format-test? s pos "<img") #t)
        ((format-test? s pos "<ul") #t)
        ((format-test? s pos "<ol") #t)
        ((format-test? s pos "<li") #t)
        ((format-test? s pos "<h1") #t)
        ((format-test? s pos "<h2") #t)
        ((format-test? s pos "<h3") #t)
        ((format-test? s pos "<h4") #t)
        ((format-test? s pos "<h5") #t)
        ((format-test? s pos "<h6") #t)
        ((format-test? s pos "<form") #t)
        ((format-test? s pos "<input") #t)
        ((format-test? s pos "<button") #t)
        ((format-test? s pos "<textarea") #t)
        ((format-test? s pos "<select") #t)
        ((format-test? s pos "<option") #t)
        ((format-test? s pos "<style") #t)
        ((format-test? s pos "<script") #t)
        ((format-test? s pos "<meta") #t)
        ((format-test? s pos "<link") #t)
        ((format-test? s pos "<!--") #t)
        ((format-test? s pos "<?xml ")
         (html-recognizes-at? s (format-skip-line s pos)))
        ((format-test? s pos "<!doctype ")
         (html-recognizes-at? s (format-skip-line s pos)))
        ((is-short-html-string? s) #t)
        ((is-html-string? s) #t)
        (else #f)))

(define (html-recognizes? s)
  (and (string? s) (html-recognizes-at? s 0)))

(define-format html
  (:name "Html")
  (:suffix "html" "xhtml" "htm")
  (:recognize html-recognizes?)
  (:option "mathml->texmacs:latex-annotations" "off"))

(lazy-define (convert html htmltm) parse-html-snippet)
(lazy-define (convert html htmltm) parse-html-document)
(lazy-define (convert html htmltm) html->texmacs)
(lazy-define (convert html htmlout) serialize-html)
(lazy-define (convert html tmhtml) texmacs->html)

(converter html-document html-stree
  (:function parse-html-document))

(converter html-stree html-document
  (:function serialize-html))

(converter html-snippet html-stree
  (:function parse-html-snippet))

(converter html-stree html-snippet
  (:function serialize-html))

(converter html-stree texmacs-stree
  (:function html->texmacs))

(converter texmacs-stree html-stree
  (:function-with-options texmacs->html)
  (:option "texmacs->html:css" "on")
  (:option "texmacs->html:mathjax" "off")
  (:option "texmacs->html:mathml" "on")
  (:option "texmacs->html:images" "off")
  (:option "texmacs->html:css-stylesheet" "---"))
