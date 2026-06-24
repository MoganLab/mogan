;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 222_22.scm
;; DESCRIPTION : Unit tests for parse-latex (multi-integral and big operator import)
;; COPYRIGHT   : (C) 2026 Mingshen Chu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define text1 (string-append "\\documentclass[12pt]{article}\n"
                             "\\usepackage{amsmath}\n"
                             "\\begin{document}\n"
                             "\\section{引言}\n"
                             "这是LaTeX文档。\n"
                             "\\end{document}"))

(define text2 "$E = mc^2$")

(define text3 "\\int_{0}^{\\infty} e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}")

(define text4 "\\[f(x) = \\sum_{n=0}^{\\infty} \\frac{f^{(n)}(a)}{n!}(x-a)^n\\]")

(define text5 (string-append "\\begin{equation}\n"
                             "\\label{eq:1}\n"
                             "\\nabla \\cdot \\mathbf{E} = \\frac{\\rho}{\\varepsilon_0}\n"
                             "\\end{equation}"))

(define text6 (string-append "\\begin{align}\n"
                             "(a+b)^2 &= a^2 + 2ab + b^2 \\ \n"
                             "(a-b)^2 &= a^2 - 2ab + b^2\n"
                             "\\end{align}"))                          

(define text7 (string-append "\\begin{pmatrix}\n"
                             "a & b \\ \n"
                             "c & d\n"
                             "\\end{pmatrix}"))

(define text8 "根据文献\\cite{knuth84}可知，算法复杂度为$O(n\\log n)$。")

(define text9 (string-append "\\usepackage[utf8]{inputenc}\n"
                             "\\usepackage[T1]{fontenc}"))

(define text10 (string-append "\\newcommand{\\vect}[1]{\\mathbf{#1}}\n"
                              "\\renewcommand{\\epsilon}{\\varepsilon}"))                            

(define text11 (string-append "\\chapter{绪论}\n"
                              "\\section{研究背景}\n"
                              "\\subsection{国内外研究现状}"))                              

(define text12 (string-append "\\begin{itemize}\n"
                              "\\item 第一项\n"
                              "\\item 第二项\n"
                              "\\item 第三项\n"
                              "\\end{itemize}"))

(define text13 (string-append "\\begin{tabular}{|c|c|}\n"
                              "\\hline\n"
                              "姓名 & 分数 \\ \n"
                              "\\hline\n"
                              "张三 & 95 \\ \n"
                              "李四 & 88 \\ \n"
                              "\\hline\n"
                              "\\end{tabular}"))                                          

(define text14 (string-append "\\begin{figure}[ht]\n"
                              "\\centering\n"
                              "\\includegraphics[width=0.8\\textwidth]{figure.png}\n"
                              "\\caption{示例图片}\n"
                              "\\label{fig:sample}\n"
                              "\\end{figure}"))                              

(define text15 (string-append "正文内容\\footnote{这是脚注内容}继续正文。\n"
                              "\\marginpar{边注内容}"))

(define text16 "\\mathbb{R}, \\mathcal{F}, \\mathfrak{g}, \\mathrm{d}x")

(define text17 "\\frac{\\partial^2 f}{\\partial x^2} + \\sqrt[3]{x+y}")

(define text18 (string-append "\\begin{theorem}[勾股定理]\n"
                              "直角三角形斜边平方等于两直角边平方和。\n"
                              "\\end{theorem}\n"
                              "\\begin{proof}\n"
                              "由几何关系易证。\n"
                              "\\end{proof}"))

(define text19 (string-append "使用\\verb|\\begin{document}|开始文档。\n"
                              "\\begin{verbatim}\n"
                              "#include <stdio.h>\n"
                              "int main() { return 0; }\n"
                              "\\end{verbatim}"))                            

(define text20 (string-append "\\documentclass[UTF8]{ctexart}\n"
                              "\\begin{document}\n"
                              "\\section{中文标题}\n"
                              "这是一个中文\\LaTeX 文档，包含公式 $\\alpha + \\beta = \\gamma$。\n"
                              "\\end{document}"))

(define text21 (string-append "要匹配反斜杠：\\ 需要\\\\ \n"
                              "路径：C:\\Users\\Name\\Files\n"
                              "特殊字符：\n \t \r \""))

(define text22 (string-append "@echo off\n"
                              "set PATH=C:\\Program Files\\App\n"
                              "if exist \"file.txt\" (\n"
                              "   echo Found: %~dp0\\file.txt\n"
                              ")"))

(define text23 (string-append "char* path = \"C:\\Users\\Name\";\n"
                              "char* regex = \"\\d+\\.\\d+\";\n"
                              "printf(\"Line: %s\\n\", str);"))

(define text24 (string-append "# 标题\n"
                              "这是Markdown文档，但包含LaTeX公式：\n"
                              "$$\n"
                              "f(x) = \\int_{-\\infty}^{\\infty} \\hat{f}(\\xi) e^{2\\pi i \\xi \\nx} d\\xi"
                              "$$\n"
                              "更多文本..."))

(define text25 (string-append "# 这个函数实现以下公式：\n"
                              "# \\frac{d}{dx}f(x) = \\lim_{h\\to 0}\\frac{f(x+h)-f(x)}{h}\n"
                              "def derivative(f, x, h=1e-5):\n"
                                  "return (f(x+h) - f(x)) / h\n"))

(define (string-split-lines s)
  (let ((len (if (>= (string-length s) 1000) 1000 (string-length s))))
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

(define (backslash-from-string s)
  (if (not (string-null? s)) 
      (let* ((len (string-length s))
            (limit (if (>= len 1000) 1000 len)))
        (let loop ((ref 0)
                  (count 0))
          (if (>= ref limit)
            (/ count len)
            (loop (+ ref 1)
                  (if (char=? (string-ref s ref) #\\)
                      (+ count 1)
                      count)))))
      #f))

(define (backslash-line-from-string s)
  (let ((lines (string-split-lines s)))
    (if (null? lines)
      0
      (let loop ((count-lines 0)
                 (count 0)
                 (remaining-lines lines))
        (if (null? remaining-lines)
            (if (> count-lines 0)
                (/ count count-lines)
                0)
            (let ((line (car remaining-lines))) 
              (loop (+ count-lines 1)
                (if (string-contains? line "\\")
                    (+ count 1)
                    count)
                (cdr remaining-lines))))))))

(define (parentheses-from-string s)
  (if (not (string-null? s)) 
      (let* ((len (string-length s))
            (limit (if (>= len 1000) 1000 len)))
        (let loop ((ref 0)
                  (count 0))
          (if (>= ref limit)
            (/ count len)
            (loop (+ ref 1)
                  (if (or 
                        (char=? (string-ref s ref) (string-ref "{" 0)) 
                        (char=? (string-ref s ref) (string-ref "}" 0)))
                      (+ count 1)
                      count)))))
      #f))

(define (determine-short-string s)
  (let* ((len (string-length s)))
    (cond ((and (> len 2)
                (char=? (string-ref s 0) #\$) 
                (char=? (string-ref s (- len 1)) #\$)) 
          #t)
          ((>= (backslash-from-string s) 0.02) #t)
          (else #f))))

(define (is-short-latex-string? s)
  (if (<= (string-length s) 50)
      (determine-short-string s)
      #f))

(define (is-latex-string? s)
  (let ((percent-slash (backslash-from-string s)))
    (if (and (>= percent-slash 0.04) 
             (<= percent-slash 0.25))
        (let ((percent-parentheses (parentheses-from-string s)))
          (if (>= percent-parentheses 0.04)
              (let ((percent-backslash-line (backslash-line-from-string s)))
                (if (>= percent-backslash-line 0.25)
                    #t
                    #f))
              #f))
        #f)))

(define (latex-recognizes-at? s pos)
  (set! pos (format-skip-spaces s pos))
  (cond ((format-test? s pos "\\document") #t)
        ((format-test? s pos "\\documentclass") #t)
        ((format-test? s pos "\\usepackage") #t)
        ((format-test? s pos "\\title") #t)
        ((format-test? s pos "\\newcommand") #t)
        ((format-test? s pos "\\input") #t)
        ((format-test? s pos "\\includeonly") #t)
        ((format-test? s pos "\\chapter") #t)
        ((format-test? s pos "\\appendix") #t)
        ((format-test? s pos "\\section") #t)
        ((format-test? s pos "\\footnote") #t)
        ((format-test? s pos "\\marginpar") #t)
        ((format-test? s pos "\\begin") #t)
        ((format-test? s pos "\\end") #t)
        ((format-test? s pos "\\begin{") #t)
        ((format-test? s pos "\\end{") #t)
        ((format-test? s pos "\\alpha") #t)
        ((format-test? s pos "\\beta") #t)
        ((format-test? s pos "\\gamma") #t)
        ((format-test? s pos "\\ref") #t)
        ((format-test? s pos "\\textbf") #t)
        ((format-test? s pos "\\textit") #t)
        ((format-test? s pos "\\mathbb") #t)
        ((format-test? s pos "\\mathcal") #t)
        ((format-test? s pos "\\frac") #t)
        ((format-test? s pos "\\cite") #t)
        ((format-test? s pos "\\item") #t)
        ((format-test? s pos "\\[") #t)
        ((format-test? s pos "\\(") #t)
        ((is-short-latex-string? s) #t)
        ((is-latex-string? s) #t)
        (else #f)))

(define (test-latex-format-determine)
  (check (latex-recognizes-at? text1 0)
         => #t)
  (check (latex-recognizes-at? text2 0)
         => #t)
  (check (latex-recognizes-at? text3 0)
         => #t)
  (check (latex-recognizes-at? text4 0)
         => #t)
  (check (latex-recognizes-at? text5 0)
         => #t)
  (check (latex-recognizes-at? text6 0)
         => #t)
  (check (latex-recognizes-at? text7 0)
         => #t)
  (check (latex-recognizes-at? text8 0)
         => #f)
  (check (latex-recognizes-at? text9 0)
         => #t)
  (check (latex-recognizes-at? text10 0)
         => #t)
  (check (latex-recognizes-at? text11 0)
         => #t)
  (check (latex-recognizes-at? text12 0)
         => #t)
  (check (latex-recognizes-at? text13 0)
         => #t)
  (check (latex-recognizes-at? text14 0)
         => #t)
  (check (latex-recognizes-at? text15 0)
         => #f)
  (check (latex-recognizes-at? text16 0)
         => #t)
  (check (latex-recognizes-at? text17 0)
         => #t)
  (check (latex-recognizes-at? text18 0)
         => #t)
  (check (latex-recognizes-at? text19 0)
         => #f)
  (check (latex-recognizes-at? text20 0)
         => #t)
  (check (latex-recognizes-at? text21 0)
         => #f)
  (check (latex-recognizes-at? text22 0)
         => #f)
  (check (latex-recognizes-at? text23 0)
         => #f)
  (check (latex-recognizes-at? text24 0)
         => #f)
  (check (latex-recognizes-at? text25 0)
         => #f))

(tm-define (test_222_25)
  (test-latex-format-determine)
  (check-report))
  