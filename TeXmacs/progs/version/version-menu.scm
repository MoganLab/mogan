
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : version-menu.scm
;; DESCRIPTION : menus for versioning portions of text
;; COPYRIGHT   : (C) 2010  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (version version-menu)
  (:use (version version-compare) (version version-tmfs))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Compare with other revision
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind version-compare-menu
  (when (version-revision? (current-buffer))
    ("Current user version" (compare-with-newer* (version-head (current-buffer))))
    ---
  ) ;when
  (with history
    (version-history* (current-buffer))
    (assuming (list? history)
      (for (line (if (< (length history) 25) history (sublist history 0 25)))
        (with (rev by date msg)
          line
          (let* ((cur (current-buffer))
                 (head (if (version-revision? cur) (version-head cur) cur))
                 (msg* (if (<= (string-length msg) 50) msg (string-append (substring msg 0 50) "..."))
                 ) ;msg*
                 (name (string-append "Version "
                         (version-beautify-revision cur rev)
                         " by "
                         by
                         " on "
                         date
                         ": "
                         msg*
                       ) ;string-append
                 ) ;name
                 (dest (version-revision-url head rev))
                ) ;
            (when (!= (url->url dest) (url->url cur))
              ((eval `(verbatim ,name))
               (if (version-newer? dest cur)
                 (compare-with-newer dest)
                 (compare-with-older dest)
               ) ;if
              ) ;
            ) ;when
          ) ;let*
        ) ;with
      ) ;for
      ---
    ) ;assuming
  ) ;with
  ("Older version"
    (choose-file compare-with-older "Compare with older version" "")
  ) ;
  ("Newer version"
    (choose-file compare-with-newer "Compare with newer version" "")
  ) ;
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main version menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind version-menu
  (assuming (versioned? (current-buffer))
    (assuming (version-supports-history? (current-buffer))
      (when (!= (version-status (current-buffer)) "unknown")
        ("History" (version-show-history (current-buffer)))
      ) ;when
      ---
    ) ;assuming
  ) ;assuming
  (assuming (version-revision? (current-buffer))
    (assuming (version-supports-history? (version-head (current-buffer)))
     ("History" (version-show-history (version-head (current-buffer))))
     ---
    ) ;assuming
  ) ;assuming
  (assuming (versioned? (current-buffer))
    (assuming (version-supports-svn-style? (current-buffer))
      (when (!= (version-status (current-buffer)) "unknown")
        ("Update" (version-interactive-update (current-buffer)))
      ) ;when
      (when (== (version-status (current-buffer)) "unknown")
        ("Register" (register-buffer (current-buffer)))
      ) ;when
      (when (and (!= (version-status (current-buffer)) "unknown")
              (or (== (version-status (current-buffer)) "modified")
                (buffer-modified? (current-buffer))
              ) ;or
            ) ;and
        ("Commit" (version-interactive-commit (current-buffer)))
      ) ;when
      ---
    ) ;assuming
  ) ;assuming
  (assuming (versioned? (current-buffer))
    (assuming (version-supports-git-style? (current-buffer))
     ("Global status" (git-show-status))
    ) ;assuming
    (assuming (version-supports-git-style? (current-buffer))
     ("Global history" (git-show-log))
    ) ;assuming
    ---
  ) ;assuming
  (assuming (or (versioned? (current-buffer)) (version-revision? (current-buffer)))
    (-> "Compare with"
      ;; (when (versioned? (current-buffer))
      ;;  (when (buffer-tmfs? (current-buffer))
      ;;    ("With current version"
      ;;      (git-compare-with-current (current-buffer))))
      ;;  (when (buffer-tmfs? (current-buffer))
      ;;    ("With parent version"
      ;;      (git-compare-with-parent (current-buffer))))
      ;;  (when (and (not (buffer-tmfs? (current-buffer)))
      ;;             (buffer-has-diff? (current-buffer)))
      ;;    ("With the HEAD"
      ;;      (git-compare-with-master (current-buffer)))))
      (link version-compare-menu)
    ) ;->
  ) ;assuming
  (assuming (not (or (versioned? (current-buffer)) (version-revision? (current-buffer))))
    (-> "Compare"
     ("With older version"
       (choose-file compare-with-older "Compare with older version" "")
     ) ;
     ("With newer version"
       (choose-file compare-with-newer "Compare with newer version" "")
     ) ;
    ) ;->
  ) ;assuming
  (-> "Move::difference"
   ("First difference" (version-first-difference))
   ("Previous difference" (version-previous-difference))
   ("Next difference" (version-next-difference))
   ("Last difference" (version-last-difference))
  ) ;->
  (when (or (inside-version?) (selection-active-any?))
    (-> "Show"
     ("Both versions" (version-show 'version-both))
     ("Old version" (version-show 'version-old))
     ("New version" (version-show 'version-new))
    ) ;->
    (-> "Retain"
     ("Current version" (version-retain 'current))
     ("Old version" (version-retain 0))
     ("New version" (version-retain 1))
    ) ;->
  ) ;when
  (-> "Grain"
   ("Detailed" (version-set-grain "detailed"))
   ("Block::version" (version-set-grain "block"))
   ("Rough" (version-set-grain "rough"))
  ) ;->
) ;menu-bind
