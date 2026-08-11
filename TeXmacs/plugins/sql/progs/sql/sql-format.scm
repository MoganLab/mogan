;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE      : sql.scm
;; DESCRIPTION : SQL format definition
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (sql sql-format))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; SQL source files
;; Common extensions: .sql (standard), .ddl, .dml, .pks, .pkb (Oracle)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format sql (:name "SQL source code") (:suffix "sql" "SQL" "ddl" "dml"))

(define (texmacs->sql x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

(define (sql->texmacs x . opts)
  (code->texmacs x)
) ;define

(define (sql-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

(converter texmacs-tree sql-document (:function texmacs->sql))

(converter sql-document texmacs-tree (:function sql->texmacs))

(converter texmacs-tree sql-snippet (:function texmacs->sql))

(converter sql-snippet texmacs-tree (:function sql-snippet->texmacs))
