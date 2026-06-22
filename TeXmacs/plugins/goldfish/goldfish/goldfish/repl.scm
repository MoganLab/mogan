
(define-library (goldfish repl)
  (import (texmacs protocol) (liii list) (liii string) (liii sys) (liii base))
  (export goldfish-welcome goldfish-repl is-sicp-mode?)
  (begin

    (define (goldfish-welcome)
      (let ((mode (last (argv))))
        (if (string=? mode "default")
          (flush-prompt "> ")
          (flush-prompt (string-append (string-upcase mode) "] "))
        ) ;if
      ) ;let

      (flush-verbatim (string-append "Goldfish Scheme "
                        (version)
                        " Community Edition by LiiiLabs\n"
                        "implemented on S7 Scheme ("
                        (substring (*s7* 'version) 3)
                        ")"
                      ) ;string-append
      ) ;flush-verbatim
    ) ;define

    (define (escape-string str)
      (string-join (map (lambda (char)
                          (if (char=? char #\")
                            (string #\\ #\")
                            (if (char=? char #\\) (string #\\ #\\) (string char))
                          ) ;if
                        ) ;lambda
                     (string->list str)
                   ) ;map
      ) ;string-join
    ) ;define
    (define (goldfish-quote s)
      (string-append "\"" (escape-string s) "\"")
    ) ;define

    (define (build-goldfish-result obj)
      (let ((output (object->string obj))
            (leadings (list "(document" "(math" "(equation*" "(align" "(with" "(graphics"))
           ) ;
        (if (find (lambda (x) (string-starts? output x)) leadings)
          output
          (string-append "(goldfish-result " (goldfish-quote output) ")")
        ) ;if
      ) ;let
    ) ;define

    (define (goldfish-print obj)
      (if (eq? obj #<unspecified>)
        (flush-scheme-u8 "")
        (flush-scheme-u8 (build-goldfish-result obj))
      ) ;if
    ) ;define

    (define (eval-and-print code)
      (catch #t
        (lambda () (goldfish-print (eval-string code (rootlet))))
        (lambda args
          (begin
            (flush-scheme-u8 (string-append "(errput (document "
                               (goldfish-quote (symbol->string (car args)))
                               (if (and (>= (length args) 2) (not (null? (cadr args))))
                                 (goldfish-quote (object->string (cadr args)))
                                 ""
                               ) ;if
                               "))"
                             ) ;string-append
            ) ;flush-scheme-u8
          ) ;begin
        ) ;lambda
      ) ;catch
    ) ;define

    (define (read-eval-print)
      (let ((code (read-paragraph-by-visible-eof)))
        (if (string=? code "") #t (eval-and-print code))
      ) ;let
    ) ;define

    (define (goldfish-repl)
      (begin
        (read-eval-print)
        (goldfish-repl)
      ) ;begin
    ) ;define

  ) ;begin
) ;define-library
