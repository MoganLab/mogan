
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-tools.scm
;; DESCRIPTION : various tools
;; COPYRIGHT   : (C) 2012  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs texmacs tm-tools))

(import (scheme base))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document statistics
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; TODO: if string in scheme represent unicode codepoint with single character
;;   rather than utf-8 encoding, replace utf8-string-length with string-length

(define (compress-newline s)
  (let* ((s1 (string-replace s "\r\n" "")) (s2 (string-replace s1 "\n" "")))
    (if (== s2 s) s (compress-newline s2))
  ) ;let*
) ;define

(tm-define (count-characters doc)
  (with s
    (convert doc "texmacs-tree" "verbatim-snippet")
    (utf8-string-length (compress-newline s))
  ) ;with
) ;tm-define

(define (compress-spaces s)
  (let* ((s1 (string-replace s "\n" " "))
         (s2 (string-replace s1 "\t" " "))
         (s3 (string-replace s2 "  " " "))
         (s4 (if (string-starts? s3 " ") (string-drop s3 1) s3))
         (s5 (if (string-ends? s4 " ") (string-drop-right s4 1) s4))
        ) ;
    (if (== s5 s) s (compress-spaces s5))
  ) ;let*
) ;define

(tm-define (count-words doc)
  (with s
    (convert doc "texmacs-tree" "verbatim-snippet")
    (let* ((tokens (string-tokenize-by-char (compress-spaces s) #\space))
           (non-chinese-tokens (filter (lambda (token) (is-pure-ascii? token)) tokens))
          ) ;
      (length non-chinese-tokens)
    ) ;let*
  ) ;with
) ;tm-define

;; 只保留纯ASCII字符的token

(define (is-pure-ascii? str)
  (let loop
    ((i 0))
    (if (>= i (string-length str))
      #t
      (let ((cp (char->integer (string-ref str i))))
        (if (<= cp 127) (loop (+ i 1)) #f)
      ) ;let
    ) ;if
  ) ;let
) ;define

(tm-define (count-lines doc)
  (with s
    (convert doc "texmacs-tree" "verbatim-snippet")
    (length (string-tokenize-by-char s #\newline))
  ) ;with
) ;tm-define

(tm-define (count-chars-no-space doc)
  (with s
    (convert doc "texmacs-tree" "verbatim-snippet")
    (utf8-string-length (compress-newline (string-replace s " " "")))
  ) ;with
) ;tm-define

(tm-define (count-chinese-and-words doc)
  (with s
    (convert doc "texmacs-tree" "verbatim-snippet")
    (let ((bv (string->byte-vector s)) (byte-len (string-length s)) (cnt-c 0) (cnt-w 0))

      (let loop
        ((pos 0))
        (if (>= pos byte-len)
          (cons cnt-c cnt-w)
          (let ((next-pos (bytevector-advance-utf8 bv pos byte-len)))
            (when (> next-pos pos)
              (let ((cp (decode-utf8-from-bv bv pos next-pos)))
                (cond ((<= 19968 cp 40959) (set! cnt-c (+ cnt-c 1)))
                      ((or (<= 65 cp 90) (<= 97 cp 122)) (set! cnt-w (+ cnt-w 1)))
                ) ;cond
              ) ;let
            ) ;when
            (loop next-pos)
          ) ;let
        ) ;if
      ) ;let
    ) ;let
  ) ;with
) ;tm-define

(define (decode-utf8-from-bv bv start end)
  (let ((b0 (bytevector-u8-ref bv start)) (len (- end start)))
    (case len
     ((1) b0)
     ((2)
      (logior (ash (logand b0 31) 6) (logand (bytevector-u8-ref bv (+ start 1)) 63))
     ) ;
     ((3)
      (logior (ash (logand b0 15) 12)
        (ash (logand (bytevector-u8-ref bv (+ start 1)) 63) 6)
        (logand (bytevector-u8-ref bv (+ start 2)) 63)
      ) ;logior
     ) ;
     ((4)
      (logior (ash (logand b0 7) 18)
        (ash (logand (bytevector-u8-ref bv (+ start 1)) 63) 12)
        (ash (logand (bytevector-u8-ref bv (+ start 2)) 63) 6)
        (logand (bytevector-u8-ref bv (+ start 3)) 63)
      ) ;logior
     ) ;
     (else 65533)
    ) ;case
  ) ;let
) ;define

(define (selection-or-document)
  (if (selection-active-any?) (selection-tree) (buffer-tree))
) ;define

(tm-widget ((show-counts-widget lines) done)
  (resize "600guipx"
    "300guipx"
    (vlist (hlist (glue #t #f 0 0)
             (vlist (glue #f #t 120 6)
               (bold (text "Stats information"))
               (glue #f #t 120 6)
               (for (l lines) === (text l))
             ) ;vlist
             (glue #t #f 0 0)
           ) ;hlist
      (glue #f #t 120 6)
      (hlist (glue #t #f 120 3) (bottom-buttons ("Close" (done))))
    ) ;vlist
  ) ;resize
) ;tm-widget

(tm-define (show-counts)
  (:interactive #t)
  (let* ((doc (selection-or-document))
         (chars (count-characters doc))
         (chars-ns (count-chars-no-space doc))
         (lines (count-lines doc))
         (p (count-chinese-and-words doc))
         (chinese (car p))
         (words (count-words doc))
         (pages (get-page-count))
         (lst (list (string-append "Page count: " (number->string pages))
                (string-append "Word count: " (number->string (+ words chinese)))
                (string-append "Character count (with spaces): " (number->string chars))
                (string-append "Character count (without spaces): " (number->string chars-ns))
                (string-append "Paragraph count: " (number->string lines))
                (string-append "Non-Chinese word: " (number->string words))
                (string-append "Chinese character: " (number->string chinese))
              ) ;list
         ) ;lst
        ) ;
    (dialogue-window (show-counts-widget lst) noop "Document statistics")
  ) ;let*
) ;tm-define

(define (save-aux-enabled?)
  (== (get-env "save-aux") "true")
) ;define
(tm-define (toggle-save-aux)
  (:synopsis "Toggle whether we save auxiliary data")
  (:check-mark "v" save-aux-enabled?)
  (let ((new (if (== (get-env "save-aux") "true") "false" "true")))
    (init-env "save-aux" new)
  ) ;let
) ;tm-define

(tm-define (clear-font-cache)
  (:synopsis "Clear font cache under TEXMACS_HOME_PATH and local cache path.")
  (system-remove (url-append (get-tm-cache-path) (string->url "font_cache.scm")))
  (map (lambda (x)
         (system-remove (url-append (get-tm-cache-path)
                          (url-append (string->url "fonts") (string->url x))
                        ) ;url-append
         ) ;system-remove
       ) ;lambda
    (list "font-database.scm" "font-features.scm" "font-characteristics.scm")
  ) ;map
) ;tm-define

(tm-define (scan-disk-for-fonts)
  (:interactive #t)
  (:synopsis "Scan disk for more fonts")
  (system-wait "Full search for more fonts on your system" "(can be long)")
  (font-database-build-local)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Miscellaneous
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (picture-gc) (picture-cache-reset) (update-all-buffers))
