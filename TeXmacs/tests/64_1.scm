(import (liii check))

(check-set-mode! 'report-failed)

(define (file-under-path path)
  (lambda (file) (url-concretize (url-append path file)))
) ;define

(define embedded-propose-4th (lambda (tree) (embedded-propose tree 4)))

(define (test-ascii-filename)
  (load-buffer "$TEXMACS_PATH/tests/64_1.tm")
  (let ((image-ascii (tree-ref (root-tree) 1 1))
        (image-untitled (tree-ref (root-tree) 1 2))
        (image-cork (tree-ref (root-tree) 1 3))
        (expected (file-under-path "$TEXMACS_PATH/tests"))
       ) ;
    (check (embedded-propose-4th image-cork) => (expected "学生：张佳.png"))
    (check (embedded-propose-4th image-ascii) => (expected "64_1.png"))
    (check (embedded-propose-4th image-untitled) => (expected "64_1-image-4.png"))
  ) ;let
) ;define

(define (test-cjk-filename)
  (load-buffer "$TEXMACS_PATH/tests/64_1_中文文件名.tm")
  (let ((image-ascii (tree-ref (root-tree) 1 1))
        (image-untitled (tree-ref (root-tree) 1 2))
        (image-cork (tree-ref (root-tree) 1 3))
        (expected (file-under-path "$TEXMACS_PATH/tests"))
       ) ;
    (check (embedded-propose-4th image-cork) => (expected "学生：张佳.png"))
    (check (embedded-propose-4th image-ascii) => (expected "64_1.png"))
    (check (embedded-propose-4th image-untitled)
      =>
      (expected "64_1_中文文件名-image-4.png")
    ) ;check
  ) ;let
) ;define

(define (test-embedded-suffix)
  (check (embedded-suffix '(image (tuple (raw-data "dummy")
                                    "<#672A><#547D><#540D><#7ED8><#56FE>.svg")
                             "100pt"
                             "80pt"
                             ""
                             "")
         ) ;embedded-suffix
    =>
    "svg"
  ) ;check
  (check (embedded-suffix '(image (tuple (raw-data "dummy") "filename.png")
                             "100pt"
                             "80pt"
                             ""
                             "")
         ) ;embedded-suffix
    =>
    "png"
  ) ;check
  (check (embedded-suffix '(image (tuple (raw-data "dummy") "png")
                             "100pt"
                             "80pt"
                             ""
                             ""))
    =>
    "png"
  ) ;check
) ;define

(tm-define (test_64_1)
  (test-ascii-filename)
  (test-cjk-filename)
  (test-embedded-suffix)
  (check-report)
) ;tm-define
