(define-library (mogan stem)
  (import (liii os)
    (liii path)
    (liii subprocess)
    (scheme base)
    (scheme process-context)
    (scheme write)
  ) ;import
  (export main)
  (begin
    (define (display-line text)
      (display text)
      (newline)
    ) ;define

    (define (display-help)
      (display-line "Usage:")
      (display-line "  gf stem --build")
      (display-line "  gf stem --run")
      (newline)
      (display-line "Commands:")
      (display-line "  --build  Clean .xmake and build dirs, then xmake config + xmake build stem"
      ) ;display-line
      (display-line "           (compile cache shared at ~/.xmake/.build_cache)")
      (display-line "  --run    Run the built stem binary (xmake run stem)")
      (newline)
      (display-line "Directories cleaned by --build:")
      (display-line "  .xmake, build, 3rdparty/.xmake, 3rdparty/build,")
      (display-line "  3rdparty/<sub>/.xmake, 3rdparty/<sub>/build")
    ) ;define

    (define (fail message)
      (display-line (string-append "Error: " message))
      1
    ) ;define

    (define (die message)
      (display-line (string-append "Error: " message))
      (exit 1)
    ) ;define

    (define (ok)
      0
    ) ;define

    (define (root . parts)
      (apply path-join (cons (getcwd) parts))
    ) ;define

    (define (exec command . opts)
      (display-line (string-append "$ " command))
      (let ((status (apply run command opts)))
        (if (= status 0) status (die (string-append "command failed: " command)))
      ) ;let
    ) ;define

    (define (trash-dir rel)
      (when (path-dir? (root rel))
        (display-line (string-append "Cleaning " rel))
        (let ((status (run (string-append "trash " rel))))
          (unless (= status 0)
            (die (string-append "command failed: trash " rel))
          ) ;unless
        ) ;let
      ) ;when
    ) ;define

    (define (clean-artifacts)
      (for-each trash-dir '(".xmake" "build" "3rdparty/.xmake" "3rdparty/build"))
      (when (path-dir? (root "3rdparty"))
        (vector-for-each (lambda (entry)
                           (when (path-dir? (root "3rdparty" entry))
                             (trash-dir (string-append "3rdparty/" entry "/.xmake"))
                             (trash-dir (string-append "3rdparty/" entry "/build"))
                           ) ;when
                         ) ;lambda
          (path-list (root "3rdparty"))
        ) ;vector-for-each
      ) ;when
    ) ;define

    (define (trash-available?)
      (let-values (((out err code)
                    (run-values "trash --version" :stdout 'discard :stderr 'discard)
                   ) ;
                  ) ;
        (= code 0)
      ) ;let-values
    ) ;define

    (define (require-trash)
      (if (trash-available?)
        #t
        (die "trash command not found; please install trash (e.g. 'trash-cli' on Linux, 'brew install trash' on macOS)"
        ) ;die
      ) ;if
    ) ;define

    (define (shared-cache-dir)
      (let ((home (or (getenv "HOME") (getenv "USERPROFILE"))))
        (if home
          (string-append home "/.xmake/.build_cache")
          (die "cannot determine the home directory (tried HOME and USERPROFILE)")
        ) ;if
      ) ;let
    ) ;define

    (define (build-stem)
      (require-trash)
      (display-line (string-append "WORK_SPACE: " (getcwd)))
      (clean-artifacts)
      (exec (string-append "xmake config -vD --yes --is_community=n"
              " --ccache=y --ccachedir="
              (shared-cache-dir)
            ) ;string-append
      ) ;exec
      (exec "xmake build stem")
      (ok)
    ) ;define

    (define (run-stem)
      (exec "xmake run stem")
      (ok)
    ) ;define

    (define (parse-args)
      (let ((argv (command-line)))
        (if (and (pair? argv) (pair? (cdr argv))) (cddr argv) '())
      ) ;let
    ) ;define

    (define (main)
      (let ((args (parse-args)))
        (cond ((or (null? args) (member "-h" args) (member "--help" args))
               (display-help)
               (ok)
              ) ;
              ((string=? (car args) "--build")
               (if (null? (cdr args))
                 (build-stem)
                 (fail "--build does not accept extra arguments")
               ) ;if
              ) ;
              ((string=? (car args) "--run")
               (if (null? (cdr args))
                 (run-stem)
                 (fail "--run does not accept extra arguments")
               ) ;if
              ) ;
              (else (fail (string-append "unknown stem command: " (car args))))
        ) ;cond
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
