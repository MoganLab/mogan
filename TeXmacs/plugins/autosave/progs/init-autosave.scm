;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-autosave.scm
;; DESCRIPTION : Initialize autosave plugin
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules
  (binary goldfish)
  (utils plugins plugin-eval))

(define (autosave-launcher)
  (let* ((goldfish (string-quote (url->system (find-binary-goldfish))))
         (home-script
           (string->url
             "$TEXMACS_HOME_PATH/plugins/autosave/goldfish/tm-autosave.scm"))
         (sys-script
           (string->url
             "$TEXMACS_PATH/plugins/autosave/goldfish/tm-autosave.scm"))
         (script
           (if (url-exists? "$TEXMACS_HOME_PATH/plugins/autosave")
               home-script
               sys-script)))
    (string-append goldfish " " (string-quote (url->system script)))))

(define (autosave-command s)
  ;; JSON is passed verbatim; the goldfish side reads until the visible EOF
  ;; marker, so callers may send compact or pretty-printed JSON.
  (string-append s "\n<EOF>\n"))

(plugin-configure autosave
  (:require (has-binary-goldfish?))
  (:launch ,(autosave-launcher))
  (:commander ,autosave-command))

(tm-define (autosave-call json)
  (:synopsis "Send a JSON request to the autosave plugin synchronously.")
  (connection-cmd "autosave" "default" json))

(tm-define (autosave-send json return)
  (:synopsis "Send a JSON request to the autosave plugin asynchronously.")
  (display* "[autosave-debug] autosave-send json=" json "\n")
  (plugin-command "autosave" "default" json return '()))
