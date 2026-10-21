
(define-library (texmacs protocol)
  (export data-begin data-end data-escape DATA_BEGIN DATA_END DATA_COMMAND
    DATA_ESCAPE flush-verbatim flush-prompt flush-scheme flush-file
    flush-markdown flush-latex flush-command read-paragraph-by-visible-eof
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

    ;; 行尾归一：R7RS 的 read-line 不保留行尾换行（goldfish >= 18.11.40 的
    ;; stdin 实现）；18.11.39 及更早的 stdin 实现用 fgets，保留换行。插件
    ;; 协议以逐行拼接还原段落，故新语义下须补回换行。
    (define (read-paragraph-line-with-eol line)
      (let ((n (string-length line)))
        (if
          (and (> n 0) (char=? (string-ref line (- n 1)) #\newline))
          line
          (string-append line "\n")
        ) ;if
      ) ;let
    ) ;define

    ;; 段末标记：同时接受带/不带行尾换行（含 CRLF）的 "<EOF>"。旧 goldfish
    ;; 送来的行自带 "\n"，新 goldfish 不带；两种都认，升级前后行为一致。
    (define (read-paragraph-eof-marker? line)
      (or (string=? line "<EOF>")
        (string=? line "<EOF>\n")
        (string=? line "<EOF>\r")
        (string=? line "<EOF>\r\n")
      ) ;or
    ) ;define

    (define (read-paragraph-by-visible-eof)
      (define (read-code code)
        (let ((line (read-line)))
          (if (read-paragraph-eof-marker? line)
            code
            (read-code (string-append code (read-paragraph-line-with-eol line)))
          ) ;if
        ) ;let
      ) ;define

      (read-code "")
    ) ;define

  ) ;begin
) ;define-library
