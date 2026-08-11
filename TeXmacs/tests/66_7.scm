(use-modules (tmu tmu-format))

(import (liii check) (liii path))

(define (test-single-quote)
  (let ((cork_60 (string (integer->char 96))) (cork_27 (string (integer->char 39))))
    (check (utf8->herk "‘’") => (string-append cork_60 "<#2019>"))
    (check (herk->utf8 (string-append cork_60 "<#2019>")) => "‘’")
  ) ;let
) ;define


(define (test-binary-data-encoding)
  ;; Test the encoding and decoding of binary data in TeXmacs documents
  ;; This test verifies that binary data is properly preserved through
  ;; TeXmacs tree and STM format conversions
  (let* ((tmu-path "TeXmacs/tests/tmu/66_7_encoding.tmu")
         ;; Read the TeXmacs document containing binary data
         (texmacs-doc (path-read-text tmu-path))
         ;; Parse the TeXmacs document into a tree structure
         (texmacs-tree (tmu->texmacs texmacs-doc))
         ;; Convert the tree to STM format for serialization
         (stm (texmacs->stm texmacs-tree))
         ;; Convert back to TeXmacs tree to verify data preservation
         (expected-texmacs-tree (stm->texmacs stm))
         ;; Path to the expected STM output file
         (stm-path "TeXmacs/texts/misc/66_7_encoding.txt")
         ;; Read the expected STM output for comparison
         (expected-stm (path-read-text stm-path))
        ) ;
    ;; (path-write-text stm-path stm)
    ;; Save the generated STM to file for inspection
    ;; Verify that the STM format matches the expected output
    (check stm => expected-stm)
    ;; Verify that the TeXmacs tree structure is preserved through conversions
    (check texmacs-tree => expected-texmacs-tree)
  ) ;let*
) ;define

(tm-define (test_66_7) (test-single-quote) (test-binary-data-encoding))
