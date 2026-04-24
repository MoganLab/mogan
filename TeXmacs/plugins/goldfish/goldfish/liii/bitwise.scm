(define-library (liii bitwise)
  (import (srfi srfi-151) (liii error))
  (export lognot logand logior logxor ash)
  (export bitwise-not
    bitwise-and
    bitwise-ior
    bitwise-xor
    bitwise-eqv
    bitwise-or
    bitwise-nor
    bitwise-nand
    bit-count
    bitwise-orc1
    bitwise-orc2
    bitwise-andc1
    bitwise-andc2
    arithmetic-shift
    integer-length
    bitwise-if
    bit-set?
    copy-bit
    bit-swap
    any-bit-set?
    every-bit-set?
    first-set-bit
    bit-field
    bit-field-any?
    bit-field-every?
    bit-field-clear
    bit-field-set
  ) ;export
  (begin
    (define bitwise-or bitwise-ior)
  ) ;begin
) ;define-library
