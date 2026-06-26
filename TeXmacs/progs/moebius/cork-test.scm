
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : cork-test.scm
;; DESCRIPTION : Test suite for the cork encoding
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lolly cork-test))

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for cork<->utf8 on alphanumeric
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-alphanum)
  (check (cork->utf8 "abcdefghijklmnopgrstuvwxyz") => "abcdefghijklmnopgrstuvwxyz")
  (check (cork->utf8 "0123456789") => "0123456789")
  (check (utf8->cork "abcdefghijklmnopgrstuvwxyz") => "abcdefghijklmnopgrstuvwxyz")
  (check (utf8->cork "0123456789") => "0123456789")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for cork<->utf8 on angle brackets
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-angle)
  (check (uint32->utf8 (cork->utf8 "<langle>")) => #x27E8)
  (check (uint32->utf8 (cork->utf8 "<rangle>")) => #x27E9)
  (check (utf8->cork (uint32->utf8 #x27E8)) => "<langle>")
  (check (utf8->cork (uint32->utf8 #x27E9)) => "<rangle>")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-cork)
  (test-alphanum)
  (test-angle)
  (check-report)
) ;tm-define
