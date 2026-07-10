
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-python.scm
;; DESCRIPTION : Initialize python plugin
;; COPYRIGHT   : (C) 2004  Ero Carrera,
;;               (C) 2012  Adrian Soto
;;               (C) 2020  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules
  (dynamic session-edit)
  (dynamic program-edit)
  (binary python3)
  (binary conda))

(lazy-format (data python) python)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Plugin configuration
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Basically, the serializer makes the input preserve the newlines
;; and adds the string character "\n<EOF>\n" by the end.
;; I guess it could send "\x04" instead to signal a real EOF,
;; but I would need to check if that does not kill the pipe...
;; An alternative approach is to use the input-done? command
;; from TeXmacs, but, at the time of this writing, it did not work.--A

(define (python-serialize lan t)
  (with u (pre-serialize lan t)
    (with s (texmacs->utf8raw (stree->tree u))
      (string-append  s  "\n<EOF>\n"))))

(define (python-utf8-command)
  (string-append (url->system (find-binary-python3)) " -X utf8 "))

(define (python-launcher)
  (if (url-exists? "$TEXMACS_HOME_PATH/plugins/python")
      (string-append (python-utf8-command)
       (string-quote
        (url->string
         (string->url
          "$TEXMACS_HOME_PATH/plugins/python/bin/python.pex"))))
      (string-append (python-utf8-command)
       (string-quote
        (url->string
         (string->url
          "$TEXMACS_PATH/plugins/python/bin/python.pex"))))))

(define (conda-launcher path)
  (if (url-exists? "$TEXMACS_HOME_PATH/plugins/python")
      (string-append
       (url->system path)
       " -X utf8 "
       (string-quote
        (url->string
         (string->url
          "$TEXMACS_HOME_PATH/plugins/python/bin/python.pex"))))
      (string-append
       (url->system path)
       " -X utf8 "
       (string-quote
        (url->string
         (string->url
          "$TEXMACS_PATH/plugins/python/bin/python.pex"))))))

(define (conda-launchers)
  (map (lambda (path)
         (list :launch
               (string-append "conda_" (conda-env-name path)) (conda-launcher path)))
       (conda-env-python-list)))

(define (all-python-launchers)
  (let* ((launchers (conda-launchers))
         (default-launcher
           (cond ((has-binary-python3?) (python-launcher))
                 ((has-binary-conda?) (caddr (car launchers)))
                 (else ""))))
    (cons (list :launch default-launcher)
          launchers)))

(plugin-configure python
  (:require (or (has-binary-conda?)
                (has-binary-python3?)))
  ,@(all-python-launchers)
  (:tab-completion #t)
  (:serializer ,python-serialize)
  (:session "Python")
  (:scripts "Python"))

;(set-session-multiline-input "python" "default" #t)
;(set-program-multiline-input "python" "default" #t)

(when (supports-python?)
  (import-from (python-menus)))

