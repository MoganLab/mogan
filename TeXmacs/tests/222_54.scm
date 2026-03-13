;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 222_54.scm
;; DESCRIPTION : Unit tests for enhanced HTML format detection
;; COPYRIGHT   : (C) 2026 Mingshen Chu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; ============================================================================
;; The dependent function
;; ============================================================================

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

;; ============================================================================
;; Test cases for HTML format detection
;; ============================================================================

;; Should be detected as HTML

;; Full HTML documents
(define html-text1 "<!DOCTYPE html>\n<html>\n<head>\n  <title>Test Page</title>\n</head>\n<body>\n  <h1>Hello World</h1>\n  <p>This is a test 
paragraph.</p>\n</body>\n</html>")

(define html-text2 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" 
\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<head>\n  <title>XHTML Document</title>\n</head>\n<body>\n  <p>This is 
XHTML.</p>\n</body>\n</html>")

;; HTML fragments
(define html-text3 "<div class=\"container\">\n  <h2>Section Title</h2>\n  <p>Some content here.</p>\n  <ul>\n    <li>Item 1</li>\n    <li>Item 2</li>\n    <li>Item 3</li>\n 
</ul>\n</div>")

(define html-text4 "<table border=\"1\">\n  <tr>\n    <th>Header 1</th>\n    <th>Header 2</th>\n  </tr>\n  <tr>\n    <td>Cell 1</td>\n    <td>Cell 2</td>\n  </tr>\n</table>")

(define html-text5 "<form action=\"/submit\" method=\"post\">\n  <label for=\"name\">Name:</label>\n  <input type=\"text\" id=\"name\" name=\"name\">\n  <br>\n  <input 
type=\"submit\" value=\"Submit\">\n</form>")

;; HTML with inline styles and scripts
(define html-text6 "<style>\n  body { font-family: Arial, sans-serif; }\n  .highlight { background-color: yellow; }\n</style>\n<script>\n  console.log('Hello from 
JavaScript');\n</script>")

;; Short HTML snippets
(define html-text7 "<p>This is a paragraph.</p>")

(define html-text8 "<a href=\"https://example.com\">Click here</a>")

(define html-text9 "<img src=\"image.jpg\" alt=\"Sample image\" width=\"100\" height=\"100\">")

(define html-text10 "<span style=\"color: red;\">Red text</span>")

;; HTML with MathML
(define html-text11 "<math xmlns=\"http://www.w3.org/1998/Math/MathML\">\n  <mrow>\n    <mi>x</mi>\n    <mo>=</mo>\n    <mfrac>\n      <mrow>\n        <mo>-</mo>\n        
<mi>b</mi>\n        <mo>±</mo>\n        <msqrt>\n          <mrow>\n            <msup>\n              <mi>b</mi>\n              <mn>2</mn>\n            </msup>\n            
<mo>-</mo>\n            <mn>4</mn>\n            <mi>a</mi>\n            <mi>c</mi>\n          </mrow>\n        </msqrt>\n      </mrow>\n      <mrow>\n        <mn>2</mn>\n    
    <mi>a</mi>\n      </mrow>\n    </mfrac>\n  </mrow>\n</math>")

;; HTML with mixed content
(define html-text12 "<div>\n  <h3>Mixed Content</h3>\n  <p>This paragraph contains <strong>bold text</strong> and <em>italic text</em>.</p>\n  <p>Here's a <a 
href=\"#\">link</a> and an <img src=\"icon.png\" alt=\"icon\"> image.</p>\n</div>")

;; HTML with comments
(define html-text13 "<!-- This is an HTML comment -->\n<div>\n  <!-- Main content starts here -->\n  <p>Visible content</p>\n  <!-- Main content ends here -->\n</div>")

;; HTML with data attributes
(define html-text14 "<div data-id=\"123\" data-type=\"widget\" data-config='{\"color\": \"blue\"}'>\n  Custom widget\n</div>")

;; HTML with aria attributes
(define html-text15 "<button aria-label=\"Close\" aria-expanded=\"false\">X</button>")

;; Should NOT be detected as HTML

;; Plain text
(define non-html-text1 "This is plain text without any HTML tags.")

(define non-html-text2 "Hello, world! This is a simple sentence.")

;; Markdown text
(define non-html-text3 "# Markdown Title\n\nThis is a paragraph in Markdown.\n\n- List item 1\n- List item 2\n- List item 3")

(define non-html-text4 "**Bold text** and *italic text* with `inline code`.")

;; LaTeX text
(define non-html-text5 "\\documentclass{article}\n\\begin{document}\n\\section{Introduction}\nThis is a LaTeX document.\n\\end{document}")

;; JSON text
(define non-html-text6 "{\n  \"name\": \"John Doe\",\n  \"age\": 30,\n  \"city\": \"New York\"\n}")

;; XML (non-HTML)
(define non-html-text7 "<?xml version=\"1.0\"?>\n<config>\n  <server>\n    <host>localhost</host>\n    <port>8080</port>\n  </server>\n</config>")

;; Code (Python)
(define non-html-text8 "def hello_world():\n    print(\"Hello, World!\")\n    return True")

;; Code (JavaScript)
(define non-html-text9 "function calculateSum(a, b) {\n    return a + b;\n}\n\nconsole.log(calculateSum(5, 3));")

;; CSV data
(define non-html-text10 "Name,Age,City\nJohn,30,New York\nJane,25,London\nBob,35,Tokyo")

;; Email addresses and URLs (without tags)
(define non-html-text11 "Contact us at info@example.com or visit https://example.com")

;; File paths
(define non-html-text12 "C:\\Users\\Name\\Documents\\file.txt\n/home/user/projects/src/main.py")

;; Edge cases

;; Text with angle brackets but not HTML
(define non-html-text13 "x < y and y > z")  ; Mathematical inequalities

(define non-html-text14 "5 < 10 > 3")  ; More inequalities

;; Text with quotes and equals but not HTML
(define non-html-text15 "name=\"John\" age=30 city=\"NYC\"")  ; Looks like attributes but no tags

;; Text with very low HTML feature density
(define non-html-text16 "This is a long text document with many paragraphs. It contains some special characters like < and > and = and \" but they are not used in HTML 
context. The document continues for many lines to ensure it's long enough for statistical analysis.")



;; ============================================================================
;; Test function
;; ============================================================================

(define (test-html-format-determine)

  ;; Should be detected as HTML
  (display "Testing HTML detection (should return #t):\n")
  (check (html-recognizes-at? html-text1 0) => #t)
  (check (html-recognizes-at? html-text2 0) => #t)
  (check (html-recognizes-at? html-text3 0) => #t)
  (check (html-recognizes-at? html-text4 0) => #t)
  (check (html-recognizes-at? html-text5 0) => #t)
  (check (html-recognizes-at? html-text6 0) => #t)
  (check (html-recognizes-at? html-text7 0) => #t)
  (check (html-recognizes-at? html-text8 0) => #t)
  (check (html-recognizes-at? html-text9 0) => #t)
  (check (html-recognizes-at? html-text10 0) => #t)
  (check (html-recognizes-at? html-text11 0) => #t)
  (check (html-recognizes-at? html-text12 0) => #t)
  (check (html-recognizes-at? html-text13 0) => #t)
  (check (html-recognizes-at? html-text14 0) => #t)
  (check (html-recognizes-at? html-text15 0) => #t)

  ;; Should NOT be detected as HTML
  (display "\nTesting non-HTML detection (should return #f):\n")
  (check (html-recognizes-at? non-html-text1 0) => #f)
  (check (html-recognizes-at? non-html-text2 0) => #f)
  (check (html-recognizes-at? non-html-text3 0) => #f)
  (check (html-recognizes-at? non-html-text4 0) => #f)
  (check (html-recognizes-at? non-html-text5 0) => #f)
  (check (html-recognizes-at? non-html-text6 0) => #f)
  (check (html-recognizes-at? non-html-text7 0) => #f)
  (check (html-recognizes-at? non-html-text8 0) => #f)
  (check (html-recognizes-at? non-html-text9 0) => #f)
  (check (html-recognizes-at? non-html-text10 0) => #f)
  (check (html-recognizes-at? non-html-text11 0) => #f)
  (check (html-recognizes-at? non-html-text12 0) => #f)
  (check (html-recognizes-at? non-html-text13 0) => #f)
  (check (html-recognizes-at? non-html-text14 0) => #f)
  (check (html-recognizes-at? non-html-text15 0) => #f)
  (check (html-recognizes-at? non-html-text16 0) => #f))

(tm-define (test_222_54)
  (test-html-format-determine)
  (check-report))

