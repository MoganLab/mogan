
(define-library (texmacs protocol)
  (export data-begin
    data-end
    data-escape
    DATA_BEGIN
    DATA_END
    DATA_COMMAND
    DATA_ESCAPE
    flush-verbatim
    flush-prompt
    flush-scheme
    flush-file
    flush-markdown
    flush-latex
    flush-command
    read-paragraph-by-visible-eof
  ) ;export
  (begin

    (define DATA_BEGIN (integer->char 2))
    (define DATA_END (integer->char 5))
    (define DATA_COMMAND (integer->char 16))
    (define DATA_ESCAPE (integer->char 27))

    (define (data-begin)
      (display DATA_BEGIN)
    ) ;define

    (define (data-end)
      (display DATA_END)
      (flush-output-port)
    ) ;define

    (define (data-escape)
      (write DATA_ESCAPE)
    ) ;define

    (define (flush-any msg)
      (data-begin)
      (display msg)
      (data-end)
    ) ;define

    (define (flush-verbatim msg)
      (flush-any (string-append "utf8:" msg))
    ) ;define

    (define (flush-scheme msg)
      (if (string? msg)
        (flush-any (string-append "scheme:" msg))
        (flush-any (string-append "scheme:" (object->string msg)))
      ) ;if
    ) ;define

    (define (flush-prompt msg)
      (flush-any (string-append "prompt#" msg))
    ) ;define

    (define (flush-file path)
      (flush-any (string-append "file:" path))
    ) ;define

    (define (flush-markdown msg)
      (flush-any (string-append "markdown:" msg))
    ) ;define

    (define (flush-command msg)
      (if (string? msg)
        (flush-any (string-append "command:" msg))
        (flush-any (string-append "command:" (object->string msg)))
      ) ;if
    ) ;define

    (define (flush-latex msg)
      (flush-any (string-append "latex:" msg))
    ) ;define

    (define (read-paragraph-by-visible-eof)
      (define (read-code code)
        (let ((line (read-line)))
          (if (string=? line "<EOF>\n") code (read-code (string-append code line)))
        ) ;let
      ) ;define

      (read-code "")
    ) ;define

  ) ;begin
) ;define-library
