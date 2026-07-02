(import (liii check))

(check-set-mode! 'report-failed)

(define (test-no-title)
  (load-buffer "$TEXMACS_PATH/tests/tm/9_1_无标题.tm")
  (check (get-metadata "title") => "9_1_无标题.tm")
) ;define

(define (test-title)
  (load-buffer "$TEXMACS_PATH/tests/tm/9_1_with_title.tm")
  (check (get-metadata "title") => "标题")
) ;define

(define (test-metadata)
  (load-buffer "$TEXMACS_PATH/tests/tm/9_1_with_metadata.tm")
  (check (get-metadata "author") => "作者")
  (check (get-metadata "subject") => "主题")
  (check (get-metadata "title") => "标题")
) ;define

(tm-define (test_9_1)
  (test-no-title)
  (test-title)
  (test-metadata)
  (check-report)
) ;tm-define
