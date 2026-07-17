
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; make-apidoc-doc.scm   (derived from build-glue.scm)
;; DESCRIPTION : generates a minimal doc file for all glue symbols
;; COPYRIGHT   : (C) 2016 The TeXmacs team
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(use-modules (ice-9 regex))

(define glue-defs '("build-glue-basic.scm" "build-glue-server.scm"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Convenient output routine
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define output-sub
  (lambda (l) (if (not (null? l)) (begin (display (car l)) (output-sub (cdr l)))))
) ;define

(define output (lambda l (output-sub l)))

(define (output-copyright from)
  noop
) ;define

(define (output-arg arg)
  (output " <scm-arg|" arg ">")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main build routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-routine l)
  ;; create an "explain" tag
  (let ((name (car l)) (croutine (cadr l)) (ret-type (caaddr l)) (args (cdaddr l)))
    (output "  <\\explain>\n    <scm|("
      (regexp-substitute/global #f "[>]" (symbol->string name) 'pre "\\<gtr\\>" 'post)
    ) ;output
    (map output-arg args)
    (output ")>\n<explain-synopsis|no synopsis>\n  <|explain>\n    Calls the <c++> function <cpp|"
      croutine
      "> which returns\n    <scm|"
      ret-type
      ">.\n  </explain>\n\n"
    ) ;output
  ) ;let
) ;define

(define build-routines
  (lambda (l)
    (if (not (null? l)) (begin (build-routine (car l)) (build-routines (cdr l))))
  ) ;lambda
) ;define


(define (build-main l)
  (build-routines (cddr l))
) ;define

(define-macro build (lambda l (build-main l)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Creation of the help document
;;
;; FIXME? it is tempting to define a macro
;; <assign|details|<\\macro|croutine|ret-type>
;;  Calls the <c++> function <cpp|<arg|croutine>> which returns
;;  <scm|<arg|ret-type>>.
;; </macro>>
;;
;; but presently macros are not expanded when the apidoc cache is collected
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(begin
  (output "<TeXmacs|1.99.4>\n\n<style|<tuple|tmdoc|english>>\n\n<\\body>\n<tmdoc-title|All glue functions>\n\nThis document lists all available <scheme> functions that are implemented in\nthe <c++> code and which, consequently, are neither defined nor documented in the\n<scheme> modules. Ideally each of these functions should be documented\nelsewhere in the documentation.\n\nThis document was generated automatically from the glue code definitions by\nthe script <verbatim|src/src/Scheme/Glue/make-apidoc-doc.scm> in <TeXmacs>\nsource code.\n\n\\;\n\n\\;\n\n"
  ) ;output

  (map load glue-defs)
  (output "\n  <tmdoc-copyright|2016|the <TeXmacs> team>\n\n  <tmdoc-license|Permission is granted to copy, distribute and/or modify this\n  document under the terms of the GNU Free Documentation License, Version 1.1\n  or any later version published by the Free Software Foundation; with no\n  Invariant Sections, with no Front-Cover Texts, and with no Back-Cover\n  Texts. A copy of the license is included in the section entitled \"GNU Free\n  Documentation License\".>\n</body>"
  ) ;output
) ;begin

