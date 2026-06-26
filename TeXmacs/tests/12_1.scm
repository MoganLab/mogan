(import (liii check))

(check-set-mode! 'report-failed)

(define (get-tag-name bib_file)
  (with bib
    (tree->stree (parse-bib (string-load bib_file)))
    (bib-car (bib-cdr (bib-cdr (bib-car (bib-cdr bib)))))
  ) ;with
) ;define

(define (test-get-tag-name)
  (check (get-tag-name "$TEXMACS_PATH/tests/bib/12_1.bib")
    =>
    (utf8->cork "数理逻辑2010汪芳庭")
  ) ;check
) ;define

(tm-define (test_12_1) (test-get-tag-name) (check-report))
