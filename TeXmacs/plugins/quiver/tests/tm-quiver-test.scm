;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-quiver-test.scm
;; DESCRIPTION : Quiver Binary plugin unit tests
;; COPYRIGHT   : (C) 2026 (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (scheme base)
  (liii check)
  (liii string)
  (liii list)
  (liii path)
)

; Simulate the wrap-quiver-code logic from tm-quiver.scm
(define (string-trim-both s)
  (string-trim-right (string-trim s)))

(define (strip-math-delimiters str)
  (let* ((s (string-trim-both str))
         (len (string-length s)))
    (cond ((and (>= len 4)
                (string-starts? s "\\[")
                (string-ends? s "\\]"))
           (strip-math-delimiters (substring s 2 (- len 2))))
          ((and (>= len 4)
                (string-starts? s "$$")
                (string-ends? s "$$"))
           (strip-math-delimiters (substring s 2 (- len 2))))
          ((and (>= len 2)
                (string-starts? s "$")
                (string-ends? s "$"))
           (strip-math-delimiters (substring s 1 (- len 1))))
          ((and (>= len 32)
                (string-starts? s "\\begin{equation*}")
                (string-ends? s "\\end{equation*}"))
           (strip-math-delimiters (substring s 17 (- len 15))))
          ((and (>= len 30)
                (string-starts? s "\\begin{equation}")
                (string-ends? s "\\end{equation}"))
           (strip-math-delimiters (substring s 16 (- len 14))))
          ((and (>= len 36)
                (string-starts? s "\\begin{displaymath}")
                (string-ends? s "\\end{displaymath}"))
           (strip-math-delimiters (substring s 19 (- len 17))))
          (else s))))

(define (wrap-quiver-code raw-code)
  (let* ((code (strip-math-delimiters raw-code))
         (trimmed (string-trim-left code)))
    (if (string-starts? trimmed "\\documentclass")
        code
        (let* ((lines (string-split code #\newline))
               (library-lines
                 (filter (lambda (line)
                           (string-starts? (string-trim-left line) "\\usetikzlibrary"))
                         lines))
               (package-lines
                 (filter (lambda (line)
                           (string-starts? (string-trim-left line) "\\usepackage"))
                         lines))
               (other-lines
                 (filter (lambda (line)
                           (let ((trimmed-line (string-trim-left line)))
                             (and (not (string-null? trimmed-line))
                                  (not (string-starts? trimmed-line "\\usetikzlibrary"))
                                  (not (string-starts? trimmed-line "\\usepackage")))))
                         lines))
               (body (string-join other-lines "\n"))
               (body-trimmed (string-trim-left body)))
          (let* ((has-tikzcd? (string-starts? body-trimmed "\\begin{tikzcd}"))
                 (inner-code
                   (if (or (string-null? body-trimmed)
                           has-tikzcd?)
                       body
                       (string-append "\\begin{tikzcd}[nodes in empty cells]\n" body "\n\\end{tikzcd}"))))
            (string-append
              "\\documentclass[tikz]{standalone}\n"
              "\\usepackage{tikz-cd}\n"
              "\\usepackage{amssymb}\n"
              "\\usetikzlibrary{calc}\n"
              "\\usetikzlibrary{decorations.pathmorphing}\n"
              "\\usetikzlibrary{spath3}\n"
              "\n"
              "\\tikzset{curve/.style={settings={#1},to path={(\\tikztostart)\n"
              "    .. controls ($(\\tikztostart)!\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
              "    and ($(\\tikztostart)!1-\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
              "    .. (\\tikztostart)\\tikztonodes}},\n"
              "    settings/.code={\\tikzset{quiver/.cd,#1}\n"
              "        \\def\\pv##1{\\pgfkeysvalueof{/tikz/quiver/##1}}},\n"
              "    quiver/.cd,pos/.initial=0.35,height/.initial=0}\n"
              "\n"
              "\\tikzset{between/.style n args={2}{/tikz/execute at end to={\n"
              "    \\tikzset{spath/split at keep middle={current}{#1}{#2}}\n"
              "}}}\n"
              "\n"
              "\\tikzset{tail reversed/.code={\\pgfsetarrowsstart{tikzcd to}}}\n"
              "\\tikzset{2tail/.code={\\pgfsetarrowsstart{Implies[reversed]}}}\n"
              "\\tikzset{2tail reversed/.code={\\pgfsetarrowsstart{Implies}}}\n"
              "\\tikzset{no body/.style={/tikz/dash pattern=on 0 off 1mm}}\n"
              "\n"
              (if (null? package-lines) "" (string-append (string-join package-lines "\n") "\n"))
              "\\begin{document}\n"
              (if (null? library-lines) "" (string-append (string-join library-lines "\n") "\n"))
              inner-code
              "\n\\end{document}"))))))

(check
  (wrap-quiver-code "\\documentclass{article}\n\\begin{document}\n\\end{document}")
  =>
  "\\documentclass{article}\n\\begin{document}\n\\end{document}"
)

(check
  (wrap-quiver-code "A \\arrow[r] & B")
  =>
  (string-append
    "\\documentclass[tikz]{standalone}\n"
    "\\usepackage{tikz-cd}\n"
    "\\usepackage{amssymb}\n"
    "\\usetikzlibrary{calc}\n"
    "\\usetikzlibrary{decorations.pathmorphing}\n"
    "\\usetikzlibrary{spath3}\n"
    "\n"
    "\\tikzset{curve/.style={settings={#1},to path={(\\tikztostart)\n"
    "    .. controls ($(\\tikztostart)!\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
    "    and ($(\\tikztostart)!1-\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
    "    .. (\\tikztostart)\\tikztonodes}},\n"
    "    settings/.code={\\tikzset{quiver/.cd,#1}\n"
    "        \\def\\pv##1{\\pgfkeysvalueof{/tikz/quiver/##1}}},\n"
    "    quiver/.cd,pos/.initial=0.35,height/.initial=0}\n"
    "\n"
    "\\tikzset{between/.style n args={2}{/tikz/execute at end to={\n"
    "    \\tikzset{spath/split at keep middle={current}{#1}{#2}}\n"
    "}}}\n"
    "\n"
    "\\tikzset{tail reversed/.code={\\pgfsetarrowsstart{tikzcd to}}}\n"
    "\\tikzset{2tail/.code={\\pgfsetarrowsstart{Implies[reversed]}}}\n"
    "\\tikzset{2tail reversed/.code={\\pgfsetarrowsstart{Implies}}}\n"
    "\\tikzset{no body/.style={/tikz/dash pattern=on 0 off 1mm}}\n"
    "\n"
    "\\begin{document}\n"
    "\\begin{tikzcd}[nodes in empty cells]\n"
    "A \\arrow[r] & B\n"
    "\\end{tikzcd}\n"
    "\\end{document}"
  )
)

(check
  (wrap-quiver-code "\\begin{tikzcd}\nA \\arrow[r] & B\n\\end{tikzcd}")
  =>
  (string-append
    "\\documentclass[tikz]{standalone}\n"
    "\\usepackage{tikz-cd}\n"
    "\\usepackage{amssymb}\n"
    "\\usetikzlibrary{calc}\n"
    "\\usetikzlibrary{decorations.pathmorphing}\n"
    "\\usetikzlibrary{spath3}\n"
    "\n"
    "\\tikzset{curve/.style={settings={#1},to path={(\\tikztostart)\n"
    "    .. controls ($(\\tikztostart)!\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
    "    and ($(\\tikztostart)!1-\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
    "    .. (\\tikztostart)\\tikztonodes}},\n"
    "    settings/.code={\\tikzset{quiver/.cd,#1}\n"
    "        \\def\\pv##1{\\pgfkeysvalueof{/tikz/quiver/##1}}},\n"
    "    quiver/.cd,pos/.initial=0.35,height/.initial=0}\n"
    "\n"
    "\\tikzset{between/.style n args={2}{/tikz/execute at end to={\n"
    "    \\tikzset{spath/split at keep middle={current}{#1}{#2}}\n"
    "}}}\n"
    "\n"
    "\\tikzset{tail reversed/.code={\\pgfsetarrowsstart{tikzcd to}}}\n"
    "\\tikzset{2tail/.code={\\pgfsetarrowsstart{Implies[reversed]}}}\n"
    "\\tikzset{2tail reversed/.code={\\pgfsetarrowsstart{Implies}}}\n"
    "\\tikzset{no body/.style={/tikz/dash pattern=on 0 off 1mm}}\n"
    "\n"
    "\\begin{document}\n"
    "\\begin{tikzcd}\n"
    "A \\arrow[r] & B\n"
    "\\end{tikzcd}\n"
    "\\end{document}"
  )
)

(check
  (wrap-quiver-code "\\[\\begin{tikzcd}\n&& \\bullet && \\bullet \\\\\n\\bullet && \\bullet\n\\arrow[from=1-3, to=2-1]\n\\arrow[from=2-3, to=1-5]\n\\end{tikzcd}\\]")
  =>
  (string-append
    "\\documentclass[tikz]{standalone}\n"
    "\\usepackage{tikz-cd}\n"
    "\\usepackage{amssymb}\n"
    "\\usetikzlibrary{calc}\n"
    "\\usetikzlibrary{decorations.pathmorphing}\n"
    "\\usetikzlibrary{spath3}\n"
    "\n"
    "\\tikzset{curve/.style={settings={#1},to path={(\\tikztostart)\n"
    "    .. controls ($(\\tikztostart)!\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
    "    and ($(\\tikztostart)!1-\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
    "    .. (\\tikztostart)\\tikztonodes}},\n"
    "    settings/.code={\\tikzset{quiver/.cd,#1}\n"
    "        \\def\\pv##1{\\pgfkeysvalueof{/tikz/quiver/##1}}},\n"
    "    quiver/.cd,pos/.initial=0.35,height/.initial=0}\n"
    "\n"
    "\\tikzset{between/.style n args={2}{/tikz/execute at end to={\n"
    "    \\tikzset{spath/split at keep middle={current}{#1}{#2}}\n"
    "}}}\n"
    "\n"
    "\\tikzset{tail reversed/.code={\\pgfsetarrowsstart{tikzcd to}}}\n"
    "\\tikzset{2tail/.code={\\pgfsetarrowsstart{Implies[reversed]}}}\n"
    "\\tikzset{2tail reversed/.code={\\pgfsetarrowsstart{Implies}}}\n"
    "\\tikzset{no body/.style={/tikz/dash pattern=on 0 off 1mm}}\n"
    "\n"
    "\\begin{document}\n"
    "\\begin{tikzcd}\n"
    "&& \\bullet && \\bullet \\\\\n"
    "\\bullet && \\bullet\n"
    "\\arrow[from=1-3, to=2-1]\n"
    "\\arrow[from=2-3, to=1-5]\n"
    "\\end{tikzcd}\n"
    "\\end{document}"
  )
)

(check
  (wrap-quiver-code "\\begin{equation*}\n\\begin{tikzcd}\n&& \\bullet && \\bullet \\\\\n\\bullet && \\bullet\n\\arrow[from=1-3, to=2-1]\n\\arrow[from=2-3, to=1-5]\n\\end{tikzcd}\n\\end{equation*}")
  =>
  (string-append
    "\\documentclass[tikz]{standalone}\n"
    "\\usepackage{tikz-cd}\n"
    "\\usepackage{amssymb}\n"
    "\\usetikzlibrary{calc}\n"
    "\\usetikzlibrary{decorations.pathmorphing}\n"
    "\\usetikzlibrary{spath3}\n"
    "\n"
    "\\tikzset{curve/.style={settings={#1},to path={(\\tikztostart)\n"
    "    .. controls ($(\\tikztostart)!\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
    "    and ($(\\tikztostart)!1-\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
    "    .. (\\tikztostart)\\tikztonodes}},\n"
    "    settings/.code={\\tikzset{quiver/.cd,#1}\n"
    "        \\def\\pv##1{\\pgfkeysvalueof{/tikz/quiver/##1}}},\n"
    "    quiver/.cd,pos/.initial=0.35,height/.initial=0}\n"
    "\n"
    "\\tikzset{between/.style n args={2}{/tikz/execute at end to={\n"
    "    \\tikzset{spath/split at keep middle={current}{#1}{#2}}\n"
    "}}}\n"
    "\n"
    "\\tikzset{tail reversed/.code={\\pgfsetarrowsstart{tikzcd to}}}\n"
    "\\tikzset{2tail/.code={\\pgfsetarrowsstart{Implies[reversed]}}}\n"
    "\\tikzset{2tail reversed/.code={\\pgfsetarrowsstart{Implies}}}\n"
    "\\tikzset{no body/.style={/tikz/dash pattern=on 0 off 1mm}}\n"
    "\n"
    "\\begin{document}\n"
    "\\begin{tikzcd}\n"
    "&& \\bullet && \\bullet \\\\\n"
    "\\bullet && \\bullet\n"
    "\\arrow[from=1-3, to=2-1]\n"
    "\\arrow[from=2-3, to=1-5]\n"
    "\\end{tikzcd}\n"
    "\\end{document}"
  )
)

(check-report "Quiver plugin unit tests")
