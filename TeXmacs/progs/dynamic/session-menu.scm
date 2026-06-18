
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : session-menu.scm
;; DESCRIPTION : menus for sessions
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic session-menu)
  (:use (dynamic session-edit)
    (dynamic program-edit)
    (generic generic-menu)
    (binary common)
  ) ;:use
) ;texmacs-module

(import (liii os) (liii sys))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Remote plugins
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (select-remote-plugin-widget quit)
  (let* ((where "")
         (set-where (lambda (new-where)
                      (when new-where
                        (set! where new-where)
                        (refresh-now "supported-plugins")
                        (refresh-now "enter-server")
                      ) ;when
                    ) ;lambda
         ) ;set-where
         (key-press (lambda (what)
                      (when what
                        (set! where (car what))
                        (when (== (cadr what) "return")
                          (set-where where)
                        ) ;when
                      ) ;when
                    ) ;lambda
         ) ;key-press
         (encode (lambda (triple)
                   (with (p v cmd)
                     triple
                     (with s (upcase-first p) (if (== v "default") s (string-append s " (" v ")")))
                   ) ;with
                 ) ;lambda
         ) ;encode
         (decode (lambda (name)
                   (and-with l
                     (list-remote-plugins where)
                     (with t
                       (list-find (cadr l) (lambda (u) (== (encode u) name)))
                       (list (car t) (string-append where "/" (cadr t)))
                     ) ;with
                   ) ;and-with
                 ) ;lambda
         ) ;decode
         (plugin-list (lambda ()
                        (with l (list-remote-plugins where) (if (not l) (list) (map encode (cadr l))))
                      ) ;lambda
         ) ;plugin-list
        ) ;
    (padded (hlist (resize "300px"
                     "400px"
                     (vlist (bold (text "Remote servers"))
                       ===
                       (refreshable "remote-servers"
                         (choice (set-where answer) (remote-connection-servers) where)
                       ) ;refreshable
                     ) ;vlist
                   ) ;resize
              //
              //
              //
              (resize "300px"
                "400px"
                (vlist (bold (text "Supported plug-ins"))
                  ===
                  (refreshable "supported-plugins"
                    (scrollable (choice (quit (decode answer)) (plugin-list) ""))
                  ) ;refreshable
                ) ;vlist
              ) ;resize
            ) ;hlist
      ===
      ===
      (hlist (text "Server:")
        //
        (refreshable "enter-server"
          (input (key-press answer) "search-server" (list where) "1w")
        ) ;refreshable
        //
        //
        //
        (explicit-buttons ("Add"
                            (detect-remote-plugins where)
                            (refresh-now "remote-servers")
                            (refresh-now "supported-plugins")
                          ) ;
          //
          ("Update" (update-remote-plugins where) (refresh-now "supported-plugins"))
          //
          ("Remove"
            (remove-remote-plugins where)
            (refresh-now "remote-servers")
            (refresh-now "supported-plugins")
          ) ;
        ) ;explicit-buttons
      ) ;hlist
    ) ;padded
  ) ;let*
) ;tm-widget

(tm-define (open-remote-plugin-selector name call-back)
  (:interactive #t)
  (dialogue-window select-remote-plugin-widget call-back name)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Inserting sessions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define binary-type "python")

(define binary-path "")

(define binary-list
  (list "python"
    "aspell"
    "conda"
    "convert"
    "goldfish"
    "ghostscript"
    "hunspell"
    "identify"
    "inkscape"
    "pandoc"
    "pdftocairo"
    "rsvg-convert"
    "maxima"
  ) ;list
) ;define

(define-format windows-executables (:name "Executable") (:suffix "exe" "bat"))

(tm-define (name-to-id name)
  (cond ((string=? name "python") "python3")
        ((string=? name "ghostcript") "gs")
        (else name)
  ) ;cond
) ;tm-define

(tm-define (update-path)
  (let* ((this-opt (string-append "plugin:binary:" (name-to-id binary-type)))
         (this-path (get-preference this-opt))
        ) ;
    (cond ((string=? this-path "default") (set! binary-path ""))
          (else (set! binary-path this-path))
    ) ;cond
    (refresh-now "input-path-widget")
  ) ;let*
) ;tm-define

(tm-define (set-binary-path)
  (let* ((binary-opt (string-append "plugin:binary:" (name-to-id binary-type))))
    (set-preference binary-opt binary-path)
  ) ;let*
) ;tm-define

(tm-define (find-binary-in-candidates)
  (when (string=? binary-path "")
    #f
  ) ;when
  (let ((url (string->url binary-path))
        (binary-name (cond ((string=? binary-type "ghostscript") "gs")
                           (else binary-type)
                     ) ;cond
        ) ;binary-name
       ) ;
    (if (and (url-exists? url)
          (url-regular? url)
          (string-contains? (url->string (url-tail url)) binary-name)
        ) ;and
      (if (access (url->string url) 'X_OK) #t #f)
      #f
    ) ;if
  ) ;let
) ;tm-define

(tm-widget (please-restart-widget cmd)
  (resize "200px"
    "50px"
    (padded (text "Please restart Mogan to apply the changes."))
  ) ;resize
  (bottom-buttons >> ("ok" (cmd "ok")))
) ;tm-widget

(tm-widget (path-incorrect-widget cmd)
  (resize "200px" "50px" (padded (text "The path you entered is incorrect!")))
  (bottom-buttons >> ("ok" (cmd "ok")))
) ;tm-widget

(tm-widget (manual-path-widget cmd)
  (resize "500px"
    "150guipx"
    (padded (invisible (update-path))
      (hlist (text "Select binary: ")
        (enum (begin
                (set! binary-type (cadr answer))
                (update-path)
              ) ;begin
          (map (lambda (x) (list 'verbatim x)) binary-list)
          (car binary-list)
          "50em"
        ) ;enum
      ) ;hlist
      (glue #t #f 0 10)
      (refreshable "input-path-widget"
        (hlist (text "Select path: ")
          (inert (input "path" "string" (list binary-path) ""))
          //
          (explicit-buttons ((balloon (icon "tm_find.xpm") "Choose binary file")
                             (choose-file (lambda (selected-url)
                                            (set! binary-path (url->system selected-url))
                                            (refresh-now "input-path-widget")
                                          ) ;lambda
                               "Choose binary file"
                               (if (os-win32?) "windows-executables" "generic")
                             ) ;choose-file
                            ) ;
          ) ;explicit-buttons
        ) ;hlist
      ) ;refreshable
      (glue #t #f 0 10)
      (hlist (glue #t #f 0 10)
        (explicit-buttons ("ok" (cmd "ok")) // ("cancel" (cmd "cancel")))
      ) ;hlist
    ) ;padded
  ) ;resize
) ;tm-widget

(tm-menu (supported-sessions-menu)
  (for (name (session-list))
    (let* ((menu-name (session-name name)) (l (local-connection-variants name)))
      (assuming (nnull? l)
        (assuming (== l (list "default"))
         ((eval `(verbatim ,menu-name)) (make-session name "default"))
        ) ;assuming
        (assuming (!= l (list "default"))
          (-> (eval `(verbatim ,menu-name))
            (for (variant l) ((eval `(verbatim ,variant)) (make-session name variant)))
          ) ;->
        ) ;assuming
      ) ;assuming
    ) ;let*
  ) ;for
) ;tm-menu

(menu-bind insert-session-menu
  (when (and (style-has? "std-dtd") (in-text?))
    ("Scheme" (make-session "scheme" "default"))
    ---
    (link supported-sessions-menu)
    ---
    (assuming #f
     ("Remote"
       (open-remote-plugin-selector "Start remote session"
         (lambda (x) (apply make-session x))
       ) ;open-remote-plugin-selector
     ) ;
     ("Other" (interactive make-session))
    ) ;assuming
    ("Manual path"
      (dialogue-window manual-path-widget
        (lambda (args)
          (when (!= args "cancel")
            (if (not (find-binary-in-candidates))
              (dialogue-window path-incorrect-widget
                (lambda (args) (set! binary-type (car binary-list)))
                "Warning"
              ) ;dialogue-window
              (begin
                (set-binary-path)
                (dialogue-window please-restart-widget
                  (lambda (args) (set! binary-type (car binary-list)))
                  "Warning"
                ) ;dialogue-window
              ) ;begin
            ) ;if
          ) ;when
          (set! binary-type (car binary-list))
        ) ;lambda
        "Set binary path"
      ) ;dialogue-window
    ) ;
  ) ;when
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Submenus of the Sessions menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind session-input-menu
  (when (in-plugin-with-converters?)
    ("Mathematical input" (toggle-session-math-input))
  ) ;when
  ("Text input" (toggle-session-text-input))
  ("Multiline input" (toggle-session-multiline-input))
) ;menu-bind

(menu-bind session-output-menu
  (if (in-scheme?)
   ("Pretty tree output" (toggle-session-scheme-trees))
   ("Pretty scheme tree output" (toggle-session-scheme-strees))
   ("Mathematical output" (toggle-session-scheme-math))
   ---
  ) ;if
  ("Show timings" (toggle-session-output-timings))
) ;menu-bind

(menu-bind session-session-menu
 ("Clear all fields" (session-clear-all))
 ("Fold all fields" (session-fold-all))
 ("Unfold all fields" (session-unfold-all))
 ---
 ("Evaluate fields in order" (toggle-session-program))
 ---
 ("Create subsession" (field-insert-fold (focus-tree)))
 ("Split session" (session-split))
) ;menu-bind

(menu-bind session-evaluate-menu
 ("Evaluate" (session-evaluate))
 ("Evaluate all" (session-evaluate-all))
 ("Evaluate above" (session-evaluate-above))
 ("Evaluate below" (session-evaluate-below))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The Session menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-session-language)
  (with lan (get-env "prog-language") (or (session-name lan) "Scheme"))
) ;tm-define
(tm-define (standard-options l)
  (:require (in? l field-tags))
  (list "framed-session" "ring-session" "large-formulas")
) ;tm-define

(tm-menu (focus-tag-menu t)
  (:require (field-context? t))
  (inert ((eval (focus-session-language)) (noop) (noop)))
  (when (alternate-context? t)
    ((check "Unfolded" "v" (alternate-second? (focus-tree)))
     (alternate-toggle (focus-tree))
    ) ;
  ) ;when
  (assuming (focus-has-preferences? t)
    (-> "Preferences" (dynamic (focus-preferences-menu t)))
  ) ;assuming
  ("Describe" (set-message "Not yet implemented" ""))
) ;tm-menu

(tm-menu (focus-move-menu t)
  (:require (field-context? t))
  ("Previous field" (traverse-previous))
  ("Next field" (traverse-next))
  ("First field" (traverse-first))
  ("Last field" (traverse-last))
) ;tm-menu

(tm-define (focus-can-insert-remove? t) (:require (field-context? t)) #t)

(tm-menu (focus-insert-menu t)
  (:require (field-context? t))
  ("Insert field above" (field-insert (focus-tree) #f))
  ("Insert field below" (field-insert (focus-tree) #t))
  ("Insert text field above" (field-insert-text (focus-tree) #f))
  ("Insert text field below" (field-insert-text (focus-tree) #t))
  ---
  ("Remove previous field" (field-remove (focus-tree) #f))
  ("Remove next field" (field-remove (focus-tree) #t))
  ("Remove banner" (field-remove-banner (focus-tree)))
  ("Remove last field" (field-remove-extreme (focus-tree) #t))
) ;tm-menu

(tm-menu (focus-hidden-menu t) (:require (field-context? t)))

(tm-menu (focus-extra-menu t)
  (:require (field-context? t))
  ---
  (-> "Input options" (link session-input-menu))
  (-> "Output options" (link session-output-menu))
  (-> "Session" (link session-session-menu))
  ---
  (-> "Evaluate" (link session-evaluate-menu))
  ("Interrupt execution" (plugin-interrupt))
  ("Close session" (plugin-stop))
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Sessions icons
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (alternate-second-name t) (:require (field-context? t)) "Unfold")

(tm-define (alternate-second-icon t)
  (:require (field-context? t))
  "tm_alternate_both.xpm"
) ;tm-define

(tm-menu (focus-tag-icons t)
  (:require (field-context? t))
  (dynamic (focus-toggle-icons t))
  (mini #t (inert ((eval (focus-session-language)) (noop))))
  (assuming (focus-has-preferences? t)
    (=> (balloon (icon "tm_focus_prefs.xpm") "Preferences for tag")
      (dynamic (focus-preferences-menu t))
    ) ;=>
  ) ;assuming
  ((balloon (icon "tm_focus_help.xpm") "Describe tag") (focus-help))
) ;tm-menu

(tm-menu (focus-move-icons t)
  (:require (field-context? t))
  ((balloon (icon "tm_similar_first.xpm") "Go to first similar tag")
   (traverse-first)
  ) ;
  ((balloon (icon "tm_similar_previous.xpm") "Go to previous similar tag")
   (traverse-previous)
  ) ;
  ((balloon (icon "tm_similar_next.xpm") "Go to next similar tag")
   (traverse-next)
  ) ;
  ((balloon (icon "tm_similar_last.xpm") "Go to last similar tag")
   (traverse-last)
  ) ;
) ;tm-menu

(tm-menu (focus-insert-icons t)
  (:require (field-context? t))
  ((balloon (icon "tm_insert_up.xpm") "Insert field above")
   (structured-insert-up)
  ) ;
  ((balloon (icon "tm_insert_down.xpm") "Insert field below")
   (structured-insert-down)
  ) ;
  ((balloon (icon "tm_delete_up.xpm") "Remove field above")
   (field-remove (focus-tree) #f)
  ) ;
  ((balloon (icon "tm_delete_down.xpm") "Remove field below")
   (field-remove (focus-tree) #t)
  ) ;
) ;tm-menu

(tm-menu (focus-hidden-icons t) (:require (field-context? t)))

(tm-menu (focus-extra-icons t)
  (:require (field-context? t))
  (glue #f #f 8 0)
  (=> (balloon (icon "tm_plugin_input.xpm") "Input options")
    (link session-input-menu)
  ) ;=>
  (=> (balloon (icon "tm_plugin_output.xpm") "Output options")
    (link session-output-menu)
  ) ;=>
  (=> (balloon (icon "tm_session_session.xpm") "Session commands")
    (link session-session-menu)
  ) ;=>
  (glue #f #f 10 0)
  (=> (balloon (icon "tm_go.xpm") "Evaluate fields") (link session-evaluate-menu))
  (if (!= (get-env "prog-language") "scheme")
   ((balloon (icon "tm_stop.xpm") "Interrupt execution") (plugin-interrupt))
   ((balloon (icon "tm_clsession.xpm") "Close session") (plugin-stop))
  ) ;if
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Help icons
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind session-help-icons
  ;; Each plugin appends its own entry
) ;menu-bind
