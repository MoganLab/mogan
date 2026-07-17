
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-telemetry.scm
;; DESCRIPTION : Initialize the fake 'telemetry' plugin
;; COPYRIGHT   : (C) 2026 Mogan Developers
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (binary goldfish))
(import (liii path))

(define (telemetry-serialize lan t)
  (with u
    (pre-serialize lan t)
    (with s (texmacs->utf8raw (stree->tree u)) (string-append s "\n<EOF>\n"))
  ) ;with
) ;define

(define (launcher)
  (let* ((home (path-from-env "TEXMACS_HOME_PATH"))
         (sys (path-from-env "TEXMACS_PATH"))
         (user (path-join home "plugins" "telemetry" "goldfish" "tm-telemetry.scm"))
         (sys-path (path-join sys "plugins" "telemetry" "goldfish" "tm-telemetry.scm"))
         (entry (if (url-exists? (path->string user))
                  (path->string user)
                  (path->string sys-path)
                ) ;if
         ) ;entry
        ) ;
    (string-append (string-quote (url->system (find-binary-goldfish)))
      " "
      (string-quote (url->system entry))
    ) ;string-append
  ) ;let*
) ;define

(plugin-configure telemetry
  (:require (has-binary-goldfish?))
  (:launch ,(launcher))
  (:serializer ,telemetry-serialize)
) ;plugin-configure
