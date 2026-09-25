(import (liii check))

(check-set-mode! 'report-failed)

(define step-delay-ms 1000)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at
          (lambda ()
            (display* "[6101-step] " label "\n")
            (act)
            (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
          ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (test_6101)
  (let* ((doc-url "$TEXMACS_PATH/tests/tm/6101.tm") (x-center 0) (x-left 0) (x-right 0))
    (run-chain
      (list
        (cons "load buffer" (lambda () (load-buffer (string->url doc-url))))
        (cons "check initial align"
          (lambda ()
            (let* ((imgs (tree-search (buffer-tree) (lambda (t) (tree-is? t 'image))))
                   (img (car imgs))
                   (par (tree-ref img :up))
                   (bbox (tree-bounding-rectangle img))
                  ) ;
              (display* "Initial par-mode: " (get-image-alignment par) "\n")
              (display* "Initial bbox: " bbox "\n")
              (check (get-image-alignment par) => "center")
              (set! x-center (car bbox))
            ) ;let*
          ) ;lambda
        ) ;cons
        (cons "set left align"
          (lambda ()
            (let* ((imgs (tree-search (buffer-tree) (lambda (t) (tree-is? t 'image))))
                   (img (car imgs))
                   (par (tree-ref img :up))
                  ) ;
              (set-image-alignment par "left")
              (refresh-window)
            ) ;let*
          ) ;lambda
        ) ;cons
        (cons "check left align bbox"
          (lambda ()
            (let* ((imgs (tree-search (buffer-tree) (lambda (t) (tree-is? t 'image))))
                   (img (car imgs))
                   (par (tree-ref img :up))
                   (bbox (tree-bounding-rectangle img))
                  ) ;
              (display* "Left par-mode: " (get-image-alignment par) "\n")
              (display* "Left bbox: " bbox "\n")
              (check (get-image-alignment par) => "left")
              (set! x-left (car bbox))
              (check (< x-left x-center) => #t)
            ) ;let*
          ) ;lambda
        ) ;cons
        (cons "set right align"
          (lambda ()
            (let* ((imgs (tree-search (buffer-tree) (lambda (t) (tree-is? t 'image))))
                   (img (car imgs))
                   (par (tree-ref img :up))
                  ) ;
              (set-image-alignment par "right")
              (refresh-window)
            ) ;let*
          ) ;lambda
        ) ;cons
        (cons "check right align bbox"
          (lambda ()
            (let* ((imgs (tree-search (buffer-tree) (lambda (t) (tree-is? t 'image))))
                   (img (car imgs))
                   (par (tree-ref img :up))
                   (bbox (tree-bounding-rectangle img))
                  ) ;
              (display* "Right par-mode: " (get-image-alignment par) "\n")
              (display* "Right bbox: " bbox "\n")
              (check (get-image-alignment par) => "right")
              (set! x-right (car bbox))
              (check (> x-right x-center) => #t)
              (check (> x-right x-left) => #t)
            ) ;let*
          ) ;lambda
        ) ;cons
        (cons "report and quit" (lambda () (check-report) (quit-TeXmacs)))
      ) ;list
    ) ;run-chain
  ) ;let*
) ;tm-define
