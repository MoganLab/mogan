
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : version-git.scm
;; DESCRIPTION : subroutines for the Git tools
;; COPYRIGHT   : (C) 2019  Darcy Shen, Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (version version-git) (:use (version version-tmfs)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Supported features
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (version-supports-svn-style? name)
  (:require (== (version-tool name) "git"))
  #f
) ;tm-define

(tm-define (version-supports-git-style? name)
  (:require (== (version-tool name) "git"))
  (versioned? name)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Git base command
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define NR_LOG_OPTION " -1000 ")

(define (delete-tail-newline a-str)
  (if (string-ends? a-str "\n")
    (delete-tail-newline (string-drop-right a-str 1))
    a-str
  ) ;if
) ;define

;; if git-versioned, return the root directory of the git repo
;; otherwise, return the root directory ("/")
(tm-define (git-root url)
  (let* ((dir (if (url-directory? url) url (url-head url)))
         (git-dir (url-append dir ".git"))
         (pdir (url-expand (url-append dir "..")))
        ) ;
    (cond ((url-directory? git-dir) (string-replace (url->system dir) "\\" "/"))
          ((== pdir dir) "/")
          (else (git-root pdir))
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (git-command url)
  (with work-dir
    (git-root url)
    (string-append "git" " --work-tree=" work-dir " --git-dir=" work-dir "/.git")
  ) ;with
) ;tm-define

;; Warning: use it carefully since the current buffer changes during tmfs reverting
(tm-define (current-git-root) (git-root (current-buffer)))

;; Warning: do not use it
(tm-define (current-git-command)
  (with work-dir
    (current-git-root)
    (string-append "git"
      " --work-tree="
      (current-git-root)
      " --git-dir="
      (current-git-root)
      "/.git"
    ) ;string-append
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; File status
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (buffer-status name) (git-status-file name (current-git-root)))

(tm-define (version-status name)
  (:require (== (version-tool name) "git"))
  (with ret
    (buffer-status name)
    (cond ((== ret "??") "unknown")
          ((== ret "  ") "unmodified")
          (else "modified")
    ) ;cond
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Predicates of Git and Buffer
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (buffer-to-unadd? name)
  (with ret
    (buffer-status name)
    (or (== ret "A ") (== ret "M ") (== ret "MM") (== ret "AM"))
  ) ;with
) ;tm-define

(tm-define (buffer-to-add? name)
  (with ret
    (buffer-status name)
    (or (== ret "??") (== ret " M") (== ret "MM") (== ret "AM"))
  ) ;with
) ;tm-define

(tm-define (buffer-histed? name)
  (with ret
    (buffer-status name)
    (or (== ret "M ") (== ret "MM") (== ret " M") (== ret "  "))
  ) ;with
) ;tm-define

(tm-define (buffer-has-diff? name)
  (with ret (buffer-status name) (or (== ret "M ") (== ret "MM") (== ret " M")))
) ;tm-define

(tm-define (buffer-tmfs? name) (string-starts? (url->string name) "tmfs"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; File history
;; 1. Eval `git log --follow --pretty=%ai%n%an%n%s%n%H --name-only <name>`
;; 2. Split the result by \n\n
;; 3. Transform each string record to texmacs document
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (git-history-item alist root)
  (with (date by msg commit blank path)
    alist
    (list (string-append commit
            ":"
            (url->tmfs-string (system->url (string-append root "/" path)))
          ) ;string-append
      by
      date
      msg
    ) ;list
  ) ;with
) ;define

(define (git-history-items alist root)
  (if (< (length alist) 6)
    (list)
    (cons (git-history-item (list-take alist 6) root)
      (git-history-items (list-drop alist 6) root)
    ) ;cons
  ) ;if
) ;define

(tm-define (version-history name)
  (:require (== (version-tool name) "git"))
  (let* ((cmd (string-append (git-command name)
                " log --follow --pretty=%ai%n%an%n%s%n%H --name-only"
                NR_LOG_OPTION
                (url->system name)
              ) ;string-append
         ) ;cmd
         (root (current-git-root))
         (ret1 (eval-system cmd))
         (ret2 (string-decompose ret1 "\n"))
        ) ;

    (git-history-items ret2 root)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; File revisions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (version-revision name rev)
  (:require (== (version-tool name) "git"))
  ;; (display* "Loading commit " rev " for " name "\n")
  (let* ((root (git-root name)) (ret (git-load-blob rev name root)))
    ;; (display* "Got " ret "\n")
    ret
  ) ;let*
) ;tm-define

(define (beautify-git-revision rev)
  (string-take rev 7)
) ;define

(tm-define (version-beautify-revision name rev)
  (:require (== (version-tool name) "git"))
  (beautify-git-revision rev)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Masters
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (git-master name)
  (let* ((cmd (string-append (git-command name) " log -1 --pretty=%H"))
         (ret (eval-system cmd))
        ) ;
    (delete-tail-newline ret)
  ) ;let*
) ;tm-define

;; Get the hashCode of the HEAD via `git log -1 --pretty=%H`
(tm-define (git-commit-master)
  (let* ((cmd (string-append (current-git-command) " log -1 --pretty=%H"))
         (ret (eval-system cmd))
        ) ;
    (delete-tail-newline ret)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Registration of files (add and reset)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (version-register name)
  (:require (== (version-tool name) "git"))
  (let* ((name-s (url->string name))
         (cmd (string-append (git-command name) " add " name-s))
         (ret (eval-system cmd))
        ) ;
    (set-message cmd (string-append "Registered file"))
  ) ;let*
) ;tm-define

(tm-define (version-unregister name)
  (:require (== (version-tool name) "git"))
  (let* ((name-s (url->string name))
         (cmd (string-append (git-command name) " reset HEAD " name-s))
         (ret (eval-system cmd))
        ) ;
    (set-message cmd "Unregistered file")
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Committing files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (git-commit-message root rev)
  (let* ((cmd (string-append (git-command root) " log -1 " rev))
         (ret (eval-system cmd))
        ) ;
    (string-split ret #\newline)
  ) ;let*
) ;tm-define

(tm-define (git-commit-parents root rev)
  (let* ((git (git-command root))
         (cmd (string-append git " show --no-patch --format=%P " rev))
         (ret1 (eval-system cmd))
         (ret2 (delete-tail-newline ret1))
         (ret3 (string-split ret2 #\newline))
         (ret4 (cAr ret3))
         (ret5 (string-split ret4 #\ ))
        ) ;
    ret5
  ) ;let*
) ;tm-define

(tm-define (git-commit-parent root rev) (cAr (git-commit-parents root rev)))

(tm-define (git-commit-file-parent file hash)
  (let* ((cmd (string-append (current-git-command)
                " log --pretty=%H "
                (current-git-root)
                "/"
                file
              ) ;string-append
         ) ;cmd
         (ret (eval-system cmd))
         (ret2 (string-decompose ret (string-append hash "\n")))
        ) ;
    ;; (display ret2)
    (if (== (length ret2) 1) hash (string-take (second ret2) 40))
  ) ;let*
) ;tm-define

(tm-define (git-commit-diff root parent hash)
  (let* ((cmd (if (== parent hash)
                (string-append (git-command root) " show " hash " --numstat --pretty=oneline")
                (string-append (git-command root) " diff --numstat " parent " " hash)
              ) ;if
         ) ;cmd
         (ret (eval-system cmd))
         (ret2 (if (== parent hash)
                 (cdr (string-split ret #\newline))
                 (string-split ret #\newline)
               ) ;if
         ) ;ret2
        ) ;
    (define (convert body)
      (let* ((alist (string-split body #\tab)))
        (with dest
          (url-append root (third alist))
          (if (== (first alist) "-")
            (list 0 0 (utf8->cork (third alist)) (string-length (third alist)))
            (list (string->number (first alist))
              (string->number (second alist))
              ($link (version-revision-url dest hash) (utf8->cork (third alist)))
              (string-length (third alist))
            ) ;list
          ) ;if
        ) ;with
      ) ;let*
    ) ;define
    (and (> (length ret2) 0) (string-null? (cAr ret2)) (map convert (cDr ret2)))
  ) ;let*
) ;tm-define

(tm-define (git-commit message)
  (let* ((cmd (string-append (current-git-command) " commit -m " (raw-quote message)))
         (ret (eval-system cmd))
        ) ;
    ;; (display ret)
    (set-message (string-append (current-git-command) " commit") message)
  ) ;let*
  (git-show-status)
) ;tm-define

(tm-define (git-interactive-commit)
  (:interactive #t)
  (git-show-status)
  (interactive (lambda (message) (git-commit message)))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Comparing versions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (git-compare-with-current name)
  (let* ((name-s (url->string name))
         (file-r (cAr (string-split name-s #\|)))
         (file (string-append (current-git-root) "/" file-r))
        ) ;
    (switch-to-buffer (string->url file))
    (compare-with-older name)
  ) ;let*
) ;tm-define

(tm-define (git-compare-with-parent name)
  (let* ((name-s (tmfs-cdr (tmfs-cdr (url->tmfs-string name))))
         (hash (first (string-split name-s #\|)))
         (file (second (string-split name-s #\|)))
         (parent (git-commit-file-parent file hash))
         (file-buffer-s (version-revision-url file parent))
         (parent (string->url file-buffer-s))
        ) ;
    (if (== name parent)
      ;; FIXME: should prompt a dialog
      (set-message "No parent" "No parent")
      (compare-with-older parent)
    ) ;if
  ) ;let*
) ;tm-define

(tm-define (git-compare-with-master name)
  (let* ((path (url->string name))
         (buffer (version-revision-url name (git-master name)))
         (master (string->url buffer))
        ) ;
    ;; (display* "\n" name "\n" buffer "\n" master "\n")
    (compare-with-older master)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Status
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tmfs-url-git root which)
  (string->url (string-append "tmfs://git/" which "/" (url->tmfs-string root)))
) ;tm-define

(tm-define (git-status root)
  (let* ((cmd (string-append (git-command root) " status --porcelain"))
         (ret1 (eval-system cmd))
         (ret2 (string-split ret1 #\newline))
        ) ;
    (define (convert name)
      (let* ((status (string-take name 2))
             (fname (string-drop name 3))
             (full (url->string (url-append root fname)))
             (file (if (or (string-starts? status "A") (string-starts? status "?"))
                     fname
                     ($link full (utf8->cork fname))
                   ) ;if
             ) ;file
            ) ;
        (list status file)
      ) ;let*
    ) ;define
    (when (> (length ret2) 0)
      (map convert (filter (lambda (x) (not (string-null? x))) ret2))
    ) ;when
  ) ;let*
) ;tm-define

(tm-define ($staged-file status file)
  (cond ((string-starts? status "A")
         (list 'concat "new file:   " file (list 'new-line))
        ) ;
        ((string-starts? status "M")
         (list 'concat "modified:   " file (list 'new-line))
        ) ;
        ((string-starts? status "R")
         (list 'concat "renamed:    " file (list 'new-line))
        ) ;
        (else "")
  ) ;cond
) ;tm-define

(tm-define ($unstaged-file status file)
  (cond ((string-ends? status "M") (list 'concat "modified:   " file (list 'new-line)))
        (else "")
  ) ;cond
) ;tm-define

(tm-define ($untracked-file status file)
  (cond ((== status "??") (list 'concat file (list 'new-line)))
        (else "")
  ) ;cond
) ;tm-define

(tm-define (git-status-content root)
  (with s
    (git-status root)
    ($generic ($when (not s) "Not git status available!")
      ($when s
        ($tmfs-title "Git Status")
        ($description-long ($describe-item "Changes to be commited"
                             ($for (x s) ($with (status file) x ($staged-file status file)))
                           ) ;$describe-item
          ($describe-item "Changes not staged for commit"
            ($for (x s) ($with (status file) x ($unstaged-file status file)))
          ) ;$describe-item
          ($describe-item "Untracked files"
            ($for (x s) ($with (status file) x ($untracked-file status file)))
          ) ;$describe-item
        ) ;$description-long
      ) ;$when
    ) ;$generic
  ) ;with
) ;tm-define

(tm-define (git-show-status)
  (cursor-history-add (cursor-path))
  (revert-buffer-revert (tmfs-url-git (current-git-root) "status"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Log
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (string->commit-diff root str)
  (if (string-null? str)
    '()
    (with alist
      (string-split str #\newline)
      (if (== (length alist) 4)
        (list (string-take (first alist) 19)
          (second alist)
          (third alist)
          ($link (tmfs-url-commit root (fourth alist))
            (beautify-git-revision (fourth alist))
          ) ;$link
        ) ;list
        '()
      ) ;if
    ) ;with
  ) ;if
) ;define

(tm-define (git-log root)
  (let* ((cmd (string-append (git-command root)
                " log --pretty=%ai%n%an%n%s%n%H%n"
                NR_LOG_OPTION
              ) ;string-append
         ) ;cmd
         (ret1 (eval-system cmd))
         (ret2 (string-decompose ret1 "\n\n"))
        ) ;
    (when (> (length ret2) 0)
      (filter (lambda (x) (> (length x) 0))
        (map (lambda (x) (string->commit-diff root x))
          (filter (lambda (x) (not (string-null? x))) ret2)
        ) ;map
      ) ;filter
    ) ;when
  ) ;let*
) ;tm-define

(tm-define (git-log-content root)
  (with h
    (git-log root)
    ($generic ($tmfs-title "Git Log")
      ($when (not h) "This directory is not under version control.")
      ($when h
        ($description-long ($for (x h)
                             ($with (date by msg commit)
                               x
                               ($describe-item ($inline "Commit " commit " by " (utf8->cork by) " on " date)
                                 (utf8->cork msg)
                               ) ;$describe-item
                             ) ;$with
                           ) ;$for
        ) ;$description-long
      ) ;$when
    ) ;$generic
  ) ;with
) ;tm-define

(tmfs-title-handler (git name doc)
  (let* ((root (tmfs-string->url (tmfs-cdr name)))
         (short (url->string (url-tail root)))
         (which (tmfs-car name))
        ) ;
    (cond ((== which "status") (string-append "Git Status - " short))
          ((== which "log") (string-append "Git Log - " short))
          (else (string-append "Git (unknown) - " short))
    ) ;cond
  ) ;let*
) ;tmfs-title-handler

(tmfs-load-handler (git name)
  (let* ((root (tmfs-string->url (tmfs-cdr name))) (which (tmfs-car name)))
    (cond ((== which "status") (git-status-content root))
          ((== which "log") (git-log-content root))
          (else '())
    ) ;cond
  ) ;let*
) ;tmfs-load-handler

(tm-define (git-show-log)
  (cursor-history-add (cursor-path))
  (revert-buffer-revert (tmfs-url-git (current-git-root) "log"))
) ;tm-define
