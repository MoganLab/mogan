;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0623.scm
;; DESCRIPTION : Comprehensive Unit and Integration tests for HTML export of inline/embedded images
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(use-modules (convert html tmhtml))

(check-set-mode! 'report-failed)

(define (test-embedded-extraction-logic)
  (display "Verifying tmhtml-extract-embedded parsing logic...\n")
  ;; 1. Check with a symbol inside a standard 2-tuple (hex literal style)
  (let* ((test-tuple '(tuple <#25504446> "pdf"))
         (extracted (tmhtml-extract-embedded test-tuple))
         (data-val (car extracted))
        ) ;
    (check (byte-vector? data-val) => #t)
    (check (vector-ref data-val 0) => 37)
    (check (vector-ref data-val 1) => 80)
    (check (vector-ref data-val 2) => 68)
    (check (vector-ref data-val 3) => 70)
    (check (cdr extracted) => "pdf")
  ) ;let*

  ;; 2. Check uppercase and lowercase hexadecimal casing
  (let* ((case-tuple '(tuple <#AaBbCcDdEeFf09> "png"))
         (res (tmhtml-extract-embedded case-tuple))
         (bytes (car res))
        ) ;
    (check (byte-vector? bytes) => #t)
    (check (length bytes) => 7)
    (check (vector-ref bytes 0) => 170)
    (check (vector-ref bytes 1) => 187)
    (check (vector-ref bytes 2) => 204)
    (check (vector-ref bytes 3) => 221)
    (check (vector-ref bytes 4) => 238)
    (check (vector-ref bytes 5) => 255)
    (check (vector-ref bytes 6) => 9)
  ) ;let*

  ;; 3. Check odd-length invalid hex string (should return empty byte-vector instead of crashing)
  (let* ((odd-tuple '(tuple <#255> "pdf")) (res (tmhtml-extract-embedded odd-tuple)))
    (check (car res) => #u8())
  ) ;let*

  ;; 4. Check empty hex symbol '<#>' (should return empty byte-vector)
  (let* ((empty-tuple '(tuple <#> "png")) (res (tmhtml-extract-embedded empty-tuple)))
    (check (car res) => #u8())
  ) ;let*

  ;; 5. Check non-hex characters fallback safety (should return safely decoded result with 0 fallback instead of crashing)
  (let* ((invalid-chars-tuple '(tuple <#255g> "png"))
         (res (tmhtml-extract-embedded invalid-chars-tuple))
         (bytes (car res))
        ) ;
    (check (byte-vector? bytes) => #t)
    (check (vector-ref bytes 0) => 37)
    (check (vector-ref bytes 1) => 80)
  ) ;let*

  ;; 6. Check with a base64 string inside a standard 2-tuple (Gnuplot style)
  ;; "mybinarydata" in base64 is "bXliaW5hcnlkYXRh"
  (let* ((name1 '(tuple "bXliaW5hcnlkYXRh" "pdf"))
         (res1 (tmhtml-extract-embedded name1))
        ) ;
    (check (byte-vector? (car res1)) => #t)
    (check (cdr res1) => "pdf")
  ) ;let*

  ;; 7. Check with a raw-data node wrapping data (classic embedded style)
  ;; "classicdata" in base64 is "Y2xhc3NpY2RhdGE="
  (let* ((name2 '(tuple (raw-data "Y2xhc3NpY2RhdGE=") "png"))
         (res2 (tmhtml-extract-embedded name2))
        ) ;
    (check (byte-vector? (car res2)) => #t)
    (check (cdr res2) => "png")
  ) ;let*

  ;; 3. Non-tuple inputs should return #f
  (check (tmhtml-extract-embedded "simplefilename.pdf") => #f)

  ;; 4. Empty and invalid tuples should return #f
  (check (tmhtml-extract-embedded '(tuple)) => #f)
  (check (tmhtml-extract-embedded '(tuple "only-one-arg")) => #f)
  (check (tmhtml-extract-embedded '(tuple "data" (invalid-suffix-type))) => #f)
) ;define

(define (test-path-suffix-base64)
  (display "Verifying path-style suffix normalizes to extension in Base64 URI...\n")
  (set! tmhtml-base64? #t)
  ;; Embedded tuple may carry a full relative path as its suffix (e.g. pasted screenshot metadata).
  ;; The suffix must be normalized to the real extension for the data URI MIME type.
  (let* ((path-tuple '(tuple <#89504E470D0A1A0A> "//d/LJQ/test.png"))
         (res (tmhtml-image (list path-tuple "0.8w" "")))
         (res-str (object->string res))
        ) ;
    (check (string-contains? res-str "data:image/png;base64,") => #t)
    (check (string-contains? res-str "data:image//d/LJQ") => #f)
  ) ;let*
  ;; Simple suffix should continue to work
  (let* ((simple-tuple '(tuple <#89504E470D0A1A0A> "png"))
         (res (tmhtml-image (list simple-tuple "" "")))
         (res-str (object->string res))
        ) ;
    (check (string-contains? res-str "data:image/png;base64,") => #t)
  ) ;let*
  ;; Path without extension should default to png
  (let* ((noext-tuple '(tuple <#89504E470D0A1A0A> "//d/LJQ/test"))
         (res (tmhtml-image (list noext-tuple "" "")))
         (res-str (object->string res))
        ) ;
    (check (string-contains? res-str "data:image/png;base64,") => #t)
  ) ;let*
  (display "Path-style suffix Base64 URI verified successfully.\n")
) ;define

(define (test-gnuplot-html-export-integration)
  (display "Verifying end-to-end Gnuplot inline image HTML export with Base64 embedding...\n"
  ) ;display
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0623_gnuplot_tuto.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html))
        ) ;
    ;; The HTML export must succeed and the output must contain images inlined as Base64 data URLs!
    (check (string-contains? html-content "data:image/png;base64,") => #t)
    ;; It should contain multiple base64 image occurrences because there are multiple plots
    (check (string-contains? html-content "<img") => #t)
    (display "HTML export and base64 embedding verified successfully.\n")
  ) ;let*
) ;define

(define (test-gnuplot-html-export-base64-off-integration)
  (display "Verifying HTML export with Base64 embedding turned off...\n")
  (set-preference "texmacs->html:base64" "off")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0623_gnuplot_tuto.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html))
        ) ;
    (set-preference "texmacs->html:base64" "on")
    ;; The HTML export must contain external file references (like "-2.png") instead of base64
    (check (string-contains? html-content "data:image/png;base64,") => #f)
    (check (string-contains? html-content "-2.png") => #t)
    (display "HTML export with Base64 off verified successfully.\n")
  ) ;let*
) ;define

(define (test-chinese-path-safety)
  (display "Verifying path safety with Chinese characters...\n")
  (let* ((chinese-url (url-glue (url-temp) "_测试_中文路径_测试.png"))
         (dummy-bytes #u8(137 80 78 71 13 10 26 10))
        ) ;
    ;; 1. Write to a Chinese path
    (tmhtml-write-binary-file chinese-url dummy-bytes)
    (check (url-exists? chinese-url) => #t)
    ;; 2. Read back from a Chinese path
    (let ((read-bytes (tmhtml-read-binary-file chinese-url)))
      (check (byte-vector? read-bytes) => #t)
      (check (length read-bytes) => 8)
      (check (vector-ref read-bytes 0) => 137)
      (check (vector-ref read-bytes 1) => 80)
      (check (vector-ref read-bytes 2) => 78)
      (check (vector-ref read-bytes 3) => 71)
    ) ;let
    ;; 3. Cleanup Chinese path file
    (url-remove chinese-url)
    (check (url-exists? chinese-url) => #f)
    (display "Chinese path safety verified successfully.\n")
  ) ;let*
) ;define

(tm-define (test_0623)
  (test-embedded-extraction-logic)
  (test-path-suffix-base64)
  (test-chinese-path-safety)
  (test-gnuplot-html-export-integration)
  (test-gnuplot-html-export-base64-off-integration)
  (check-report)
) ;tm-define
