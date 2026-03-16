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
(load "./TeXmacs/plugins/html/progs/data/html.scm")

(check-set-mode! 'report-failed)

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

