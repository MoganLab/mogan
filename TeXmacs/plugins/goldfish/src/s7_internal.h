/* s7_internal.h - internal definitions extracted from s7.c
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef S7_INTERNAL_H
#define S7_INTERNAL_H

#include <stdbool.h>
#include "s7.h"
#include "s7_internal_helpers.h"
/* s7, a Scheme interpreter
 *
 *   derived from TinyScheme 1.39, but not a single byte of that code remains
 *   SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 *
 * Mike Scholz provided the FreeBSD support (complex trig funcs, etc)
 * Rick Taube, Andrew Burnson, Donny Ward, Greg Santucci, and Christos Vagias provided the MS Visual C++ support
 * Kjetil Matheussen provided the mingw support
 *
 * Documentation is in s7.h, s7.html, s7-ffi.html, and s7-scm.html.
 * s7test.scm is a regression test.
 * repl.scm is a vt100-based listener.
 * nrepl.scm is a notcurses-based listener.
 * cload.scm and lib*.scm tie in various C libraries.
 * lint.scm checks Scheme code for infelicities.
 * r7rs.scm implements some of r7rs (small).
 * write.scm currrently has pretty-print.
 * mockery.scm has the mock-data definitions.
 * reactive.scm has reactive-set and friends.
 * stuff.scm has some stuff.
 * profile.scm has code to display profile data.
 * debug.scm has debugging aids.
 * case.scm has case*, an extension of case to pattern matching.
 * timing tests are in the s7 tools directory
 *
 * s7.c is organized as follows:
 *    structs and type flags
 *    internal debugging stuff
 *    constants
 *    GC
 *    stacks
 *    symbols and keywords
 *    lets
 *    continuations
 *    numbers
 *    characters
 *    strings
 *    ports
 *    format
 *    lists
 *    vectors
 *    hash-tables
 *    c-objects
 *    functions
 *    equal?
 *    generic length, copy, reverse, fill!, append
 *    error handlers
 *    sundry leftovers
 *    the optimizers
 *    multiple-values, quasiquote
 *    eval
 *    *s7*
 *    initialization and free
 *    repl
 *
 * naming conventions: s7_* usually are C accessible (s7.h), g_* are scheme accessible,
 *   H_* are documentation strings, Q_* are procedure signatures, scheme "?" corresponds to C "is_", scheme "->" to C "_to_",
 *   *_1 are ancillary functions, big_* refer to gmp, *_nr means no return, Inline means always-inline.
 *   In variables, i, j, and k are ints, p is a pair (usually), e is a let (environment), x and y are numbers (usually), o is opt_info*.
 *
 * ---------------- compile time switches ----------------
 */

#if defined __has_include
#  if __has_include ("mus-config.h")
#    include "mus-config.h"
#  endif
#else
#    include "mus-config.h"
#endif

/*
 * Your config file goes here, or just replace that #include line with the defines you need.
 * The compile-time switches involve booleans, complex numbers, and multiprecision arithmetic.
 * Currently we assume we have setjmp.h (used by the error handlers).
 *
 * Complex number support, which is problematic in C++, Solaris, and netBSD
 *   is on the HAVE_COMPLEX_NUMBERS switch. In OSX or Linux, if you're not using C++,
 *
 *   #define HAVE_COMPLEX_NUMBERS 1
 *   #define HAVE_COMPLEX_TRIG 1
 *
 *   In g++ I use:
 *
 *   #define HAVE_COMPLEX_NUMBERS 1
 *   #define HAVE_COMPLEX_TRIG 0
 *
 *   In Windows and tcc both are 0.
 *
 *   Some systems (FreeBSD) have complex.h, but some random subset of the trig funcs, so
 *   HAVE_COMPLEX_NUMBERS means we can find
 *      cimag creal cabs csqrt carg conj
 *   and HAVE_COMPLEX_TRIG means we have
 *      cacos cacosh casin casinh catan catanh ccos ccosh cexp clog cpow csin csinh ctan ctanh
 *
 * When HAVE_COMPLEX_NUMBERS is 0, the complex functions are stubs that simply return their
 *   argument -- this will be very confusing for the s7 user because, for example, (sqrt -2)
 *   will return something bogus (it might not signal an error).
 *
 * so the incoming (non-s7-specific) compile-time switches are
 *     HAVE_COMPLEX_NUMBERS, HAVE_COMPLEX_TRIG, SIZEOF_VOID_P
 * if SIZEOF_VOID_P is not defined, we look for __SIZEOF_POINTER__ instead,
 *   the default is to assume that we're running on a 64-bit machine.
 *
 * and we use these predefined macros: __cplusplus, _MSC_VER, __GNUC__, __clang__, __ANDROID__
 *
 * if WITH_SYSTEM_EXTRAS is 1 (default is 1 unless _MSC_VER), various OS and file related functions are included.
 * if you want this file to compile into a stand-alone interpreter, define WITH_MAIN,
 *   to use nrepl also define WITH_NOTCURSES
 *
 * -O3 is often slower than -O2 (at least according to callgrind)
 * -march=native seems to improve tree-vectorization which is important in Snd
 * -ffast-math makes a mess of NaNs, and does not appear to be faster
 *   -fno-math-errno -fno-signed-zeros are slower
 *   I also tried -fno-signaling-nans -fno-trapping-math -fassociative-math, but at least one of them is much slower
 * this code doesn't compile anymore in gcc 4.3
 */

#if (defined(__GNUC__) || defined(__clang__) || defined(__TINYC__)) /* s7 uses PRId64 so (for example) g++ 4.4 is too old. clang defines __GNUC__ */
  #define WITH_GCC 1
#else
  #define WITH_GCC 0
#endif
#if (defined(__clang__) && __cplusplus) /* pointless -- this is a moving target */
  #define WITH_CLANG_PP 1
#else
  #define WITH_CLANG_PP 0
#endif


/* ---------------- initial sizes ---------------- */

#ifndef INITIAL_HEAP_SIZE
  #define INITIAL_HEAP_SIZE 64000         /* 29-Jul-21 -- seems faster */
#endif
/* the heap grows as needed, this is its initial size. If the initial heap is small, s7 can run in about 2.5 Mbytes of memory.
 * There are many cases where a bigger heap is faster (but hardware cache size probably matters more).
 * The heap size must be a multiple of 32.  Each object takes 48 bytes.  s7 is fine with the initial heap size set to 800.
 */

#ifndef SYMBOL_TABLE_SIZE
  #define SYMBOL_TABLE_SIZE 32749
#endif
/* names are hashed into the symbol table (a vector) and collisions are chained as lists.
 *   4129: tlet +530 [symbol_p_pp], thash +565 [make_symbol], max-bin: (3 5),  tlet: (258 3)
 *  16381: tlet +80  [symbol_p_pp], thash +80  [make_symbol], max-bin: (2 25), tlet: (85 1)
 *  24001: tlet +33  [symbol_p_pp], thash +50  [make_symbol], max-bin: (2 19), tlet: (56 7)
 *  32749: (677 symbols if exit.scm)                          max-bin: (2 13), tlet: (40 4)
 *  72101: tlet -40  [symbol_p_pp], thash -40  [make_symbol], max-bin: (2 11), tlet: (30 5)
 */

#ifndef INITIAL_STACK_SIZE
  #define INITIAL_STACK_SIZE 4096  /* was 2048 17-Mar-21 */
#endif
/* the stack grows as needed, each frame takes 4 entries, this is its initial size. (*s7* 'stack-top) divides size by 4 */

#define STACK_RESIZE_TRIGGER 256   /* was INITIAL_STACK_SIZE/2 which seems excessive */

#ifndef GC_TEMPS_SIZE
  #define GC_TEMPS_SIZE 256
#endif
/* the number of recent objects that are temporarily gc-protected; 8 works for s7test and snd-test.
 *    For the FFI, this sets the lag between a call on s7_cons and the first moment when its result
 *    might be vulnerable to the GC.
 */

#ifndef INITIAL_PROTECTED_OBJECTS_SIZE
  #define INITIAL_PROTECTED_OBJECTS_SIZE 16
#endif
/* a vector of objects that are (semi-permanently) protected from the GC, grows as needed */


/* ---------------- scheme choices ---------------- */




#ifndef WITH_PURE_S7
  #define WITH_PURE_S7 0
#endif
#if WITH_PURE_S7
  #define WITH_EXTRA_EXPONENT_MARKERS 0
  #define WITH_IMMUTABLE_UNQUOTE 1
  /* also omitted: *-ci* functions, char-ready?, cond-expand, multiple-values-bind|set!, call-with-values
   *   and a lot more (inexact/exact, integer-length, etc) -- see s7.html.
   */
#endif

#ifndef WITH_R7RS
  #define WITH_R7RS !WITH_PURE_S7
  /* this also requires (set! (*s7* 'scheme-version) 'r7rs) */
#endif

#ifndef WITH_EXTRA_EXPONENT_MARKERS
  #define WITH_EXTRA_EXPONENT_MARKERS 0
#endif
/* if 1, s7 recognizes "d", "f", "l", and "s" as exponent markers, in addition to "e" (also "D", "F", "L", "S") */

#ifndef WITH_SYSTEM_EXTRAS
  #define WITH_SYSTEM_EXTRAS (!_MSC_VER)
  /* this adds several functions that access file info, directories, times, etc */
#endif

#ifndef WITH_IMMUTABLE_UNQUOTE
  #define WITH_IMMUTABLE_UNQUOTE 0  /* this removes the name "unquote" */
#endif

#ifndef WITH_C_LOADER
  #if WITH_GCC && (!__MINGW32__) && (!__CYGWIN__)
    #define WITH_C_LOADER 1
  /* (load file.so [e]) looks for ([e] 'init_func) and if found, calls it as the shared object init function.
   * If WITH_SYSTEM_EXTRAS is 0, the caller needs to supply system and delete-file so that cload.scm works.
   */
  #else
    #define WITH_C_LOADER 0
    /* I think dlopen et al are available in MS C, but I have no way to test them; see load_shared_object below */
  #endif
#endif

#ifndef WITH_HISTORY
  #define WITH_HISTORY 0
  /* this includes a circular buffer of previous evaluations for debugging, ((owlet) 'error-history) and (*s7* 'history-size) */
#endif

#ifndef DEFAULT_HISTORY_SIZE
  #define DEFAULT_HISTORY_SIZE 8
  /* this is the default length of the eval history buffer */
#endif
#if WITH_HISTORY
  #define MAX_HISTORY_SIZE 1048576
#endif

#ifndef DEFAULT_PRINT_LENGTH
  #define DEFAULT_PRINT_LENGTH 40 /* (*s7* 'print-length) initial value, was 32 but that's too small 26-May-24 */
#endif

#ifndef WITH_NUMBER_SEPARATOR
  #define WITH_NUMBER_SEPARATOR 0
#endif

/* in case mus-config.h forgets these */
#ifdef _MSC_VER
  #ifndef HAVE_COMPLEX_NUMBERS
    #define HAVE_COMPLEX_NUMBERS 0
    /* Da Shen adds that you'll need the compiler flag /fp:precise if you're using github actions */
  #endif
  #ifndef HAVE_COMPLEX_TRIG
    #define HAVE_COMPLEX_TRIG 0
  #endif
#else
  #ifndef HAVE_COMPLEX_NUMBERS
    #if __TINYC__ || (__clang__ && __cplusplus) /* clang++ is hopeless */
      #define HAVE_COMPLEX_NUMBERS 0
    #else
      #define HAVE_COMPLEX_NUMBERS 1
    #endif
  #endif
  #if __cplusplus || __TINYC__
    #ifndef HAVE_COMPLEX_TRIG
      #define HAVE_COMPLEX_TRIG 0
    #endif
  #else
    #ifndef HAVE_COMPLEX_TRIG
      #define HAVE_COMPLEX_TRIG 1
    #endif
  #endif
#endif

#ifndef WITH_MULTITHREAD_CHECKS
  #define WITH_MULTITHREAD_CHECKS 0
  /* debugging aid if using s7 in a multithreaded program -- this code courtesy of Kjetil Matheussen */
#endif

#ifndef WITH_WARNINGS
  #define WITH_WARNINGS 0
  /* int+int overflows to real, etc: this adds warnings which are expensive even though they are never called (procedure overhead) */
#endif

#ifndef S7_DEBUGGING
  #define S7_DEBUGGING 0
#endif

#undef DEBUGGING
#define DEBUGGING typo!
#define HAVE_GMP typo!

#define SHOW_EVAL_OPS 0

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE  /* for qsort_r, grumble... */
#endif

#ifndef _MSC_VER
  #include <unistd.h>
  #include <sys/param.h>
  #include <strings.h>
  #include <errno.h>
#else
  /* in Snd these are in mus-config.h */
  #ifndef MUS_CONFIG_H_LOADED
    #if _MSC_VER < 1900
      #define snprintf _snprintf
    #endif
    #if _MSC_VER > 1200
      #define _CRT_SECURE_NO_DEPRECATE 1
      #define _CRT_NONSTDC_NO_DEPRECATE 1
      #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
    #endif
  #endif
  #include <io.h>
  #pragma warning(disable: 4244) /* conversion might cause loss of data warning */
#endif

#if WITH_GCC && (!S7_DEBUGGING)
  #define Inline inline __attribute__((__always_inline__))
#else
  #ifdef _MSC_VER
    #define Inline __forceinline
  #else
    #define Inline inline
  #endif
#endif

#ifndef WITH_VECTORIZE
  #define WITH_VECTORIZE 1
#endif

#if (WITH_VECTORIZE) && (defined(__GNUC__) && (__GNUC__ >= 5)) /* is this included -in -O2 now? */
  #define Vectorized __attribute__((optimize("tree-vectorize")))
#else
  #define Vectorized
#endif

#if WITH_GCC
  #define Sentinel __attribute__((sentinel))
#else
  #define Sentinel
#endif

#ifdef _MSC_VER
  #define no_return _Noreturn /* deprecated in C23 */
#else
  #define no_return __attribute__((noreturn))
  /* this is ok in gcc/g++/clang and tcc; clang++ complains about "noreturn", hence "no_return" */
  /* pure attribute is rarely applicable here, and does not seem to be helpful (maybe safe_strlen) */
#endif

#ifndef S7_ALIGNED
  #define S7_ALIGNED 0
  /* memclr, local_memset, local_strncmp */
#endif

#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <setjmp.h>
#include <locale.h>

#ifdef _MSC_VER
  #define MS_WINDOWS 1
#else
  #define MS_WINDOWS 0
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
  #define Jmp_Buf       jmp_buf
  #define SetJmp(A, B)  setjmp(A)
  #define LongJmp(A, B) longjmp(A, B)
#else
  #define Jmp_Buf       sigjmp_buf
  #define SetJmp(A, B)  sigsetjmp(A, B)
  #define LongJmp(A, B) siglongjmp(A, B)
  /* we need sigsetjmp, not setjmp for nrepl's interrupt (something to do with signal masks??)
   *   unfortunately sigsetjmp is slower than setjmp. In one case, the sigsetjmp version runs
   *   in 24 seconds, but the setjmp version takes 10 seconds, yet callgrind says there is almost no difference?
   */
#endif

#if !MS_WINDOWS
  #include <pthread.h>
#endif

#if __cplusplus
  #include <cmath>
#else
  #include <math.h>
#endif

#include "s7.h"
#include "s7_internal_helpers.h"
#include "s7_scheme_format.h"
#include "s7_scheme_base.h"
#include "s7_scheme_inexact.h"
#include "s7_scheme_complex.h"
#include "s7_scheme_char.h"
#include "s7_scheme_write.h"
#include "s7_scheme_symbol.h"
#include "s7_scheme_predicate.h"
#include "s7_liii_bitwise.h"
#include "s7_liii_string.h"
#include "s7_liii_hash_table.h"
#include "s7_liii_list.h"
#include "s7_liii_vector.h"
#include "s7_module.h"
#include "s7_dtoa.h"
#include "s7_op_names.h"
#include "s7_ctables.h"

/* there is also apparently __STDC_NO_COMPLEX__ */
#if WITH_CLANG_PP
  #define CMPLX(x, y) __builtin_complex ((double) (x), (double) (y))
#endif
#if HAVE_COMPLEX_NUMBERS
  #if __cplusplus
    #include <complex>
    using namespace std;  /* the code has to work in C as well as C++, so we can't scatter std:: all over the place */
    /* moved the typedef to s7.h. */
  #else
    #include <complex.h>
    /* typedef double complex s7_complex; */
    #if defined(__sun) && defined(__SVR4)
      #undef _Complex_I
      #define _Complex_I 1.0i
    #endif
  #endif
  #ifndef CMPLX
    #if (!(defined(__cplusplus))) && (__GNUC__ > 4 || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7))) && !defined(__INTEL_COMPILER)
      #define CMPLX(x, y) __builtin_complex ((double) (x), (double) (y))
    #else
      #define CMPLX(r, i) ((r) + ((i) * (s7_complex)_Complex_I))
    #endif
  #endif
#endif

#if WITH_CLANG_PP
  #define s7_complex_i ((double)1.0i)
#else
#if (defined(__GNUC__))
  #define s7_complex_i 1.0i
#else
  #define s7_complex_i (s7_complex)_Complex_I /* a float, but we want a double */
#endif
#endif

#ifndef M_PI
  #define M_PI 3.1415926535897932384626433832795029L
#endif

#ifndef INFINITY
  #ifndef HUGE_VAL
    #define INFINITY (1.0/0.0) /* -log(0.0) is triggering dumb complaints from cppcheck */
    /* there is sometimes a function, infinity(), MSC apparently uses HUGE_VALF, gcc has __builtin_huge_val() */
  #else
    #define INFINITY HUGE_VAL
  #endif
#endif

#ifndef NAN /* deprecated in C23? */
  #define NAN (INFINITY / INFINITY) /* apparently ieee754 suggests 0.0/0.0 */
#endif

#if ((!__NetBSD__) && ((_MSC_VER) || (!defined(__STC__)) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ < 199901L))))
  #define __func__ __FUNCTION__
#endif

#ifndef POINTER_32 /* for testing */
#if (((defined(SIZEOF_VOID_P)) && (SIZEOF_VOID_P == 4)) || ((defined(__SIZEOF_POINTER__)) && (__SIZEOF_POINTER__ == 4)) || (!defined(__LP64__)))
  #define POINTER_32 true
#else
  #define POINTER_32 false
#endif
#endif

#define WRITE_REAL_PRECISION 16
#ifdef __TINYC__
  typedef double long_double; /* (- .1 1) -> 0.9! and others similarly: (- double long_double) is broken */
#else
  typedef long double long_double;
#endif
typedef uint64_t s7_uint;

#define ld64 PRId64
/* #define lu64 PRIu64 */
#define p64 PRIdPTR

#define MAX_FLOAT_FORMAT_PRECISION 128 /* does this make any sense? 53 bits in mantissa: 16 digits, are the extra digits just garbage? */

/* types */
enum {T_FREE = 0,
      T_PAIR, T_NIL, T_UNUSED, T_UNDEFINED, T_UNSPECIFIED, T_EOF, T_BOOLEAN, T_CHARACTER, T_SYNTAX, T_SYMBOL,
      T_INTEGER, T_RATIO, T_REAL, T_COMPLEX, T_BIG_INTEGER, T_BIG_RATIO, T_BIG_REAL, T_BIG_COMPLEX,
      T_STRING, T_C_OBJECT, T_VECTOR, T_INT_VECTOR, T_FLOAT_VECTOR, T_BYTE_VECTOR, T_COMPLEX_VECTOR,
      T_CATCH, T_DYNAMIC_WIND, T_HASH_TABLE, T_LET, T_ITERATOR,
      T_STACK, T_COUNTER, T_SLOT, T_C_POINTER, T_OUTPUT_PORT, T_INPUT_PORT, T_RANDOM_STATE, T_CONTINUATION, T_GOTO,
      T_CLOSURE, T_CLOSURE_STAR, T_MACRO, T_MACRO_STAR, T_BACRO, T_BACRO_STAR,
      T_C_MACRO, T_C_FUNCTION_STAR, T_C_FUNCTION, T_C_RST_NO_REQ_FUNCTION,
      NUM_TYPES};
/* T_UNUSED, T_STACK, T_SLOT, T_DYNAMIC_WIND, T_CATCH, and T_COUNTER are internal */

static const char *s7_type_names[] =
  {"free", "pair", "nil", "unused", "undefined", "unspecified", "eof_object", "boolean", "character", "syntax", "symbol",
   "integer", "ratio", "real", "complex", "big_integer", "big_ratio", "big_real", "big_complex",
   "string", "c_object", "vector", "int_vector", "float_vector", "byte_vector", "complex_vector",
   "catch", "dynamic_wind", "hash_table", "let", "iterator",
   "stack", "counter", "slot", "c_pointer", "output_port", "input_port", "random_state", "continuation", "goto",
   "closure", "closure*", "macro", "macro*", "bacro", "bacro*",
   "c_macro", "c_function*", "c_function", "c_rst_no_req_function",
   };

/* 1:pair, 2:nil, 3:unused, 4:undefined, 5:unspecified, 6:eof, 7:boolean, 8:character, 9:syntax, 10:symbol,
   11:integer, 12:ratio, 13:real, 14:complex, 15:big_integer, 16:big_ratio, 17:big_real, 18:big_complex,
   19:string, 20:c_object, 21:vector, 22:int_vector, 23:float_vector, 24:byte_vector, 25:complex_vector,
   26:catch, 27:dynamic_wind, 28:hash_table, 29:let, 30:iterator,
   31:stack, 32:counter, 33:slot, 34:c_pointer, 35:output_port, 36:input_port, 37:random_state, 38:continuation, 39:goto,
   40:closure, 41:closure_star, 42:macro, 43:macro_star, 44:bacro, 45:bacro_star,
   46:c_macro, 47:c_function_star, 48:c_function, 49:c_rst_no_req_function,
   50:num_types
*/

typedef struct block_t {
  union {
    void *data;
    s7_pointer d_ptr;
    s7_int *i_ptr;
    s7_int tag;
  } dx;
  int32_t index;
  union {
    bool needs_free;
    uint32_t iter_or_size;
  } ln;
  union {
    s7_int size;
    s7_uint usize;
  } sz;
  union {
    struct block_t *next;
    char *documentation;
    s7_pointer ksym;
    s7_uint nx_uint;
    s7_int *ix_ptr;
    struct {
      uint32_t i1, i2;
    } ix;
  } nx;
  union {
    s7_pointer ex_ptr;
    void *ex_info;
    s7_int ckey;
  } ex;
} block_t;

#define NUM_BLOCK_LISTS 18
#define TOP_BLOCK_LIST 17
#define BLOCK_LIST 0

#define block_data(p)                    p->dx.data
#define block_index(p)                   p->index
#define block_set_index(p, Index)        p->index = Index
#define block_size(p)                    p->sz.size
#define block_set_size(p, Size)          p->sz.size = Size
#define block_next(p)                    p->nx.next
#define block_info(p)                    p->ex.ex_info

typedef block_t hash_entry_t; /* I think this means we waste 8 bytes per entry but can use the mallocate functions */
#define hash_entry_key(p)                p->dx.d_ptr
#define hash_entry_value(p)              (p)->ex.ex_ptr
#define hash_entry_set_value(p, Val)     p->ex.ex_ptr = Val
#define hash_entry_next(p)               block_next(p)
#define hash_entry_raw_hash(p)           p->sz.usize         /* block_size(p) */
#define hash_entry_set_raw_hash(p, Hash) p->sz.usize = Hash  /* block_set_size(p, Hash) */

typedef block_t vdims_t;
#define vdims_rank(p)                    p->sz.size
#define vector_elements_should_be_freed(p) p->ln.needs_free
#define vdims_dims(p)                    p->dx.i_ptr
#define vdims_offsets(p)                 p->nx.ix_ptr
#define vdims_original(p)                p->ex.ex_ptr


typedef enum {token_eof, token_left_paren, token_right_paren, token_dot, token_atom, token_quote, token_double_quote,
	      token_back_quote, token_comma, token_at_mark, token_sharp_const,
              token_vector, token_byte_vector, token_int_vector, token_float_vector, token_complex_vector} token_t;

typedef enum {no_article, indefinite_article} article_t;
typedef enum {dwind_init, dwind_body, dwind_finish} dwind_t;
enum {no_safety = 0, immutable_vector_safety, more_safety_warnings};  /* (*s7* 'safety) settings, if typedef'd becomes uint32_t (but we want -1) */

typedef enum {file_port, string_port, function_port} port_type_t;

typedef struct {
  int32_t (*read_character)(s7_scheme *sc, s7_pointer port);             /* function to read a character, int32_t for EOF */
  void (*write_character)(s7_scheme *sc, uint8_t c, s7_pointer port);    /* function to write a character */
  void (*write_string)(s7_scheme *sc, const char *str, s7_int len, s7_pointer port); /* function to write a string of known length */
  token_t (*read_semicolon)(s7_scheme *sc, s7_pointer port);             /* internal skip-to-semicolon reader */
  int32_t (*read_white_space)(s7_scheme *sc, s7_pointer port);           /* internal skip white space reader */
  s7_pointer (*read_name)(s7_scheme *sc, s7_pointer port);               /* internal get-next-name reader */
  s7_pointer (*read_sharp)(s7_scheme *sc, s7_pointer port);              /* internal get-next-sharp-constant reader */
  s7_pointer (*read_line)(s7_scheme *sc, s7_pointer port, bool eol_case);/* function to read a string up to \n */
  void (*displayer)(s7_scheme *sc, const char *s, s7_pointer port);      /* (display s pt) -- port_write_string without strlen?? */
  void (*close_port)(s7_scheme *sc, s7_pointer port);                    /* close-in|output-port */
} port_functions_t;

typedef struct {
  bool needs_free, is_closed;
  port_type_t ptype;
  FILE *file;
  char *filename;
  block_t *filename_block;
  uint32_t line_number, file_number;
  s7_int filename_length;
  block_t *block;
  s7_pointer orig_str;    /* GC protection for string port string or function port function */
  const port_functions_t *pf;
  s7_pointer (*input_function)(s7_scheme *sc, s7_read_t read_choice, s7_pointer port);
  void (*output_function)(s7_scheme *sc, uint8_t c, s7_pointer port);
} port_t;

typedef enum {o_d_v, o_d_vd, o_d_vdd, o_d_vid, o_d_id, o_d_7pi, o_d_7pii, o_d_7piid,
	      o_d_ip, o_d_pd, o_d_7p, o_d_7pid, o_d, o_d_d, o_d_dd, o_d_7dd, o_d_ddd, o_d_dddd,
	      o_i_i, o_i_7i, o_i_ii, o_i_7ii, o_i_iii, o_i_7pi, o_i_7pii, o_i_7piii, o_d_p,
	      o_b_p, o_b_7p, o_b_pp, o_b_7pp, o_b_pp_unchecked, o_b_pi, o_b_ii, o_b_7ii, o_b_dd,
	      o_p, o_p_p, o_p_ii, o_p_d, o_p_dd, o_i_7d, o_i_7p, o_d_7d, o_p_pp, o_p_ppp, o_p_pi, o_p_pi_unchecked,
	      o_p_ppi, o_p_i, o_p_pii, o_p_pip, o_p_pip_unchecked, o_p_piip, o_b_i, o_b_d} opt_func_t;

typedef struct opt_funcs_t {
  opt_func_t typ;
  void *func;
  struct opt_funcs_t *next;
} opt_funcs_t;

typedef struct {
  const char *name;
  int32_t name_length;
  uint32_t class_id;     /* can't use "class" -- confuses g++ */
  const char *doc;
  opt_funcs_t *opt_data; /* vunion-functions (see below) */
  s7_pointer generic_ff, setter, signature, pars, let;
  s7_pointer (*chooser)(s7_scheme *sc, s7_pointer f, int32_t args, s7_pointer expr);
  /* arg_defaults|names call_args only T_C_FUNCTION_STAR -- call args for GC protection */
  union {
    s7_pointer *arg_defaults;
    s7_pointer bool_setter;
  } dam;
  union {
    s7_pointer *arg_names;
    s7_pointer c_sym;
  } sam;
  union {
    s7_pointer call_args;
    void (*marker)(s7_pointer p, s7_int len);
  } cam;
} c_proc_t; /* 104 = sizeof(c_proc_t) */


typedef struct {
  s7_int type, outer_type;
  s7_pointer scheme_name, getter, setter;
  void (*mark)(void *val);
  void (*free)(void *value);
  bool (*eql)(void *val1, void *val2);
#if !DISABLE_DEPRECATED
  char *(*print)(s7_scheme *sc, void *value);
#endif
  s7_pointer (*equal)      (s7_scheme *sc, s7_pointer args);
  s7_pointer (*equivalent) (s7_scheme *sc, s7_pointer args);
  s7_pointer (*ref)        (s7_scheme *sc, s7_pointer args);
  s7_pointer (*set)        (s7_scheme *sc, s7_pointer args);
  s7_pointer (*length)     (s7_scheme *sc, s7_pointer args);
  s7_pointer (*reverse)    (s7_scheme *sc, s7_pointer args);
  s7_pointer (*copy)       (s7_scheme *sc, s7_pointer args);
  s7_pointer (*fill)       (s7_scheme *sc, s7_pointer args);
  s7_pointer (*to_list)    (s7_scheme *sc, s7_pointer args);
  s7_pointer (*to_string)  (s7_scheme *sc, s7_pointer args);
  s7_pointer (*gc_mark)    (s7_scheme *sc, s7_pointer args);
  s7_pointer (*gc_free)    (s7_scheme *sc, s7_pointer args);
} c_object_t;


typedef s7_uint (*hash_map_t)(s7_scheme *sc, s7_pointer table, s7_pointer key);        /* hash-table object->location mapper */
typedef hash_entry_t *(*hash_check_t)(s7_scheme *sc, s7_pointer table, s7_pointer key); /* hash-table object equality function */
static hash_map_t default_hash_map[NUM_TYPES];

typedef s7_int (*s7_i_7pi_t)(s7_scheme *sc, s7_pointer p, s7_int i1);
typedef s7_int (*s7_i_7pii_t)(s7_scheme *sc, s7_pointer p, s7_int i1, s7_int i2);
typedef s7_int (*s7_i_7piii_t)(s7_scheme *sc, s7_pointer p, s7_int i1, s7_int i2, s7_int i3);
typedef s7_int (*s7_i_iii_t)(s7_int i1, s7_int i2, s7_int i3);
typedef s7_int (*s7_i_7i_t)(s7_scheme *sc, s7_int i1);
typedef s7_int (*s7_i_7ii_t)(s7_scheme *sc, s7_int i1, s7_int i2);
typedef bool (*s7_b_pp_t)(s7_pointer p1, s7_pointer p2);
typedef bool (*s7_b_7pp_t)(s7_scheme *sc, s7_pointer p1, s7_pointer p2);
typedef bool (*s7_b_7p_t)(s7_scheme *sc, s7_pointer p1);
typedef bool (*s7_b_pi_t)(s7_scheme *sc, s7_pointer p1, s7_int i2);
typedef bool (*s7_b_d_t)(s7_double p1);
typedef bool (*s7_b_i_t)(s7_int p1);
typedef bool (*s7_b_ii_t)(s7_int p1, s7_int p2);
typedef bool (*s7_b_7ii_t)(s7_scheme *sc, s7_int p1, s7_int p2);
typedef bool (*s7_b_dd_t)(s7_double p1, s7_double p2);
typedef s7_pointer (*s7_p_t)(s7_scheme *sc);
typedef s7_pointer (*s7_p_ppi_t)(s7_scheme *sc, s7_pointer p1, s7_pointer p2, s7_int i1);
typedef s7_pointer (*s7_p_pi_t)(s7_scheme *sc, s7_pointer p1, s7_int i1);
typedef s7_pointer (*s7_p_pii_t)(s7_scheme *sc, s7_pointer p1, s7_int i1, s7_int i2);
typedef s7_pointer (*s7_p_pip_t)(s7_scheme *sc, s7_pointer p1, s7_int i1, s7_pointer p2);
typedef s7_pointer (*s7_p_piip_t)(s7_scheme *sc, s7_pointer p1, s7_int i1, s7_int i2, s7_pointer p3);
typedef s7_pointer (*s7_p_i_t)(s7_scheme *sc, s7_int i);
typedef s7_pointer (*s7_p_ii_t)(s7_scheme *sc, s7_int i1, s7_int i2);
typedef s7_pointer (*s7_p_dd_t)(s7_scheme *sc, s7_double x1, s7_double x2);
typedef s7_double (*s7_d_7d_t)(s7_scheme *sc, s7_double p1);
typedef s7_double (*s7_d_7dd_t)(s7_scheme *sc, s7_double p1, s7_double p2);
typedef s7_double (*s7_d_7p_t)(s7_scheme *sc, s7_pointer p1);
typedef s7_double (*s7_d_7pii_t)(s7_scheme *sc, s7_pointer p1, s7_int i1, s7_int i2);
typedef s7_double (*s7_d_7piid_t)(s7_scheme *sc, s7_pointer p1, s7_int i1, s7_int i2, s7_double x1);

typedef struct opt_info opt_info;

typedef union {
  s7_int i;
  s7_double x;
  s7_pointer p;
  void *gen;
  opt_info *o1;
  s7_function call;
  s7_double (*d_f)(void);
  s7_double (*d_d_f)(s7_double x);
  s7_double (*d_7d_f)(s7_scheme *sc, s7_double x);
  s7_double (*d_dd_f)(s7_double x1, s7_double x2);
  s7_double (*d_7dd_f)(s7_scheme *sc, s7_double x1, s7_double x2);
  s7_double (*d_ddd_f)(s7_double x1, s7_double x2, s7_double x3);
  s7_double (*d_dddd_f)(s7_double x1, s7_double x2, s7_double x3, s7_double x4);
  s7_double (*d_v_f)(void *obj);
  s7_double (*d_vd_f)(void *obj, s7_double fm);
  s7_double (*d_vdd_f)(void *obj, s7_double x1, s7_double x2);
  s7_double (*d_vid_f)(void *obj, s7_int i, s7_double fm);
  s7_double (*d_id_f)(s7_int i, s7_double fm);
  s7_double (*d_7pi_f)(s7_scheme *sc, s7_pointer obj, s7_int i1);
  s7_double (*d_7pid_f)(s7_scheme *sc, s7_pointer obj, s7_int i1, s7_double x);
  s7_double (*d_7pii_f)(s7_scheme *sc, s7_pointer obj, s7_int i1, s7_int i2);
  s7_double (*d_7piid_f)(s7_scheme *sc, s7_pointer obj, s7_int i1, s7_int i2, s7_double x);
  s7_double (*d_ip_f)(s7_int i1, s7_pointer p);
  s7_double (*d_pd_f)(s7_pointer obj, s7_double x);
  s7_double (*d_p_f)(s7_pointer p);
  s7_double (*d_7p_f)(s7_scheme *sc, s7_pointer p);
  s7_int (*i_7d_f)(s7_scheme *sc, s7_double i1);
  s7_int (*i_7p_f)(s7_scheme *sc, s7_pointer i1);
  s7_int (*i_i_f)(s7_int i1);
  s7_int (*i_7i_f)(s7_scheme *sc, s7_int i1);
  s7_int (*i_ii_f)(s7_int i1, s7_int i2);
  s7_int (*i_7ii_f)(s7_scheme *sc, s7_int i1, s7_int i2);
  s7_int (*i_iii_f)(s7_int i1, s7_int i2, s7_int i3);
  s7_int (*i_7pi_f)(s7_scheme *sc, s7_pointer p, s7_int i1);
  s7_int (*i_7pii_f)(s7_scheme *sc, s7_pointer p, s7_int i1, s7_int i2);
  s7_int (*i_7piii_f)(s7_scheme *sc, s7_pointer p, s7_int i1, s7_int i2, s7_int i3);
  bool (*b_i_f)(s7_int p);
  bool (*b_d_f)(s7_double p);
  bool (*b_p_f)(s7_pointer p);
  bool (*b_pp_f)(s7_pointer p1, s7_pointer p2);
  bool (*b_7pp_f)(s7_scheme *sc, s7_pointer p1, s7_pointer p2);
  bool (*b_7p_f)(s7_scheme *sc, s7_pointer p1);
  bool (*b_pi_f)(s7_scheme *sc, s7_pointer p1, s7_int i2);
  bool (*b_ii_f)(s7_int i1, s7_int i2);
  bool (*b_7ii_f)(s7_scheme *sc, s7_int i1, s7_int i2);
  bool (*b_dd_f)(s7_double x1, s7_double x2);
  s7_pointer (*p_f)(s7_scheme *sc);
  s7_pointer (*p_p_f)(s7_scheme *sc, s7_pointer p);
  s7_pointer (*p_pp_f)(s7_scheme *sc, s7_pointer p1, s7_pointer p2);
  s7_pointer (*p_ppp_f)(s7_scheme *sc, s7_pointer p, s7_pointer p2, s7_pointer p3);
  s7_pointer (*p_pi_f)(s7_scheme *sc, s7_pointer p1, s7_int i1);
  s7_pointer (*p_pii_f)(s7_scheme *sc, s7_pointer p1, s7_int i1, s7_int i2);
  s7_pointer (*p_ppi_f)(s7_scheme *sc, s7_pointer p1, s7_pointer p2, s7_int i1);
  s7_pointer (*p_pip_f)(s7_scheme *sc, s7_pointer p1, s7_int i1, s7_pointer p2);
  s7_pointer (*p_piip_f)(s7_scheme *sc, s7_pointer p1, s7_int i1, s7_int i2, s7_pointer p3);
  s7_pointer (*p_i_f)(s7_scheme *sc, s7_int i);
  s7_pointer (*p_ii_f)(s7_scheme *sc, s7_int x1, s7_int x2);
  s7_pointer (*p_d_f)(s7_scheme *sc, s7_double x);
  s7_pointer (*p_dd_f)(s7_scheme *sc, s7_double x1, s7_double x2);
  s7_double (*fd)(opt_info *o);
  s7_int (*fi)(opt_info *o);
  bool (*fb)(opt_info *o);
  s7_pointer (*fp)(opt_info *o);
} vunion;
/* libgsl 15 d_i */

#define num_vunions 15
struct opt_info {
  vunion v[num_vunions];
  s7_scheme *sc;
};

#define q_temp(o) o->v[num_vunions - 1]



typedef intptr_t opcode_t;

typedef struct unlet_entry_t {
  s7_pointer symbol;
  struct unlet_entry_t *next;
} unlet_entry_t;


/* -------------------------------- cell structure -------------------------------- */

typedef struct s7_cell {
  union {
    s7_uint u64_type;             /* type info */
    s7_int s64_type;
    uint8_t type_field;
    struct {
      uint16_t low_bits;          /* 8 bits for type (type_field above, pair?/string? etc, 6 bits in use), 8 flag bits */
      uint16_t mid_bits;          /* 16 more flag bits */
      uint16_t opt_bits;          /* 16 bits for opcode_t (eval choice), 10 in use) */
      uint16_t high_bits;         /* 16 more flag bits */
    } bits;
  } tf;
  union {

    union {
      s7_int integer_value;       /* integers */
      s7_double real_value;       /* floats */

      struct {                    /* ratios */
	s7_int numerator;
	s7_int denominator;
      } fraction_value;

      union {
#if !WITH_CLANG_PP
	s7_complex z;
#endif
	struct {                  /* complex numbers */
	  s7_double rl;
	  s7_double im;
	} complex_value;
      } cz;


    } number;

    struct {                      /* ports */
      port_t *port;
      uint8_t *data;
      s7_int size, point;
      block_t *block;
    } prt;

    struct{                       /* characters */
      uint32_t c, up_c;
      int32_t length;
      bool alpha_c, digit_c, space_c, upper_c, lower_c;
      char c_name[12];
    } chr;

    struct {                      /* c-pointers */
      void *c_pointer;
      s7_pointer c_type, info, weak1, weak2;
    } cptr;

    struct {                      /* vectors */
      s7_int length;
      union {
	s7_pointer *objects;
	s7_int *ints;
	s7_double *floats;
	s7_complex *complexes;
	uint8_t *bytes;
      } elements;
      block_t *block;
      s7_pointer (*vget)(s7_scheme *sc, s7_pointer vec, s7_int loc);
      union {
	s7_pointer (*vset)(s7_scheme *sc, s7_pointer vec, s7_int loc, s7_pointer val);
	s7_pointer fset;
      } setv;
    } vector;

    struct {                        /* stacks (internal) struct must match vector above for length/objects */
      s7_int length;
      s7_pointer *objects;
      block_t *block;
      s7_int top, flags;
    } stk;

    struct {                        /* hash-tables */
      s7_uint mask;
      hash_entry_t **elements;      /* a pointer into block below: takes up a field in object.hasher but is faster (50 in thash) */
      hash_check_t hash_func;
      hash_map_t *loc;
      block_t *block;
    } hasher;

    struct {                        /* iterators */
      s7_pointer seq, cur;
      union {
	s7_int loc;
	s7_pointer slot;            /* let iterator current slow */
      } lc;
      union {
	s7_int len;
	s7_pointer slow;            /* pair iterator cycle check */
	hash_entry_t *entry;        /* hash-table iterator current entry */
      } lw;
      s7_pointer (*next)(s7_scheme *sc, s7_pointer iterator);
    } iter;

    struct {
      c_proc_t *c_proc;             /* C functions, macros */
      s7_function ff;
      s7_int required_args, optional_args, all_args; /* these could be uint32_t */
    } fnc;

    struct {                        /* pairs */
      s7_pointer car, cdr, opt1;
      union
      {
	s7_pointer opt2;
	s7_int n;
      } o2;
      union {
	s7_pointer opt3;
	s7_int n;
	uint8_t opt_type;
      } o3;
    } cons;

    struct {                       /* special purpose pairs (symbol-table etc) */
      s7_pointer unused_car, unused_cdr;
      s7_uint hash;
      const char *fstr;
      s7_uint location;            /* line/file/position, also used in symbol_table as raw_len */
    } sym_cons;

    struct {                       /* scheme functions */
      s7_pointer args, body, let, setter; /* args can be a symbol, as well as a list, setter can be #f as well as a procedure/closure */
      int32_t arity;
    } func;

    struct {                       /* strings */
      s7_int length;
      char *svalue;
      s7_uint hash;                /* string hash-index */
      block_t *block;
      block_t *gensym_block;
    } string;

    struct {                       /* symbols */
      s7_pointer name, global_slot, local_slot;
      s7_int id;                   /* which let last bound the symbol -- for faster symbol lookup */
      uint32_t ctr;                /* how many times has symbol been bound */
      uint32_t small_symbol_tag;   /* symbol as member of a (small) set (tree-set-memq etc), assumed to be uint32_t in clear_small_symbol_set */
    } sym;

    struct {                       /* syntax */
      s7_pointer symbol;
      opcode_t op;
      int32_t min_args, max_args;
      const char *documentation;
      /* 1 unused */
    } syn;

    struct {                       /* slots (bindings) */
      s7_pointer sym, val, nxt, pending_value, expr;  /* pending_value is also the setter field which works by a whisker */
    } slt;

    struct {                       /* lets (environments) */
      s7_pointer slots, nxt;
      s7_int id;                   /* id of rootlet is -1 */
      union {
	struct {
	  s7_pointer function;     /* *function* (symbol) if this is a funclet */
	  uint32_t line, file;     /* *function* location if it is known */
	} efnc;
	struct {
	  s7_pointer dox1, dox2;   /* do loop variables */
	} dox;
	s7_int key;                /* sc->baffle_ctr type */
      } edat;
    } let;

    struct {                       /* special stuff like #<unspecified> */
      s7_pointer car, cdr;         /* unique_car|cdr, for sc->nil these are sc->unspecified for faster assoc etc */
      s7_int unused_let_id;
      const char *name;
      s7_int len;
    } unq;

    struct {                        /* #<...> */
      char *name;                   /* not const because the GC frees it */
      s7_int len;
      /* 3 unused */
    } undef;

    struct {                        /* #<eof> */
      const char *name;
      s7_int len;
      /* 3 unused */
    } eof;

    struct {                        /* counter (internal) */
      s7_pointer result, list, let, slots; /* let = counter_let (curlet after map/for-each let created) */
      s7_uint cap;                 /* sc->capture_let_counter for let reuse */
    } ctr;

    struct {                        /* random-state */
      s7_uint seed, carry;
      /* for 64-bit floats we probably need 4 state fields */
    } rng;

    struct {                        /* additional object types (C) */
      s7_int type;
      void *value;                  /* the value the caller associates with the c_object */
      s7_pointer let;               /* the method list, if any (openlet) */
      s7_scheme *sc;
      /* 1 unused */
    } c_obj;

    struct {                        /* continuations */
      block_t *block;
      s7_pointer stack, op_stack;
      s7_pointer *stack_start, *stack_end;
    } cwcc;

    struct {                        /* call-with-exit */
      s7_uint goto_loc, op_stack_loc;
      bool active;
      s7_pointer name;
      /* 1 unused */
    } rexit;

    struct {                        /* catch */
      s7_uint goto_loc, op_stack_loc;
      s7_pointer tag;
      s7_pointer handler;
      Jmp_Buf *cstack;
    } rcatch; /* C++ reserves "catch" I guess */

    struct {                       /* dynamic-wind */
      s7_pointer in, out, body;
      dwind_t state;
      /* 1 unused */
    } winder;
  } object;

#if S7_DEBUGGING
  int32_t alloc_line, uses, explicit_free_line, gc_line, holders, carrier_line;
  s7_int alloc_type, debugger_bits;
  const char *alloc_func, *gc_func, *root;
  s7_pointer holder;
#endif
} s7_cell;


typedef struct s7_big_cell {
  s7_cell cell;
  s7_int big_hloc;
} s7_big_cell;
typedef struct s7_big_cell *s7_big_pointer;

typedef struct heap_block_t {
  intptr_t start, end;
  s7_int offset;
  struct heap_block_t *next;
} heap_block_t;

typedef struct {
  s7_pointer *objs;
  int32_t size, top, ref, size2;
  bool has_hits;
  int32_t *refs;
  s7_pointer cycle_port, init_port;
  s7_int cycle_loc, init_loc, ctr;
  bool *defined;
} shared_info_t;

typedef struct gc_obj_t {
  s7_pointer p;
  struct gc_obj_t *nxt;
} gc_obj_t;

typedef struct {
  s7_pointer *list;
  s7_int size, loc;
} gc_list_t;

typedef struct {
  s7_int size, top, excl_size, excl_top;
  s7_pointer *funcs, *let_names, *files;
  s7_int *timing_data, *excl, *lines;
} profile_data_t;

typedef enum {no_jump, call_with_exit_jump, throw_jump, catch_jump, error_jump, error_quit_jump} jump_loc_t;
typedef enum {no_set_jump, read_set_jump, load_set_jump, dynamic_wind_set_jump, s7_call_set_jump, eval_set_jump} setjmp_loc_t;
static const char *jump_string[6] = {"no_jump", "call_with_exit_jump", "throw_jump", "catch_jump", "error_jump", "error_quit_jump"};


/* -------------------------------- s7_scheme struct -------------------------------- */
struct s7_scheme {
  s7_pointer code;    /* layout of first 4 entries should match stack frame layout */
  s7_pointer curlet;
  s7_pointer args;
  opcode_t cur_op;

  s7_pointer value, cur_code;
  s7_pointer nil;                     /* empty list */
  s7_pointer T;                       /* #t */
  s7_pointer F;                       /* #f */
  s7_pointer undefined;               /* #<undefined> */
  s7_pointer unspecified;             /* #<unspecified> */
  s7_pointer no_value;                /* the (values) value */
  s7_pointer unused;                  /* a marker for an unoccupied slot in sc->protected_objects (and other similar stuff) */

  s7_pointer stack;                   /* stack is a vector */
  uint32_t stack_size;
  s7_pointer *stack_start, *stack_end, *stack_resize_trigger;

  s7_pointer *op_stack, *op_stack_now, *op_stack_end;
  uint32_t op_stack_size, max_stack_size;

  s7_cell **heap, **free_heap, **free_heap_top, **free_heap_trigger, **previous_free_heap_top;
  s7_int heap_size, gc_freed, gc_total_freed, max_heap_size, gc_temps_size;
  s7_double gc_resize_heap_fraction, gc_resize_heap_by_4_fraction;
  s7_int gc_calls, gc_total_time, gc_start, gc_end, gc_true_calls, gc_true_total_time;
  heap_block_t *heap_blocks;

#if WITH_HISTORY
  s7_pointer eval_history1, eval_history2, error_history, history_sink, history_pairs, old_cur_code;
  bool using_history1;
#endif

#if WITH_MULTITHREAD_CHECKS
  int32_t lock_count;
  pthread_mutex_t lock;
#endif

  gc_obj_t *semipermanent_objects, *semipermanent_lets;
  s7_pointer protected_objects, protected_setters, protected_setter_symbols;  /* vectors of gc-protected objects */
  s7_int *protected_objects_free_list;    /* to avoid a linear search for a place to store an object in sc->protected_objects */
  s7_int protected_objects_size, protected_setters_size, protected_setters_loc;
  s7_int protected_objects_free_list_loc;

  s7_pointer symbol_table;
  s7_pointer rootlet, rootlet_slots, shadow_rootlet;
  unlet_entry_t *unlet_entries;       /* original bindings of predefined functions */

  s7_pointer input_port;              /* current-input-port */
  s7_pointer *input_port_stack;       /*   input port stack (load and read internally) */
  uint32_t input_port_stack_size, input_port_stack_loc;

  s7_pointer output_port;             /* current-output-port */
  s7_pointer error_port;              /* current-error-port */
  s7_pointer owlet;                   /* owlet */
  s7_pointer error_type, error_data, error_code, error_line, error_file, error_position; /* owlet slots */
  s7_pointer standard_input, standard_output, standard_error;

  s7_pointer sharp_readers;           /* the binding pair for the global *#readers* list */
  s7_pointer load_hook;               /* *load-hook* hook object */
  s7_pointer autoload_hook;           /* *autoload-hook* hook object */
  s7_pointer unbound_variable_hook;   /* *unbound-variable-hook* hook object */
  s7_pointer missing_close_paren_hook, rootlet_redefinition_hook;
  s7_pointer error_hook, read_error_hook; /* *error-hook* hook object, and *read-error-hook* */
  s7_pointer exit_hook;                 /* *exit-hook* hook object */
  token_t tok;
  bool gc_off, gc_in_progress;        /* gc_off: if true, the GC won't run */
  uint32_t gc_stats, gensym_counter, f_class, add_class, multiply_class, subtract_class, num_eq_class;
  int32_t format_column, error_argnum;
  s7_uint capture_let_counter;
  bool short_print, is_autoloading, in_with_let, object_out_locked, has_openlets, is_expanding, accept_all_keyword_arguments;
  bool got_tc, got_rec, not_tc, muffle_warnings, symbol_quote, reset_error_hook;
  s7_int rec_tc_args;
  s7_int let_number;
  unsigned char number_separator;
  s7_double default_rationalize_error, equivalent_float_epsilon, hash_table_float_epsilon;
  s7_int default_hash_table_length, initial_string_port_length, print_length, objstr_max_len, history_size, true_history_size, output_file_port_length;
  s7_int max_vector_length, max_string_length, max_list_length, max_vector_dimensions, max_string_port_length, rec_loc, rec_len, max_show_stack_frames;
  s7_pointer stacktrace_defaults, symbol_printer, do_body_p, iterator_at_end_value, scheme_version;

  s7_pointer rec_stack, rec_testp, rec_f1p, rec_f2p, rec_f3p, rec_f4p, rec_f5p, rec_f6p, rec_f7p, rec_f8p;
  s7_pointer rec_resp, rec_slot1, rec_slot2, rec_slot3, rec_p1, rec_p2;
  s7_pointer *rec_els;
  s7_function rec_testf, rec_f1f, rec_f2f, rec_f3f, rec_f4f, rec_f5f, rec_f6f, rec_f7f, rec_f8f, rec_resf, rec_fn;
  s7_int (*rec_fi1)(opt_info *o);
  s7_int (*rec_fi2)(opt_info *o);
  s7_int (*rec_fi3)(opt_info *o);
  s7_int (*rec_fi4)(opt_info *o);
  s7_int (*rec_fi5)(opt_info *o);
  s7_int (*rec_fi6)(opt_info *o);
  bool (*rec_fb1)(opt_info *o);
  bool (*rec_fb2)(opt_info *o);

  opt_info *rec_test_o, *rec_result_o, *rec_a1_o, *rec_a2_o, *rec_a3_o, *rec_a4_o, *rec_a5_o, *rec_a6_o;
  s7_i_ii_t rec_i_ii_f;
  s7_d_dd_t rec_d_dd_f;
  s7_pointer rec_val1, rec_val2;
  bool rec_bool;

  int32_t float_format_precision;
  vdims_t *wrap_only;

  char *typnam;
  int32_t typnam_len, print_width;
  s7_pointer *singletons;
  block_t *unentry;                   /* hash-table lookup failure indicator */

  #define INITIAL_FILE_NAMES_SIZE 8
  s7_pointer *file_names;
  int32_t file_names_size, file_names_top;

  #define INITIAL_STRBUF_SIZE 1024
  s7_int strbuf_size;
  char *strbuf;

  char *read_line_buf;
  s7_int read_line_buf_size;

  s7_pointer v, w, x, y, z;
  s7_pointer temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8, temp9, read_dims;
  s7_pointer t1_1, t2_1, t2_2, t3_1, t3_2, t3_3, t4_1, u1_1;
  s7_pointer elist_1, elist_2, elist_3, elist_4, elist_5, elist_6, elist_7;
  s7_pointer plist_1, plist_2, plist_2_2, plist_3, plist_4;
  s7_pointer qlist_2, qlist_3, clist_1, clist_2, dlist_1, mlist_1, mlist_2; /* dlist|clist and ulist must not overlap */

  Jmp_Buf *goto_start;
  bool longjmp_ok;
  setjmp_loc_t setjmp_loc;

  void (*begin_hook)(s7_scheme *sc, bool *val);
  opcode_t begin_op;

  bool debug_or_profile, profiling_gensyms;
  s7_int current_line, s7_call_line, debug, profile, profile_position;
  s7_pointer profile_prefix;
  profile_data_t *profile_data;
  const char *current_file, *s7_call_file, *s7_call_name;

  shared_info_t *circle_info;
  format_data_t **fdats;
  int32_t num_fdats, safety;
  gc_list_t *strings, *vectors, *input_ports, *output_ports, *input_string_ports, *continuations, *c_objects, *hash_tables;
  gc_list_t *gensyms, *undefineds, *multivectors, *weak_refs, *weak_hash_iterators, *opt1_funcs;

  s7_pointer *setters;
  s7_int setters_size, setters_loc;
  s7_pointer *tree_pointers;
  int32_t tree_pointers_size, tree_pointers_top, semipermanent_cells, num_to_str_size;
  s7_pointer format_ports;
  uint32_t alloc_pointer_k, alloc_function_k, alloc_symbol_k, alloc_big_pointer_k;
  s7_cell *alloc_pointer_cells;
  c_proc_t *alloc_function_cells;
  s7_big_cell *alloc_big_pointer_cells;
  s7_pointer string_wrappers, integer_wrappers, real_wrappers, complex_wrappers, c_pointer_wrappers, let_wrappers, slot_wrappers;
  uint8_t *alloc_symbol_cells;
  char *num_to_str;

  block_t *block_lists[NUM_BLOCK_LISTS];
  size_t alloc_string_k;
  char *alloc_string_cells;

  c_object_t **c_object_types;
  int32_t c_object_types_size, num_c_object_types;
  s7_pointer type_to_typers[NUM_TYPES];

  s7_int big_symbol_tag;
  uint32_t small_symbol_tag;
#if S7_DEBUGGING
  int32_t big_symbol_set_line, small_symbol_set_line, big_symbol_set_state, small_symbol_set_state, y_line, v_line, x_line, t_line;
  const char *big_symbol_set_func, *small_symbol_set_func;
#endif
  s7_int baffle_ctr, map_call_ctr;
  s7_pointer default_random_state;

  s7_pointer sort_body, sort_begin, sort_v1, sort_v2;
  opcode_t sort_op;
  s7_int sort_body_len;
  s7_b_7pp_t sort_f;
  opt_info *sort_o;
  bool (*sort_fb)(opt_info *o);

  #define INT_TO_STR_SIZE 32
  char int_to_str1[INT_TO_STR_SIZE], int_to_str2[INT_TO_STR_SIZE], int_to_str3[INT_TO_STR_SIZE], int_to_str4[INT_TO_STR_SIZE], int_to_str5[INT_TO_STR_SIZE];

  s7_pointer abs_symbol, acos_symbol, acosh_symbol, add_symbol, angle_symbol, append_symbol, apply_symbol, apply_values_symbol, arity_symbol,
             ash_symbol, asin_symbol, asinh_symbol, assoc_symbol, assq_symbol, assv_symbol, atan_symbol, atanh_symbol, autoload_symbol, autoloader_symbol,
             bacro_symbol, bacro_star_symbol, byte_vector_symbol, byte_vector_ref_symbol, byte_vector_set_symbol, byte_vector_to_string_symbol,
             c_pointer_symbol, c_pointer_info_symbol, c_pointer_to_list_symbol, c_pointer_type_symbol, c_pointer_weak1_symbol, c_pointer_weak2_symbol,
             caaaar_symbol, caaadr_symbol, caaar_symbol, caadar_symbol, caaddr_symbol, caadr_symbol,
             caar_symbol, cadaar_symbol, cadadr_symbol, cadar_symbol, caddar_symbol, cadddr_symbol, caddr_symbol, cadr_symbol,
             call_cc_symbol, call_with_current_continuation_symbol, call_with_exit_symbol, call_with_input_file_symbol,
             call_with_input_string_symbol, call_with_output_file_symbol, call_with_output_string_symbol, car_symbol,
             catch_symbol, cdaaar_symbol, cdaadr_symbol, cdaar_symbol, cdadar_symbol, cdaddr_symbol, cdadr_symbol, cdar_symbol,
             cddaar_symbol, cddadr_symbol, cddar_symbol, cdddar_symbol, cddddr_symbol, cdddr_symbol, cddr_symbol, cdr_symbol,
             ceiling_symbol, char_eq_symbol, char_geq_symbol, char_gt_symbol, char_leq_symbol, char_lt_symbol,
             char_position_symbol, char_to_integer_symbol, cload_directory_symbol, close_input_port_symbol,
             close_output_port_symbol, complex_symbol, complex_vector_ref_symbol, complex_vector_set_symbol, complex_vector_symbol,
             cond_expand_symbol, cons_symbol, copy_symbol, cos_symbol, cosh_symbol, coverlet_symbol,
             curlet_symbol, current_error_port_symbol, current_input_port_symbol, current_output_port_symbol, cutlet_symbol, cyclic_sequences_symbol,
             denominator_symbol, dilambda_symbol, display_symbol, divide_symbol, documentation_symbol, dynamic_wind_symbol, dynamic_unwind_symbol,
             num_eq_symbol, error_symbol, eval_string_symbol, eval_symbol, exact_to_inexact_symbol, exit_symbol, exp_symbol, expt_symbol,
             features_symbol, file__symbol, fill_symbol, float_vector_ref_symbol, float_vector_set_symbol, float_vector_symbol, floor_symbol,
             flush_output_port_symbol, for_each_symbol, format_symbol, funclet_symbol, _function__symbol, procedure_arglist_symbol,
             gc_symbol, gcd_symbol, gensym_symbol, geq_symbol, get_output_string_symbol, gt_symbol,
             hash_table_size_symbol, hash_table_key_typer_symbol, hash_table_ref_symbol, hash_table_set_symbol, hash_table_symbol,
             hash_table_value_typer_symbol, help_symbol, hook_functions_symbol,
             imag_part_symbol, immutable_symbol, inexact_to_exact_symbol, inlet_symbol, int_vector_ref_symbol, int_vector_set_symbol, int_vector_symbol,
             integer_decode_float_symbol, integer_to_char_symbol,
             is_aritable_symbol, is_boolean_symbol, is_byte_symbol, is_byte_vector_symbol,
             is_c_object_symbol, c_object_let_symbol, c_object_type_symbol, is_c_pointer_symbol,
             is_char_alphabetic_symbol, is_char_symbol, is_char_whitespace_symbol,
             is_complex_symbol, is_complex_vector_symbol, is_constant_symbol,
             is_continuation_symbol, is_defined_symbol, is_dilambda_symbol, is_eof_object_symbol, is_eq_symbol, is_equal_symbol,
             is_eqv_symbol, is_even_symbol, is_exact_symbol, is_float_vector_symbol, is_funclet_symbol,
             is_gensym_symbol, is_goto_symbol, is_hash_table_symbol, is_immutable_symbol,
             is_inexact_symbol, is_infinite_symbol, is_input_port_symbol, is_int_vector_symbol, is_integer_symbol, is_iterator_symbol,
             is_keyword_symbol, is_let_symbol, is_list_symbol, is_macro_symbol, is_equivalent_symbol, is_nan_symbol, is_negative_symbol,
             is_null_symbol, is_number_symbol, is_odd_symbol, is_openlet_symbol, is_output_port_symbol, is_pair_symbol,
             is_port_closed_symbol, is_positive_symbol, is_procedure_symbol, is_proper_list_symbol, is_provided_symbol,
             is_random_state_symbol, is_rational_symbol, is_real_symbol, is_sequence_symbol, is_string_symbol, is_subvector_symbol,
             is_symbol_symbol, is_syntax_symbol, is_vector_symbol, is_weak_hash_table_symbol, is_zero_symbol,
             is_float_symbol, is_integer_or_real_at_end_symbol, is_integer_or_any_at_end_symbol, is_integer_or_number_at_end_symbol,
             is_unspecified_symbol, is_undefined_symbol,
             iterate_symbol, iterator_is_at_end_symbol, iterator_sequence_symbol,
             keyword_to_symbol_symbol,
             lcm_symbol, length_symbol, leq_symbol, let_ref_fallback_symbol, let_ref_symbol, let_set_fallback_symbol,
             let_set_symbol, let_temporarily_symbol, libraries_symbol, list_ref_symbol, list_set_symbol, list_symbol, list_tail_symbol, list_values_symbol,
             load_path_symbol, load_symbol, log_symbol, logand_symbol, logbit_symbol, logior_symbol, lognot_symbol, logxor_symbol, lt_symbol,
             local_documentation_symbol, local_signature_symbol, local_setter_symbol, local_iterator_symbol,
             macro_symbol, macro_star_symbol, magnitude_symbol,
             make_byte_vector_symbol, make_complex_vector_symbol, make_float_vector_symbol, make_hash_table_symbol,
             make_weak_hash_table_symbol, make_int_vector_symbol, make_iterator_symbol, make_list_symbol, make_string_symbol,
             make_vector_symbol, map_symbol, max_symbol, member_symbol, memq_symbol, memv_symbol, min_symbol, modulo_symbol, multiply_symbol,
             name_symbol, nan_symbol, nan_payload_symbol, newline_symbol, not_symbol, number_to_string_symbol, numerator_symbol,
             object_to_string_symbol, object_to_let_symbol, open_input_file_symbol, open_input_function_symbol, open_input_string_symbol,
             open_output_file_symbol, open_output_function_symbol, open_output_string_symbol, openlet_symbol, outlet_symbol, owlet_symbol,
             pair_filename_symbol, pair_line_number_symbol, peek_char_symbol, pi_symbol, port_filename_symbol, port_line_number_symbol,
             port_file_symbol, port_position_symbol, port_string_symbol, procedure_source_symbol, provide_symbol,
             qq_append_symbol, quotient_symbol,
             random_state_symbol, random_state_to_list_symbol, random_symbol, rationalize_symbol, read_byte_symbol,
             read_char_symbol, read_line_symbol, read_string_symbol, read_symbol, reader_cond_symbol, real_part_symbol, remainder_symbol,
             require_symbol, reverse_symbol, reverseb_symbol, rootlet_symbol, round_symbol,
             setter_symbol, set_car_symbol, set_cdr_symbol,
             set_current_error_port_symbol, set_current_input_port_symbol, set_current_output_port_symbol,
             signature_symbol, sin_symbol, sinh_symbol, sort_symbol, sqrt_symbol,
             stacktrace_symbol, string_append_symbol, string_copy_symbol, string_eq_symbol, string_fill_symbol,
             string_geq_symbol, string_gt_symbol, string_leq_symbol, string_lt_symbol, string_position_symbol, string_ref_symbol,
             string_set_symbol, string_symbol, string_to_keyword_symbol, string_to_number_symbol, string_to_symbol_symbol,
             sublet_symbol, substring_symbol, substring_uncopied_symbol, subtract_symbol, subvector_symbol, subvector_position_symbol, subvector_vector_symbol,
             symbol_symbol, symbol_to_dynamic_value_symbol, symbol_initial_value_symbol,
             symbol_to_keyword_symbol, symbol_to_string_symbol, symbol_to_value_symbol,
             tan_symbol, tanh_symbol, throw_symbol, string_to_byte_vector_symbol,
             tree_count_symbol, tree_leaves_symbol, tree_memq_symbol, tree_set_memq_symbol, tree_is_cyclic_symbol, truncate_symbol, type_of_symbol,
             unlet_symbol,
             values_symbol, varlet_symbol, vector_append_symbol, vector_dimension_symbol, vector_dimensions_symbol, vector_fill_symbol,
             vector_rank_symbol, vector_ref_symbol, vector_set_symbol, vector_symbol, vector_typer_symbol,
             weak_hash_table_symbol, with_input_from_file_symbol, with_input_from_string_symbol, with_output_to_file_symbol, with_output_to_string_symbol,
             write_byte_symbol, write_char_symbol, write_string_symbol, write_symbol;
  s7_pointer hash_code_symbol, dummy_equal_hash_table, features_setter;
#if !WITH_PURE_S7
  s7_pointer integer_length_symbol,
             is_char_ready_symbol, let_to_list_symbol, list_to_string_symbol, list_to_vector_symbol, make_polar_symbol, string_length_symbol,
             string_to_list_symbol, vector_length_symbol, vector_to_list_symbol;
#endif
#if WITH_R7RS
  s7_pointer clock_gettime_symbol, uname_symbol;
#endif
  bool r7rs_inited;
  s7_pointer s7_symbol, r5rs_symbol, r7rs_symbol, global_is_eq, initial_is_eq, global_memq, initial_memq, global_assq, initial_assq;

  /* syntax symbols et al */
  s7_pointer allow_other_keys_keyword, and_symbol, anon_symbol, autoload_error_symbol, bad_result_symbol, baffled_symbol, begin_symbol, body_symbol, case_symbol,
             class_name_symbol, cond_symbol, define_bacro_star_symbol, define_bacro_symbol, define_constant_symbol, define_expansion_star_symbol,
             define_expansion_symbol, define_macro_star_symbol, define_macro_symbol, define_star_symbol, define_symbol, display_keyword,
             division_by_zero_symbol, do_symbol, else_symbol, feed_to_symbol, format_error_symbol, if_keyword, if_symbol, immutable_error_symbol,
             invalid_exit_function_symbol, io_error_symbol, lambda_star_symbol, lambda_symbol, let_star_symbol, let_symbol,
             letrec_star_symbol, letrec_symbol, macroexpand_symbol, missing_method_symbol, no_setter_symbol, number_to_real_symbol, or_symbol,
             out_of_memory_symbol, out_of_range_symbol, profile_in_symbol, quasiquote_function, quasiquote_symbol, quote_function, quote_symbol,
             read_error_symbol, readable_keyword, rest_keyword, set_symbol, string_read_error_symbol, symbol_table_symbol,
             syntax_error_symbol, trace_in_symbol, type_symbol, unbound_variable_symbol, unless_symbol,
             unquote_symbol, value_symbol, when_symbol, with_baffle_symbol, with_let_symbol, write_keyword,
             wrong_number_of_args_symbol, wrong_type_arg_symbol;

  /* signatures of sequences used as applicable objects: ("hi" 1) */
  s7_pointer  byte_vector_signature, c_object_signature, float_vector_signature, hash_table_signature, int_vector_signature,
             let_signature, pair_signature, string_signature, vector_signature, complex_vector_signature;
  /* common signatures */
  s7_pointer pcl_bc, pcl_bs, pcl_bt, pcl_c, pcl_f, pcl_i, pcl_n, pcl_r, pcl_s, pcl_v, pl_bc, pl_bn, pl_bt, pl_p, pl_sf, pl_tl, pl_nn;

  /* optimizer s7_functions */
  s7_pointer add_1x, add_2, add_3, add_4, add_i_random, add_x1, append_2, ash_ic, ash_ii, bv_ref_2, bv_ref_3, bv_set_3,
             cdr_let_ref, cdr_let_set, char_equal_2, char_greater_2, char_less_2, char_position_csi, complex_wrapped, curlet_ref, cv_ref_2, cv_set_3,
             display_2, display_f, dynamic_wind_body, dynamic_wind_init, dynamic_wind_unchecked,
             format_as_objstr, format_f, format_just_control_string, format_no_column, fv_ref_2, fv_ref_3, fv_set_3, fv_set_unchecked, geq_2,
             get_output_string_uncopied, hash_table_2, hash_table_ref_2, int_log2, is_defined_in_rootlet, is_defined_in_unlet, iv_ref_2, iv_ref_3, iv_set_3,
             list_0, list_1, list_2, list_3, list_4, list_ref_at_0, list_ref_at_1, list_ref_at_2, list_set_i,
             logand_2, logand_ii, logior_ii, logior_2, logxor_2, memq_2, memq_3, memq_4, memq_any, multiply_3,
             outlet_unlet, profile_out, read_char_1, restore_setter, rootlet_ref, simple_char_eq, simple_char_eq1, simple_char_eq2,
             simple_inlet, simple_list_values, starlet_ref, starlet_set,
             string_append_2, string_c1, string_equal_2, string_equal_2c, string_greater_2, string_less_2, sublet_curlet, substring_uncopied, subtract_1,
             subtract_2, subtract_2f, subtract_3, subtract_f2, subtract_x1, sv_unlet_ref, symbol_to_string_uncopied, tree_set_memq_syms,
             unlet_disabled, unlet_ref, unlet_set, values_uncopied, vector_2, vector_3, vector_ref_2, vector_ref_3, vector_set_3, vector_set_4, write_2;

  s7_pointer divide_2, divide_by_2, geq_xf, geq_xi, greater_2, greater_xf, greater_xi, invert_1, invert_x, leq_2, leq_ixx,
             leq_xi, less_2, less_x0, less_xf, less_xi, max_2, max_3, min_2, min_3,
             multiply_2, num_eq_2, num_eq_ix, num_eq_xi, random_1, random_f, random_i;
  s7_pointer seed_symbol, carry_symbol;

  /* object->let symbols */
  s7_pointer active_symbol, alias_symbol, at_end_symbol, c_object_ref_symbol, c_type_symbol, class_symbol, closed_symbol,
             current_value_symbol, data_symbol, dimensions_symbol, entries_symbol, file_info_symbol, file_symbol, function_symbol, info_symbol,
             is_mutable_symbol, line_symbol, open_symbol, original_vector_symbol, pointer_symbol, port_type_symbol, position_symbol,
             sequence_symbol, size_symbol, source_symbol, weak_symbol;


  s7_pointer open_input_function_choices[S7_NUM_READ_CHOICES];
  s7_pointer closed_input_function, closed_output_function;
  s7_pointer vector_set_function, string_set_function, list_set_function, hash_table_set_function, let_set_function, c_object_set_function, last_function;
  s7_pointer wrong_type_arg_info, out_of_range_info, sole_arg_wrong_type_info, sole_arg_out_of_range_info;
  s7_pointer unicode_chars_table;

  #define NUM_SAFE_PRELISTS 8
  #define NUM_SAFE_LISTS 32               /* 36 is the biggest normally (lint.scm), 49 in s7test, 57 in snd-test, > 16 doesn't happen much */
  s7_pointer safe_lists[NUM_SAFE_LISTS];
  int32_t current_safe_list;
  int32_t **current_distance;
#if S7_DEBUGGING
  s7_int safe_list_uses[NUM_SAFE_LISTS];
  int32_t *tc_rec_calls;
  bool printing_gc_info;
  s7_int blocks_allocated, format_ports_allocated, c_functions_allocated;
  s7_int blocks_borrowed[NUM_BLOCK_LISTS], blocks_freed[NUM_BLOCK_LISTS], blocks_mallocated[NUM_BLOCK_LISTS];
  s7_int string_wrapper_allocs, integer_wrapper_allocs, real_wrapper_allocs, complex_wrapper_allocs, c_pointer_wrapper_allocs, let_wrapper_allocs, slot_wrapper_allocs;
#endif

  s7_pointer autoload_table, starlet, starlet_symbol, temp_error_hook;
  const char ***autoload_names;
  s7_int *autoload_names_sizes;
  bool **autoloaded_already;
  s7_int autoload_names_loc, autoload_names_top;
  int32_t format_depth;
  bool undefined_identifier_warnings, undefined_constant_warnings, stop_at_error;

  opt_funcs_t *alloc_opt_func_cells;
  int32_t alloc_opt_func_k;

  int32_t pc;
  #define OPTS_SIZE 256      /* pqw-vox needs 178 */
  opt_info *opts[OPTS_SIZE]; /* this form is a lot faster than opt_info**! */

  #define INITIAL_SAVED_POINTERS_SIZE 256
  void **saved_pointers;
  s7_int saved_pointers_loc, saved_pointers_size;

  s7_pointer type_names[NUM_TYPES];
  s7_int overall_start_time;
};     /* store all s7_scheme bools in one int? ca 60 bytes saved out of 11440? */

typedef enum {p_display, p_write, p_readable, p_key, p_code} use_write_t;


/* ---------------- extracted for s7_continuation.c ---------------- */

/* basic accessors */
#define full_type(p) ((p)->tf.u64_type)
#define low_type_bits(p) ((p)->tf.bits.low_bits)

/* type bit manipulation macros */
#define set_type_bit(p, b) full_type(p) |= (b)
#define clear_type_bit(p, b) full_type(p) &= (~(b))
#define has_type_bit(p, b) ((full_type(p) & (b)) != 0)
#define set_low_type_bit(p, b) low_type_bits(p) |= (b)
#define clear_low_type_bit(p, b) low_type_bits(p) &= (~(b))
#define has_low_type_bit(p, b) ((low_type_bits(p) & (b)) != 0)
#define set_mid_type_bit(p, b) (p)->tf.bits.mid_bits |= (b)
#define has_mid_type_bit(p, b) (((p)->tf.bits.mid_bits & (b)) != 0)
#define set_high_type_bit(p, b) (p)->tf.bits.high_bits |= (b)
#define has_high_type_bit(p, b) (((p)->tf.bits.high_bits & (b)) != 0)

/* type flags */
#define T_MULTIPLE_VALUE (1 << (8 + 7))
#define T_SHORT_VERY_SAFE_CLOSURE (1 << 4)
#define T_MATCHED T_MULTIPLE_VALUE
#define T_MID_IMMUTABLE (1 << 8)
#define T_BAFFLE_LET T_SHORT_VERY_SAFE_CLOSURE
#define T_GC_MARK 0x8000000000000000
#define T_SYNTACTIC (1 << (8 + 1))
#define T_OPTIMIZED (1 << (8 + 3))

/* T_* identity macros (non-debug versions) */
#define T_App(P)  P
#define T_Arg(P)  P
#define T_Bgf(P)  P
#define T_Bgi(P)  P
#define T_Bgr(P)  P
#define T_Bgz(P)  P
#define T_BVc(P)  P
#define T_Cat(P)  P
#define T_CFn(P)  P
#define T_Chr(P)  P
#define T_Clo(P)  P
#define T_CMac(P) P
#define T_Cmp(P)  P
#define T_Con(P)  P
#define T_Ctr(P)  P
#define T_Cvc(P)  P
#define T_Dyn(P)  P
#define T_Eof(P)  P
#define T_Exs(P)  P
#define T_Ext(P)  P
#define T_Fnc(P)  P
#define T_Frc(P)  P
#define T_Fst(P)  P
#define T_Fvc(P)  P
#define T_Got(P)  P
#define T_Hsh(P)  P
#define T_Int(P)  P
#define T_Itr(P)  P
#define T_Ivc(P)  P
#define T_Key(P)  P
#define T_Let(P)  P
#define T_Lst(P)  P
#define T_Mac(P)  P
#define T_Met(P)  P
#define T_Muti(P) P
#define T_Nmv(P)  P
#define T_Num(P)  P
#define T_Nvc(P)  P
#define T_Obj(P)  P
#define T_Op(P)   P
#define T_Out(P)  P
#define T_Pair(P) P
#define T_Pcs(P)  P
#define T_Pos(P)  P
#define T_Prc(P)  P
#define T_Prf(P)  P
#define T_Pri(P)  P
#define T_Pro(P)  P
#define T_Prt(P)  P
#define T_Ptr(P)  P
#define T_Ran(P)  P
#define T_Rel(P)  P
#define T_Seq(P)  P
#define T_Sld(P)  P
#define T_Sln(P)  P
#define T_Slt(P)  P
#define T_Stk(P)  P
#define T_Str(P)  P
#define T_SVec(P) P
#define T_Sym(P)  P
#define T_Syn(P)  P
#define T_Undf(P) P
#define T_Vec(P)  P

/* begin_temp (macro) */
#if S7_DEBUGGING
#define begin_temp(P, Val)             do {s7_pointer __val__ = Val; begin_temp_1(sc, P, __val__, __func__, __LINE__); P = __val__;} while (0)
#else
#define begin_temp(P, Val)             P = Val
#endif

/* c_function_call (macro) */
#define c_function_call(f)             (T_Fnc(f))->object.fnc.ff

/* caar (macro) */
#define caar(p)                        car(car(p))

/* caddr (macro) */
#define caddr(p)                       car(cdr(cdr(p)))

/* cadr (macro) */
#define cadr(p)                        car(cdr(p))

/* call_gc (macro) */
#if S7_DEBUGGING
  #define call_gc(Sc) gc(Sc, __func__, __LINE__)
  s7_int gc(s7_scheme *sc, const char *func, int32_t line);
#else
  #define call_gc(Sc) gc(Sc)
  s7_int gc(s7_scheme *sc);
#endif

/* car (macro) */
#define car(p)                         (T_Pair(p))->object.cons.car

/* cddr (macro) */
#define cddr(p)                        cdr(cdr(p))

/* cdr (macro) */
#define cdr(p)                         (T_Pair(p))->object.cons.cdr

/* clear_match_pair (macro) */
#define clear_match_pair(p)            clear_low_type_bit(T_Pair(p), T_MATCHED)

/* closure_arity (macro) */
#define closure_arity(p)               (T_Clo(p))->object.func.arity

/* closure_pars (macro) */
#define closure_pars(p)                T_Arg((T_Clo(p))->object.func.args)

/* counter_capture (macro) */
#define counter_capture(p)             (T_Ctr(p))->object.ctr.cap

/* counter_list (macro) */
#define counter_list(p)                (T_Ctr(p))->object.ctr.list

/* counter_result (macro) */
#define counter_result(p)              (T_Ctr(p))->object.ctr.result

/* counter_set_capture (macro) */
#define counter_set_capture(p, Val)    (T_Ctr(p))->object.ctr.cap = Val

/* counter_set_let (macro) */
#define counter_set_let(p, L)          (T_Ctr(p))->object.ctr.let = T_Let(L)

/* counter_set_list (macro) */
#define counter_set_list(p, Val)       (T_Ctr(p))->object.ctr.list = T_Ext(Val)

/* counter_set_result (macro) */
#define counter_set_result(p, Val)     (T_Ctr(p))->object.ctr.result = T_Ext(Val)

/* counter_set_slots (macro) */
#define counter_set_slots(p, Val)      (T_Ctr(p))->object.ctr.slots = T_Sln(Val)

/* counter_slots (macro) */
#define counter_slots(p)               T_Sln(T_Ctr(p)->object.ctr.slots)

/* current_input_port (macro) */
#define current_input_port(Sc)         T_Pri(Sc->input_port)

/* dynamic_wind_in (macro) */
#define dynamic_wind_in(p)             (T_Dyn(p))->object.winder.in

/* dynamic_wind_out (macro) */
#define dynamic_wind_out(p)            (T_Dyn(p))->object.winder.out

/* dynamic_wind_state (macro) */
#define dynamic_wind_state(p)          (T_Dyn(p))->object.winder.state

/* end_temp (macro) */
#define end_temp(p)                     p = sc->unused

/* error_nr (decl) */
no_return void error_nr(s7_scheme *sc, s7_pointer type, s7_pointer info);


/* fx_call (macro) */
#define fx_call(Sc, F) fx_proc(F)(Sc, car(F))

/* if_method_exists_return_value (macro) */
#define if_method_exists_return_value(Sc, Obj, Method, Args)		\
  {							\
    s7_pointer _Func_;					\
    if ((s7i_has_active_methods(Sc, Obj)) &&				\
	((_Func_ = s7i_find_method_with_let(Sc, Obj, Method)) != Sc->undefined)) \
      return(s7_apply_function(Sc, _Func_, Args)); \
  }

/* inline_make_let_with_slot (decl) */
extern inline s7_pointer inline_make_let_with_slot(s7_scheme *sc, s7_pointer old_let, s7_pointer symbol, s7_pointer value);

/* is_any_closure (macro) */
#define is_any_closure(P)              t_any_closure_p[type(P)]

/* is_baffle_let (macro) */
#define is_baffle_let(p)               has_high_type_bit(T_Let(p), T_BAFFLE_LET)

/* is_c_function (macro) */
#define is_c_function(f)               (type(f) >= T_C_FUNCTION)                   /* does not include T_C_FUNCTION_STAR */

/* is_closure (macro) */
#define is_closure(p)                  (type(p) == T_CLOSURE)

/* is_counter (macro) */
#define is_counter(p)                  (type(p) == T_COUNTER)

/* is_immutable (macro) */
#define is_immutable(p)                has_mid_type_bit(T_Exs(p), T_MID_IMMUTABLE)

/* is_marked (macro) */
#define is_marked(p)                   has_type_bit(p, T_GC_MARK)

/* is_matched_pair (macro) */
#define is_matched_pair(p)             has_low_type_bit(T_Pair(p), T_MATCHED)

/* is_null (macro) */
#define is_null(p)                     ((T_Exs(p)) == sc->nil)

/* is_pair (macro) */
#define is_pair(p)                     (type(p) == T_PAIR)

/* is_symbol (macro) */
#define is_symbol(p)                   (type(p) == T_SYMBOL)

/* is_t_procedure (macro) */
#define is_t_procedure(p)              (t_procedure_p[type(p)])

/* let_baffle_key (macro) */
#define let_baffle_key(p)              (T_Let(p))->object.let.edat.key

/* let_outlet (macro) */
#define let_outlet(p)                  T_Out((T_Let(p))->object.let.nxt)

/* let_set_baffle_key (macro) */
#define let_set_baffle_key(p, K)       (T_Let(p))->object.let.edat.key = K

/* list_1 (macro) */
#define list_1(Sc, A)                  cons(Sc, A, Sc->nil)

/* list_1_unchecked (macro) */
#define list_1_unchecked(Sc, A)        cons_unchecked(Sc, A, Sc->nil)

/* list_2_unchecked (macro) */
#define list_2_unchecked(Sc, A, B)     cons_unchecked(Sc, A, cons_unchecked(Sc, B, Sc->nil))

/* lookup_checked (macro) */
#if WITH_GCC
  #if S7_DEBUGGING
    #define lookup_checked(Sc, Sym) ({s7_pointer _x_; _x_ = lookup_1(Sc, T_Sym(Sym)); ((_x_) ? _x_ : unbound_variable(Sc, T_Sym(Sym)));})
  #else
    #define lookup_checked(Sc, Sym) ({s7_pointer _x_; _x_ = lookup(Sc, T_Sym(Sym)); ((_x_) ? _x_ : unbound_variable(Sc, T_Sym(Sym)));})
  #endif
#else
  #define lookup_checked(Sc, Sym) lookup(Sc, Sym)
#endif

/* lookup (decl) */
s7_pointer lookup(s7_scheme *sc, const s7_pointer symbol);

/* make_let (decl) */
s7_pointer make_let(s7_scheme *sc, s7_pointer old_let);

/* make_simple_vector (decl) */
extern inline s7_pointer make_simple_vector(s7_scheme *sc, s7_int len);

/* mallocate_block (decl) */
extern inline block_t *mallocate_block(s7_scheme *sc);

/* mark_stack_1 (decl) */
void mark_stack_1(s7_pointer stack, s7_int top);

/* method_or_bust_p (decl) */
s7_pointer method_or_bust_p(s7_scheme *sc, s7_pointer obj, s7_pointer method, s7_pointer typ);


/* new_cell (macro) */
#define new_cell(Sc, Obj, Type)			\
  do {						\
    if (Sc->free_heap_top <= Sc->free_heap_trigger) try_to_call_gc(Sc); \
    Obj = (*(--(Sc->free_heap_top))); \
    set_full_type(Obj, Type);	      \
    } while (0)

/* opt2_pair (macro) */
#define opt2_pair(P)                   T_Lst(opt2(P,                OPT2_PAIR))

/* pair_set_syntax_op (macro) */
#define pair_set_syntax_op(p, X)       do {set_optimize_op(p, X); set_syntactic_pair(p);} while (0)

/* stack_end macros */
#define stack_end_code(Sc) Sc->stack_end[0]
#define stack_end_let(Sc)  Sc->stack_end[1]
#define stack_end_args(Sc) Sc->stack_end[2]
#define stack_end_op(Sc)   Sc->stack_end[3]

/* pop_stack (macro) */
#if S7_DEBUGGING
#define pop_stack(Sc) pop_stack_1(Sc, __func__, __LINE__)
#else
#define pop_stack(Sc)       do {Sc->stack_end -= 4; memcpy((void *)Sc, (void *)(Sc->stack_end), 4 * sizeof(s7_pointer));} while (0)
#endif

/* port_write_character (macro) */
#define port_write_character(p)        port_port(p)->pf->write_character

/* port_write_string (macro) */
#define port_write_string(p)           port_port(p)->pf->write_string

/* push_stack (macro) */
#if S7_DEBUGGING
#define push_stack(Sc, Op, Args, Code)	\
  do {s7_pointer *_end_; _end_ = Sc->stack_end; push_stack_1(Sc, Op, Args, Code, _end_, __func__, __LINE__);} while (0)
#else
#define push_stack(Sc, Op, Args, Code) \
  do { \
      stack_end_code(Sc) = Code; \
      stack_end_let(Sc) = Sc->curlet; \
      stack_end_args(Sc) = Args; \
      stack_end_op(Sc) = (s7_pointer)(opcode_t)(Op); \
      Sc->stack_end += 4; \
  } while (0)
#endif

/* push_stack_no_let_no_code (macro) */
#if S7_DEBUGGING
#define push_stack_no_let_no_code(Sc, Op, Args) push_stack(Sc, Op, Args, Sc->unused)
#else
#define push_stack_no_let_no_code(Sc, Op, Args) \
  do { \
      stack_end_args(Sc) = Args; \
      stack_end_op(Sc) = (s7_pointer)(opcode_t)(Op); \
      Sc->stack_end += 4; \
  } while (0)
#endif

/* push_stack_op_let (macro) */
#if S7_DEBUGGING
#define push_stack_op_let(Sc, Op)               push_stack(Sc, Op, Sc->unused, Sc->unused)
#else
#define push_stack_op_let(Sc, Op) \
  do { \
      stack_end_let(Sc) = Sc->curlet; \
      stack_end_op(Sc) = (s7_pointer)(opcode_t)(Op); \
      Sc->stack_end += 4; \
  } while (0)
#endif

/* resize_heap (macro) */
#define resize_heap(Sc) resize_heap_to(Sc, 0)

/* set_baffle_let (macro) */
#define set_baffle_let(p)              set_high_type_bit(T_Let(p), T_BAFFLE_LET)

/* set_cdr (macro) */
#define set_cdr(p, Val) cdr(p) = T_Ext(Val)

/* set_curlet (macro) */
#define set_curlet(Sc, P)              Sc->curlet = T_Let(P)

/* set_current_input_port (macro) */
#define set_current_input_port(Sc, P)  Sc->input_port = T_Pri(P)

/* set_current_output_port (macro) */
#define set_current_output_port(Sc, P) Sc->output_port = T_Pro(P)

/* set_elist_1 (decl) */
s7_pointer set_elist_1(s7_scheme *sc, s7_pointer x1);


/* set_elist_2 (decl) */
s7_pointer s7i_set_elist_2(s7_scheme *sc, s7_pointer x1, s7_pointer x2);

/* set_full_type (macro) */
#if S7_DEBUGGING
  #define set_full_type(p, f) set_type_1(p, f, __func__, __LINE__)
#else
  #define set_full_type(p, f) full_type(p) = f
#endif

/* set_immutable_pair (macro) */
#define set_immutable_pair(p)          set_mid_type_bit(T_Pair(p), T_MID_IMMUTABLE)

/* set_mark (macro) */
#define set_mark(p)                    set_type_bit(T_Pos(p), T_GC_MARK)

/* set_match_pair (macro) */
#define set_match_pair(p)              set_low_type_bit(T_Pair(p), T_MATCHED)

/* set_plist_1 (decl) */
	  s7_pointer set_plist_1(s7_scheme *sc, s7_pointer x1);


/* set_stack_op (macro) */
#define set_stack_op(Stack, Loc, Op) stack_element(Stack, Loc) = (s7_pointer)(opcode_t)(Op)

/* stack_args (macro) */
#define stack_args(Stack, Loc)       stack_element(Stack, Loc - 1)

/* stack_clear_flags (macro) */
#define stack_clear_flags(p)           (T_Stk(p))->object.stk.flags = 0

/* stack_code (macro) */
#define stack_code(Stack, Loc)       stack_element(Stack, Loc - 3)

/* stack_elements (macro) */
#define stack_elements(p)              vector_elements_unchecked(T_Stk(p))

/* stack_has_counters (macro) */
#define stack_has_counters(p)          (((T_Stk(p))->object.stk.flags & 2) != 0)

/* stack_has_pairs (macro) */
#define stack_has_pairs(p)             (((T_Stk(p))->object.stk.flags & 1) != 0)

/* stack_let (macro) */
#define stack_let(Stack, Loc)        stack_element(Stack, Loc - 2)

/* stack_op (macro) */
#define stack_op(Stack, Loc)         ((opcode_t)T_Op(stack_element(Stack, Loc)))

/* stack_set_has_counters (macro) */
#define stack_set_has_counters(p)      (T_Stk(p))->object.stk.flags |= 2

/* stack_set_has_pairs (macro) */
#define stack_set_has_pairs(p)         (T_Stk(p))->object.stk.flags |= 1

/* stack_top (macro) */
#define stack_top(Sc)                  ((Sc)->stack_end - (Sc)->stack_start)

/* temp_stack_top (macro) */
#define temp_stack_top(p)              (T_Stk(p))->object.stk.top

/* type_unchecked (macro) */
#define type_unchecked(p) ((p)->tf.type_field)

/* type (macro) */
#define type(p) ((p)->tf.type_field)

/* vector_elements (macro) */
#define vector_elements(p)             (T_Nvc(p))->object.vector.elements.objects

/* vector_length (macro) */
#define vector_length(p)               (p)->object.vector.length

/* missing macros for s7_continuation.c */
#define T_MID_HAS_METHODS (1 << 14)
#define OPT2_FX (1 << 18)
#define cons(Sc, A, B) s7_cons(Sc, A, B)
#define counter_let(p) T_Let((T_Ctr(p))->object.ctr.let)
#define display(Obj) string_value(s7_object_to_string(sc, Obj, false))
#define fx_proc(f) ((s7_function)(opt2(f, OPT2_FX)))
#define opt2(p, r) ((p)->object.cons.o2.opt2)
#define port_port(p) (T_Prt(p))->object.prt.port
/* resize_heap_to (macro) */
#if S7_DEBUGGING
#define resize_heap_to(Sc, Size) resize_heap_to_1(Sc, Size, __func__, __LINE__)
#else
void resize_heap_to(s7_scheme *sc, s7_int size);
#endif
#define set_optimize_op(P, Op) (T_Ext(P))->tf.bits.opt_bits = (Op)
#define set_syntactic_pair(p) full_type(T_Pair(p)) = (T_PAIR | T_SYNTACTIC | (full_type(p) & (0xffffffffffff0000 & ~T_OPTIMIZED)))
#define stack_element(p, i) vector_element_unchecked(T_Stk(p), i)
#define string_value(p) (T_Str(p))->object.string.svalue
#define vector_element_unchecked(p, i) ((p)->object.vector.elements.objects[i])
#define vector_elements_unchecked(p) (p)->object.vector.elements.objects

/* wrap_string (decl) */
s7_pointer wrap_string(s7_scheme *sc, const char *str, s7_int len);

/* add_to_gc_list (decl) */
void add_to_gc_list(s7_scheme *sc, gc_list_t *gp, s7_pointer p);

/* b_simple_setter (decl) */
s7_pointer b_simple_setter(s7_scheme *sc, int32_t typer, s7_pointer args);

/* cons_unchecked (decl) */
s7_pointer cons_unchecked(s7_scheme *sc, s7_pointer a, s7_pointer b);

/* liberate_block (decl) */
void liberate_block(s7_scheme *sc, block_t *blk);

/* sole_arg_wrong_type_error_nr (decl) */
void sole_arg_wrong_type_error_nr(s7_scheme *sc, s7_pointer caller, s7_pointer arg, s7_pointer typ);

/* starlet_set_1 (decl) */
s7_pointer starlet_set_1(s7_scheme *sc, s7_pointer sym, s7_pointer val);

/* symbol_to_port (decl) */
void symbol_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci);

/* syntax_error_nr (decl) */
void syntax_error_nr(s7_scheme *sc, const char *errmsg, s7_int len, s7_pointer obj);

/* begin_temp_1 (decl) */
void begin_temp_1(s7_scheme *sc, s7_pointer p, s7_pointer val, const char *func, int line);

/* set_type_1 (decl) */
void set_type_1(s7_pointer p, s7_uint typ, const char *func, int32_t line);

/* resize_heap_to_1 (decl) */
void resize_heap_to_1(s7_scheme *sc, s7_int size, const char *func, int line);

/* try_to_call_gc (macro/decl) */
#if S7_DEBUGGING
void try_to_call_gc_1(s7_scheme *sc, const char *func, int32_t line);
#define try_to_call_gc(Sc) try_to_call_gc_1(Sc, __func__, __LINE__)
#else
void try_to_call_gc(s7_scheme *sc);
#endif

/* splice_in_values (decl) */
s7_pointer splice_in_values(s7_scheme *sc, s7_pointer args);

/* let_temp_done (decl) */
void let_temp_done(s7_scheme *sc, s7_pointer args, s7_pointer let);

/* let_temp_unwind (decl) */
void let_temp_unwind(s7_scheme *sc, s7_pointer slot, s7_pointer new_value);

/* dynamic_unwind (decl) */
s7_pointer dynamic_unwind(s7_scheme *sc, s7_pointer func, s7_pointer args);

/* pop_input_port (decl) */
void pop_input_port(s7_scheme *sc);

/* unbound_variable (decl) */
s7_pointer unbound_variable(s7_scheme *sc, s7_pointer sym);

/* set_elist_2 (decl) */
s7_pointer set_elist_2(s7_scheme *sc, s7_pointer x1, s7_pointer x2);

/* pop_stack_1 (decl) */
void pop_stack_1(s7_scheme *sc, const char *func, int32_t line);

/* push_stack_1 (decl) */
void push_stack_1(s7_scheme *sc, opcode_t op, s7_pointer args, s7_pointer code, s7_pointer *end, const char *func, int32_t line);

/* lookup_1 (decl) */
s7_pointer lookup_1(s7_scheme *sc, const s7_pointer symbol);

/* type predicates tables (decls) */
extern bool t_procedure_p[NUM_TYPES];
extern bool t_any_closure_p[NUM_TYPES];
extern s7_pointer a_procedure_string;

/* opcodes for s7_continuation.c */
enum {OP_UNOPT, OP_GC_PROTECT, /* must be an even number of ops here, op_gc_protect used below as lower boundary marker */

      OP_SAFE_C_NC, HOP_SAFE_C_NC, OP_SAFE_C_S, HOP_SAFE_C_S,
      OP_SAFE_C_SS, HOP_SAFE_C_SS, OP_SAFE_C_SC, HOP_SAFE_C_SC, OP_SAFE_C_CS, HOP_SAFE_C_CS, OP_SAFE_C_CQ, HOP_SAFE_C_CQ,
      OP_SAFE_C_SSS, HOP_SAFE_C_SSS, OP_SAFE_C_SCS, HOP_SAFE_C_SCS, OP_SAFE_C_SSC, HOP_SAFE_C_SSC, OP_SAFE_C_CSS, HOP_SAFE_C_CSS,
      OP_SAFE_C_SCC, HOP_SAFE_C_SCC, OP_SAFE_C_CSC, HOP_SAFE_C_CSC, OP_SAFE_C_CCS, HOP_SAFE_C_CCS,
      OP_SAFE_C_NS, HOP_SAFE_C_NS, OP_SAFE_C_opNCq, HOP_SAFE_C_opNCq, OP_SAFE_C_opSq, HOP_SAFE_C_opSq,
      OP_SAFE_C_opSSq, HOP_SAFE_C_opSSq, OP_SAFE_C_opSCq, HOP_SAFE_C_opSCq,
      OP_SAFE_C_opCSq, HOP_SAFE_C_opCSq, OP_SAFE_C_S_opSq, HOP_SAFE_C_S_opSq,
      OP_SAFE_C_C_opSCq, HOP_SAFE_C_C_opSCq, OP_SAFE_C_S_opSCq, HOP_SAFE_C_S_opSCq, OP_SAFE_C_S_opCSq, HOP_SAFE_C_S_opCSq,
      OP_SAFE_C_opSq_S, HOP_SAFE_C_opSq_S, OP_SAFE_C_opSq_C, HOP_SAFE_C_opSq_C,
      OP_SAFE_C_opSq_opSq, HOP_SAFE_C_opSq_opSq, OP_SAFE_C_S_opSSq, HOP_SAFE_C_S_opSSq, OP_SAFE_C_C_opSq, HOP_SAFE_C_C_opSq,
      OP_SAFE_C_opCSq_C, HOP_SAFE_C_opCSq_C, OP_SAFE_C_opSSq_C, HOP_SAFE_C_opSSq_C, OP_SAFE_C_C_opSSq, HOP_SAFE_C_C_opSSq,
      OP_SAFE_C_opSSq_opSSq, HOP_SAFE_C_opSSq_opSSq, OP_SAFE_C_opSSq_opSq, HOP_SAFE_C_opSSq_opSq, OP_SAFE_C_opSq_opSSq, HOP_SAFE_C_opSq_opSSq,
      OP_SAFE_C_opSSq_S, HOP_SAFE_C_opSSq_S, OP_SAFE_C_opCSq_S, HOP_SAFE_C_opCSq_S, OP_SAFE_C_opSCq_C, HOP_SAFE_C_opSCq_C,
      OP_SAFE_C_op_opSSqq_S, HOP_SAFE_C_op_opSSqq_S, OP_SAFE_C_op_opSqq, HOP_SAFE_C_op_opSqq,
      OP_SAFE_C_op_S_opSqq, HOP_SAFE_C_op_S_opSqq, OP_SAFE_C_op_opSq_Sq, HOP_SAFE_C_op_opSq_Sq, OP_SAFE_C_opSq_CS, HOP_SAFE_C_opSq_CS,

      OP_SAFE_C_A, HOP_SAFE_C_A, OP_SAFE_C_AA, HOP_SAFE_C_AA, OP_SAFE_C_SA, HOP_SAFE_C_SA, OP_SAFE_C_AS, HOP_SAFE_C_AS,
      OP_SAFE_C_CA, HOP_SAFE_C_CA, OP_SAFE_C_AC, HOP_SAFE_C_AC, OP_SAFE_C_AAA, HOP_SAFE_C_AAA, OP_SAFE_C_4A, HOP_SAFE_C_4A,
      OP_SAFE_C_NA, HOP_SAFE_C_NA, OP_SAFE_C_ALL_CA, HOP_SAFE_C_ALL_CA,
      OP_SAFE_C_SSA, HOP_SAFE_C_SSA, OP_SAFE_C_SAS, HOP_SAFE_C_SAS, OP_SAFE_C_SAA, HOP_SAFE_C_SAA,
      OP_SAFE_C_CSA, HOP_SAFE_C_CSA, OP_SAFE_C_SCA, HOP_SAFE_C_SCA, OP_SAFE_C_ASS, HOP_SAFE_C_ASS,
      OP_SAFE_C_CAC, HOP_SAFE_C_CAC, OP_SAFE_C_AGG, HOP_SAFE_C_AGG,
      OP_SAFE_C_opAq, HOP_SAFE_C_opAq, OP_SAFE_C_opAAq, HOP_SAFE_C_opAAq, OP_SAFE_C_opAAAq, HOP_SAFE_C_opAAAq,
      OP_SAFE_C_S_opAq, HOP_SAFE_C_S_opAq, OP_SAFE_C_opAq_S, HOP_SAFE_C_opAq_S, OP_SAFE_C_S_opAAq, HOP_SAFE_C_S_opAAq,
      OP_SAFE_C_STAR, HOP_SAFE_C_STAR, OP_SAFE_C_STAR_A, HOP_SAFE_C_STAR_A,
      OP_SAFE_C_STAR_AA, HOP_SAFE_C_STAR_AA, OP_SAFE_C_STAR_NA, HOP_SAFE_C_STAR_NA,

      OP_SAFE_C_P, HOP_SAFE_C_P, OP_SAFE_C_PP, HOP_SAFE_C_PP, OP_SAFE_C_FF, HOP_SAFE_C_FF, OP_SAFE_C_SP, HOP_SAFE_C_SP,
      OP_SAFE_C_CP, HOP_SAFE_C_CP, OP_SAFE_C_AP, HOP_SAFE_C_AP, OP_SAFE_C_PA, HOP_SAFE_C_PA, OP_SAFE_C_PS, HOP_SAFE_C_PS,
      OP_SAFE_C_PC, HOP_SAFE_C_PC, OP_SAFE_C_SSP, HOP_SAFE_C_SSP, OP_ANY_C_NP, HOP_ANY_C_NP, OP_SAFE_C_3P, HOP_SAFE_C_3P,

      OP_THUNK, HOP_THUNK, OP_THUNK_O, HOP_THUNK_O, OP_THUNK_ANY, HOP_THUNK_ANY,
      OP_SAFE_THUNK, HOP_SAFE_THUNK, OP_SAFE_THUNK_A, HOP_SAFE_THUNK_A, OP_SAFE_THUNK_ANY, HOP_SAFE_THUNK_ANY,

      OP_CLOSURE_S, HOP_CLOSURE_S, OP_CLOSURE_S_O, HOP_CLOSURE_S_O,
      OP_CLOSURE_A, HOP_CLOSURE_A, OP_CLOSURE_A_O, HOP_CLOSURE_A_O, OP_CLOSURE_P, HOP_CLOSURE_P,
      OP_CLOSURE_AP, HOP_CLOSURE_AP, OP_CLOSURE_PA, HOP_CLOSURE_PA, OP_CLOSURE_PP, HOP_CLOSURE_PP,
      OP_CLOSURE_FA, HOP_CLOSURE_FA, OP_CLOSURE_SS, HOP_CLOSURE_SS, OP_CLOSURE_SS_O, HOP_CLOSURE_SS_O,
      OP_CLOSURE_SC, HOP_CLOSURE_SC, OP_CLOSURE_SC_O, HOP_CLOSURE_SC_O,
      OP_CLOSURE_3S, HOP_CLOSURE_3S, OP_CLOSURE_3S_O, HOP_CLOSURE_3S_O, OP_CLOSURE_4S, HOP_CLOSURE_4S, OP_CLOSURE_4S_O, HOP_CLOSURE_4S_O, OP_CLOSURE_5S, HOP_CLOSURE_5S,
      OP_CLOSURE_AA, HOP_CLOSURE_AA, OP_CLOSURE_AA_O, HOP_CLOSURE_AA_O, OP_CLOSURE_3A, HOP_CLOSURE_3A, OP_CLOSURE_4A, HOP_CLOSURE_4A,
      OP_CLOSURE_NA, HOP_CLOSURE_NA, OP_CLOSURE_ASS, HOP_CLOSURE_ASS, OP_CLOSURE_SAS, HOP_CLOSURE_SAS ,OP_CLOSURE_AAS, HOP_CLOSURE_AAS,
      OP_CLOSURE_SAA, HOP_CLOSURE_SAA, OP_CLOSURE_ASA, HOP_CLOSURE_ASA, OP_CLOSURE_NS, HOP_CLOSURE_NS,

      OP_SAFE_CLOSURE_S, HOP_SAFE_CLOSURE_S, OP_SAFE_CLOSURE_S_O, HOP_SAFE_CLOSURE_S_O,
      OP_SAFE_CLOSURE_S_A, HOP_SAFE_CLOSURE_S_A, OP_SAFE_CLOSURE_S_TO_S, HOP_SAFE_CLOSURE_S_TO_S, OP_SAFE_CLOSURE_S_TO_SC, HOP_SAFE_CLOSURE_S_TO_SC,
      OP_SAFE_CLOSURE_P, HOP_SAFE_CLOSURE_P, OP_SAFE_CLOSURE_P_A, HOP_SAFE_CLOSURE_P_A,
      OP_SAFE_CLOSURE_AP, HOP_SAFE_CLOSURE_AP, OP_SAFE_CLOSURE_PA, HOP_SAFE_CLOSURE_PA, OP_SAFE_CLOSURE_PP, HOP_SAFE_CLOSURE_PP,
      OP_SAFE_CLOSURE_A, HOP_SAFE_CLOSURE_A, OP_SAFE_CLOSURE_A_O, HOP_SAFE_CLOSURE_A_O, OP_SAFE_CLOSURE_A_A, HOP_SAFE_CLOSURE_A_A,
      OP_SAFE_CLOSURE_A_TO_SC, HOP_SAFE_CLOSURE_A_TO_SC,
      OP_SAFE_CLOSURE_SS, HOP_SAFE_CLOSURE_SS, OP_SAFE_CLOSURE_SS_O, HOP_SAFE_CLOSURE_SS_O, OP_SAFE_CLOSURE_SS_A, HOP_SAFE_CLOSURE_SS_A,
      OP_SAFE_CLOSURE_SC, HOP_SAFE_CLOSURE_SC, OP_SAFE_CLOSURE_SC_O, HOP_SAFE_CLOSURE_SC_O,
      OP_SAFE_CLOSURE_AA, HOP_SAFE_CLOSURE_AA, OP_SAFE_CLOSURE_AA_O, HOP_SAFE_CLOSURE_AA_O, OP_SAFE_CLOSURE_AA_A, HOP_SAFE_CLOSURE_AA_A,
      OP_SAFE_CLOSURE_SAA, HOP_SAFE_CLOSURE_SAA, OP_SAFE_CLOSURE_SSA, HOP_SAFE_CLOSURE_SSA,
      OP_SAFE_CLOSURE_AGG, HOP_SAFE_CLOSURE_AGG, OP_SAFE_CLOSURE_3A, HOP_SAFE_CLOSURE_3A, OP_SAFE_CLOSURE_NA, HOP_SAFE_CLOSURE_NA,
      OP_SAFE_CLOSURE_3S, HOP_SAFE_CLOSURE_3S, OP_SAFE_CLOSURE_NS, HOP_SAFE_CLOSURE_NS, /* safe_closure_4s gained very little */
      OP_SAFE_CLOSURE_3S_A, HOP_SAFE_CLOSURE_3S_A,

      OP_ANY_CLOSURE_3P, HOP_ANY_CLOSURE_3P, OP_ANY_CLOSURE_4P, HOP_ANY_CLOSURE_4P, OP_ANY_CLOSURE_NP, HOP_ANY_CLOSURE_NP,
      OP_ANY_CLOSURE_SYM, HOP_ANY_CLOSURE_SYM, OP_ANY_CLOSURE_A_SYM, HOP_ANY_CLOSURE_A_SYM,

      OP_CLOSURE_STAR_A, HOP_CLOSURE_STAR_A, OP_CLOSURE_STAR_NA, HOP_CLOSURE_STAR_NA,
      OP_SAFE_CLOSURE_STAR_A, HOP_SAFE_CLOSURE_STAR_A, OP_SAFE_CLOSURE_STAR_AA, HOP_SAFE_CLOSURE_STAR_AA,
      OP_SAFE_CLOSURE_STAR_AA_O, HOP_SAFE_CLOSURE_STAR_AA_O, OP_SAFE_CLOSURE_STAR_A1, HOP_SAFE_CLOSURE_STAR_A1,
      OP_SAFE_CLOSURE_STAR_KA, HOP_SAFE_CLOSURE_STAR_KA, OP_CLOSURE_STAR_KA, HOP_CLOSURE_STAR_KA, OP_SAFE_CLOSURE_STAR_3A, HOP_SAFE_CLOSURE_STAR_3A,
      OP_SAFE_CLOSURE_STAR_NA, HOP_SAFE_CLOSURE_STAR_NA, OP_SAFE_CLOSURE_STAR_NA_0, HOP_SAFE_CLOSURE_STAR_NA_0,
      OP_SAFE_CLOSURE_STAR_NA_1, HOP_SAFE_CLOSURE_STAR_NA_1, OP_SAFE_CLOSURE_STAR_NA_2, HOP_SAFE_CLOSURE_STAR_NA_2,

      OP_C_SS, HOP_C_SS, OP_C_S, HOP_C_S, OP_C_SC, HOP_C_SC, OP_READ_S, HOP_READ_S, OP_C_P, HOP_C_P, OP_C_AP, HOP_C_AP,
      OP_C_A, HOP_C_A, OP_C_AA, HOP_C_AA, OP_C, HOP_C, OP_C_NC, HOP_C_NC, OP_C_NA, HOP_C_NA,

      OP_CL_S, HOP_CL_S, OP_CL_SS, HOP_CL_SS, OP_CL_A, HOP_CL_A, OP_CL_AA, HOP_CL_AA,
      OP_CL_NA, HOP_CL_NA, OP_CL_FA, HOP_CL_FA, OP_CL_SAS, HOP_CL_SAS,
      /* end of h_opts */

      OP_APPLY_SS, OP_APPLY_SA, OP_APPLY_SL, OP_MACRO_D, OP_MACRO_STAR_D,
      OP_WITH_IO, OP_WITH_IO_1, OP_WITH_OUTPUT_TO_STRING, OP_WITH_IO_C, OP_CALL_WITH_OUTPUT_STRING,
      OP_S, OP_S_G, OP_S_A, OP_S_AA, OP_A_A, OP_A_AA, OP_A_SC, OP_P_S, OP_P_S_1, OP_MAP_FOR_EACH_FA, OP_MAP_FOR_EACH_FAA,
      OP_F, OP_F_A, OP_F_AA, OP_F_NP, OP_F_NP_1,

      OP_IMPLICIT_GOTO, OP_IMPLICIT_GOTO_A, OP_IMPLICIT_CONTINUATION_A, OP_IMPLICIT_ITERATE,
      OP_IMPLICIT_VECTOR_REF_A, OP_IMPLICIT_VECTOR_REF_AA,
      OP_IMPLICIT_STRING_REF_A, OP_IMPLICIT_C_OBJECT_REF_A, OP_IMPLICIT_PAIR_REF_A, OP_IMPLICIT_PAIR_REF_AA,
      OP_IMPLICIT_HASH_TABLE_REF_A, OP_IMPLICIT_HASH_TABLE_REF_AA,
      OP_IMPLICIT_LET_REF_C, OP_IMPLICIT_LET_REF_A, OP_IMPLICIT_STARLET_REF_S, OP_IMPLICIT_STARLET_SET_S,
      OP_UNKNOWN, OP_UNKNOWN_NS, OP_UNKNOWN_NA, OP_UNKNOWN_S, OP_UNKNOWN_GG, OP_UNKNOWN_A, OP_UNKNOWN_AA, OP_UNKNOWN_NP,

      OP_SYMBOL, OP_CONSTANT, OP_PAIR_SYM, OP_PAIR_PAIR, OP_PAIR_ANY, HOP_HASH_TABLE_INCREMENT, OP_CLEAR_OPTS,

      OP_READ_INTERNAL, OP_EVAL, OP_EVAL_ARGS, OP_EVAL_ARGS1, OP_EVAL_ARGS2, OP_EVAL_ARGS3, OP_EVAL_ARGS4, OP_EVAL_ARGS5,
      OP_EVAL_SET1_NO_MV, OP_EVAL_SET2, OP_EVAL_SET2_MV, OP_EVAL_SET2_NO_MV, OP_EVAL_SET3, OP_EVAL_SET3_MV, OP_EVAL_SET3_NO_MV,
      OP_APPLY, OP_EVAL_MACRO, OP_LAMBDA, OP_QUOTE, OP_QUOTE_UNCHECKED, OP_MACROEXPAND, OP_CALL_CC, OP_CALL_WITH_EXIT, OP_CALL_WITH_EXIT_O,
      OP_C_CATCH, OP_C_CATCH_ALL, OP_C_CATCH_ALL_O, OP_C_CATCH_ALL_A,

      OP_DEFINE, OP_DEFINE1, OP_BEGIN, OP_BEGIN_HOOK, OP_BEGIN_NO_HOOK, OP_BEGIN_UNCHECKED, OP_BEGIN_2_UNCHECKED, OP_BEGIN_NA, OP_BEGIN_AA,
      OP_IF, OP_IF1, OP_WHEN, OP_UNLESS, OP_SET, OP_SET1, OP_SET2,
      OP_LET, OP_LET1, OP_LET_STAR, OP_LET_STAR1, OP_LET_STAR2, OP_LET_STAR_SHADOWED,
      OP_LETREC, OP_LETREC1, OP_LETREC_STAR, OP_LETREC_STAR1,
      OP_LET_TEMPORARILY, OP_LET_TEMP_UNCHECKED, OP_LET_TEMP_INIT1, OP_LET_TEMP_INIT2, OP_LET_TEMP_DONE, OP_LET_TEMP_DONE1,
      OP_LET_TEMP_S7, OP_LET_TEMP_NA, OP_LET_TEMP_A, OP_LET_TEMP_SETTER, OP_LET_TEMP_UNWIND, OP_LET_TEMP_S7_UNWIND, OP_LET_TEMP_SETTER_UNWIND,
      OP_LET_TEMP_A_A, OP_LET_TEMP_S7_OPENLETS, OP_LET_TEMP_S7_OPENLETS_UNWIND,
      OP_COND, OP_COND1, OP_FEED_TO_1, OP_COND_SIMPLE, OP_COND1_SIMPLE, OP_COND_SIMPLE_O, OP_COND1_SIMPLE_O,
      OP_AND, OP_OR,
      OP_DEFINE_MACRO, OP_DEFINE_MACRO_STAR, OP_DEFINE_EXPANSION, OP_DEFINE_EXPANSION_STAR, OP_MACRO, OP_MACRO_STAR,
      OP_CASE,
      OP_READ_LIST, OP_READ_NEXT, OP_READ_DOT, OP_READ_QUOTE,
      OP_READ_QUASIQUOTE, OP_READ_UNQUOTE, OP_READ_APPLY_VALUES,
      OP_READ_VECTOR, OP_READ_BYTE_VECTOR, OP_READ_INT_VECTOR, OP_READ_FLOAT_VECTOR, OP_READ_COMPLEX_VECTOR, OP_READ_DONE,
      OP_LOAD_RETURN_IF_EOF, OP_LOAD_CLOSE_AND_POP_IF_EOF, OP_EVAL_DONE, OP_SPLICE_VALUES, OP_NO_VALUES,
      OP_CATCH, OP_DYNAMIC_WIND, OP_DYNAMIC_UNWIND, OP_DYNAMIC_UNWIND_PROFILE, OP_PROFILE_IN,
      OP_DEFINE_CONSTANT, OP_DEFINE_CONSTANT1,
      OP_DO, OP_DO_END, OP_DO_END1, OP_DO_STEP, OP_DO_STEP2, OP_DO_INIT,
      OP_DEFINE_STAR, OP_LAMBDA_STAR, OP_LAMBDA_STAR_DEFAULT, OP_ERROR_QUIT, OP_UNWIND_INPUT, OP_UNWIND_OUTPUT, OP_ERROR_HOOK_QUIT,
      OP_WITH_LET, OP_WITH_LET1, OP_WITH_LET_UNCHECKED, OP_WITH_LET_S,
      OP_WITH_BAFFLE, OP_WITH_BAFFLE_UNCHECKED, OP_EXPANSION,
      OP_FOR_EACH, OP_FOR_EACH_1, OP_FOR_EACH_2, OP_FOR_EACH_3,
      OP_MAP, OP_MAP_1, OP_MAP_2, OP_MAP_GATHER, OP_MAP_GATHER_1, OP_MAP_GATHER_2, OP_MAP_GATHER_3, OP_MAP_UNWIND,
      OP_BARRIER, OP_DEACTIVATE_GOTO,
      OP_DEFINE_BACRO, OP_DEFINE_BACRO_STAR, OP_BACRO, OP_BACRO_STAR,
      OP_GET_OUTPUT_STRING,
      OP_SORT, OP_SORT1, OP_SORT2, OP_SORT3, OP_SORT_PAIR_END, OP_SORT_VECTOR_END, OP_SORT_STRING_END,
      OP_EVAL_STRING,
      OP_MEMBER_IF, OP_ASSOC_IF, OP_MEMBER_IF1, OP_ASSOC_IF1,
      OP_LAMBDA_UNCHECKED, OP_LET_UNCHECKED, OP_CATCH_1, OP_CATCH_2, OP_CATCH_ALL,

      OP_SET_UNCHECKED, OP_SET_S_C, OP_SET_S_S, OP_SET_S_P, OP_SET_S_A,
      OP_SET_NORMAL, OP_SET_opSq_A, OP_SET_opSAq_A, OP_SET_opSAq_P, OP_SET_opSAq_P_1, OP_SET_opSAAq_A, OP_SET_opSAAq_P, OP_SET_opSAAq_P_1,
      OP_SET_FROM_SETTER, OP_SET_FROM_LET_TEMP, OP_SET_SAFE,
      OP_INCREMENT_BY_1, OP_DECREMENT_BY_1, OP_INCREMENT_SS, OP_INCREMENT_SA, OP_INCREMENT_SAA, OP_SET_CONS,

      OP_LETREC_UNCHECKED, OP_LETREC_STAR_UNCHECKED, OP_COND_UNCHECKED,
      OP_LAMBDA_STAR_UNCHECKED, OP_DO_UNCHECKED, OP_DEFINE_UNCHECKED, OP_DEFINE_STAR_UNCHECKED, OP_DEFINE_FUNCHECKED, OP_DEFINE_CONSTANT_UNCHECKED,
      OP_DEFINE_WITH_SETTER,

      OP_LET_NO_VARS, OP_NAMED_LET, OP_NAMED_LET_NO_VARS, OP_NAMED_LET_A, OP_NAMED_LET_AA, OP_NAMED_LET_NA, OP_NAMED_LET_STAR,
      OP_LET_NA_OLD, OP_LET_NA_NEW, OP_LET_2A_OLD, OP_LET_2A_NEW, OP_LET_3A_OLD, OP_LET_3A_NEW,
      OP_LET_opaSSq_OLD, OP_LET_opaSSq_NEW, OP_LET_ONE_OLD, OP_LET_ONE_NEW, OP_LET_ONE_P_OLD, OP_LET_ONE_P_NEW,
      OP_LET_ONE_OLD_1, OP_LET_ONE_NEW_1, OP_LET_ONE_P_OLD_1, OP_LET_ONE_P_NEW_1,
      OP_LET_A_OLD, OP_LET_A_NEW, OP_LET_A_P_OLD, OP_LET_A_P_NEW,
      OP_LET_A_A_OLD, OP_LET_A_A_NEW, OP_LET_A_NA_OLD, OP_LET_A_NA_NEW, OP_LET_A_OLD_2, OP_LET_A_NEW_2,
      OP_LET_STAR_NA, OP_LET_STAR_NA_A,

      OP_CASE_A_E_S, OP_CASE_A_I_S, OP_CASE_A_G_S, OP_CASE_A_E_G, OP_CASE_A_G_G, OP_CASE_A_S_G,
      OP_CASE_P_E_S, OP_CASE_P_I_S, OP_CASE_P_G_S, OP_CASE_P_E_G, OP_CASE_P_G_G,
      OP_CASE_E_S, OP_CASE_I_S, OP_CASE_G_S, OP_CASE_E_G, OP_CASE_G_G,
      OP_CASE_A_I_S_A, OP_CASE_A_E_S_A, OP_CASE_A_G_S_A, OP_CASE_A_S_G_A,

      OP_IF_UNCHECKED, OP_AND_P, OP_AND_P1, OP_AND_AP, OP_AND_PAIR_P,
      OP_AND_SAFE_P1, OP_AND_SAFE_P2, OP_AND_SAFE_P3, OP_AND_SAFE_P_REST, OP_AND_2A, OP_AND_3A, OP_AND_N, OP_AND_S_2,
      OP_OR_P, OP_OR_P1, OP_OR_AP, OP_OR_2A, OP_OR_3A, OP_OR_N, OP_OR_S_2, OP_OR_S_TYPE_2,
      OP_WHEN_S, OP_WHEN_A, OP_WHEN_P, OP_WHEN_AND_AP, OP_WHEN_AND_2A, OP_WHEN_AND_3A, OP_UNLESS_S, OP_UNLESS_A, OP_UNLESS_P,

      OP_IF_A_C_C, OP_IF_A_A, OP_IF_A_A_A, OP_IF_S_A_A, OP_IF_AND2_S_A, OP_IF_NOT_A_A, OP_IF_NOT_A_A_A,
      OP_IF_B_A, OP_IF_B_P, OP_IF_B_R, OP_IF_B_A_P, OP_IF_B_P_A, OP_IF_B_P_P, OP_IF_B_N_N,
      OP_IF_A_A_P, OP_IF_A_P_A, OP_IF_S_P_A, OP_IF_S_A_P, OP_IF_S_P, OP_IF_S_P_P, OP_IF_S_R, OP_IF_S_N, OP_IF_S_N_N,
      OP_IF_opSq_P, OP_IF_opSq_P_P, OP_IF_opSq_R, OP_IF_opSq_N, OP_IF_opSq_N_N,
      OP_IF_IS_TYPE_S_P, OP_IF_IS_TYPE_S_P_P, OP_IF_IS_TYPE_S_R, OP_IF_IS_TYPE_S_N, OP_IF_IS_TYPE_S_N_N, OP_IF_IS_TYPE_S_P_A, OP_IF_IS_TYPE_S_A_A, OP_IF_IS_TYPE_S_A_P,
      OP_IF_A_P, OP_IF_A_P_P, OP_IF_A_R, OP_IF_A_N, OP_IF_A_N_N,
      OP_IF_AND2_P, OP_IF_AND2_P_P, OP_IF_AND2_R, OP_IF_AND2_N, OP_IF_AND2_N_N,
      OP_IF_AND3_P, OP_IF_AND3_P_P, OP_IF_AND3_R, OP_IF_AND3_N, OP_IF_AND3_N_N,  /* or3 got few hits */
      OP_IF_P_P, OP_IF_P_P_P, OP_IF_P_R, OP_IF_P_N, OP_IF_P_N_N,
      OP_IF_ANDP_P, OP_IF_ANDP_P_P, OP_IF_ANDP_R, OP_IF_ANDP_N, OP_IF_ANDP_N_N,
      OP_IF_ORP_P, OP_IF_ORP_P_P, OP_IF_ORP_R, OP_IF_ORP_N, OP_IF_ORP_N_N,
      OP_IF_OR2_P, OP_IF_OR2_P_P, OP_IF_OR2_R, OP_IF_OR2_N, OP_IF_OR2_N_N,
      OP_IF_PP, OP_IF_PPP, OP_IF_PN, OP_IF_PR, OP_IF_PRR, OP_WHEN_PP, OP_UNLESS_PP,

      OP_COND_NA_NA, OP_COND_NA_NP, OP_COND_NA_NP_1, OP_COND_NA_2E, OP_COND_NA_3E, OP_COND_NA_NP_O,
      OP_COND_FEED, OP_COND_FEED_1,

      OP_SIMPLE_DO, OP_SIMPLE_DO_STEP, OP_SAFE_DOTIMES, OP_SAFE_DOTIMES_STEP, OP_SAFE_DOTIMES_STEP_O,
      OP_SAFE_DO, OP_SAFE_DO_STEP, OP_DOX, OP_DOX_STEP, OP_DOX_STEP_O, OP_DOX_NO_BODY, OP_DOX_PENDING_NO_BODY, OP_DOX_INIT,
      OP_DOTIMES_P, OP_DOTIMES_STEP_O,
      OP_DO_NO_VARS, OP_DO_NO_VARS_NO_OPT, OP_DO_NO_VARS_NO_OPT_1,
      OP_DO_NO_BODY_NA_VARS, OP_DO_NO_BODY_NA_VARS_STEP, OP_DO_NO_BODY_NA_VARS_STEP_1,

      OP_SAFE_C_P_1, OP_SAFE_C_PP_1, OP_SAFE_C_PP_3_MV, OP_SAFE_C_PP_5,
      OP_SAFE_C_3P_1, OP_SAFE_C_3P_2, OP_SAFE_C_3P_3, OP_SAFE_C_3P_1_MV, OP_SAFE_C_3P_2_MV, OP_SAFE_C_3P_3_MV,
      OP_SAFE_C_SP_1, OP_SAFE_CONS_SP_1, OP_SAFE_ADD_SP_1, OP_SAFE_MULTIPLY_SP_1, OP_SAFE_C_PS_1, OP_SAFE_C_PC_1,
      OP_EVAL_MACRO_MV, OP_MACROEXPAND_1, OP_APPLY_LAMBDA,
      OP_ANY_C_NP_1, OP_ANY_C_NP_MV, OP_SAFE_C_SSP_1, OP_C_P_1, OP_C_AP_1, OP_ANY_C_NP_2, OP_SAFE_C_PA_1,
      OP_SET_WITH_LET_1, OP_SET_WITH_LET_2,

      OP_CLOSURE_AP_1, OP_CLOSURE_PA_1, OP_CLOSURE_PP_1, OP_CLOSURE_P_1,
      OP_SAFE_CLOSURE_P_1, OP_SAFE_CLOSURE_P_A_1, OP_SAFE_CLOSURE_AP_1, OP_SAFE_CLOSURE_PA_1, OP_SAFE_CLOSURE_PP_1,
      OP_ANY_CLOSURE_3P_1, OP_ANY_CLOSURE_3P_2, OP_ANY_CLOSURE_3P_3, OP_ANY_CLOSURE_NP_1,
      OP_ANY_CLOSURE_4P_1, OP_ANY_CLOSURE_4P_2, OP_ANY_CLOSURE_4P_3, OP_ANY_CLOSURE_4P_4, OP_ANY_CLOSURE_NP_2,

      OP_TC_AND_A_OR_A_LA, OP_TC_OR_A_AND_A_LA, OP_TC_AND_A_OR_A_L2A, OP_TC_OR_A_AND_A_L2A, OP_TC_AND_A_OR_A_L3A, OP_TC_OR_A_AND_A_L3A,
      OP_TC_OR_A_A_AND_A_A_LA, OP_TC_OR_A_AND_A_A_L3A, OP_TC_AND_A_OR_A_A_LA, OP_TC_OR_A_AND_A_A_LA,
      OP_TC_WHEN_LA, OP_TC_WHEN_L2A, OP_TC_WHEN_L3A, OP_TC_LET_WHEN_L2A,
      OP_TC_COND_A_Z_A_L2A_L2A, OP_TC_LET_COND, OP_TC_COND_N,
      OP_TC_IF_A_Z_LA, OP_TC_IF_A_Z_L2A, OP_TC_IF_A_Z_L3A,
      OP_TC_IF_A_Z_IF_A_Z_LA, OP_TC_IF_A_Z_IF_A_LA_Z, OP_TC_IF_A_Z_IF_A_Z_L2A, OP_TC_IF_A_Z_IF_A_L2A_Z,
      OP_TC_IF_A_Z_IF_A_Z_L3A, OP_TC_IF_A_Z_IF_A_L3A_Z, OP_TC_IF_A_Z_IF_A_L3A_L3A,
      OP_TC_LET_IF_A_Z_LA, OP_TC_LET_IF_A_Z_L2A, OP_TC_IF_A_Z_LET_IF_A_Z_L2A,
      OP_TC_AND_A_IF_A_Z_LA, OP_TC_AND_A_IF_A_LA_Z,
      OP_TC_CASE_LA, OP_TC_CASE_L2A, OP_TC_CASE_L3A, /* treat this as last tc op (see below) */

      OP_RECUR_IF_A_A_opLA_LAq, OP_RECUR_IF_A_A_opL2A_L2Aq, OP_RECUR_IF_A_A_opL3A_L3Aq,
      OP_RECUR_IF_A_A_opA_LAq, OP_RECUR_IF_A_A_opA_L2Aq, OP_RECUR_IF_A_A_opA_L3Aq,
      OP_RECUR_IF_A_A_opLA_LA_LAq, OP_RECUR_IF_A_A_AND_A_L2A_L2A, OP_RECUR_IF_A_A_opA_LA_LAq,
      OP_RECUR_IF_A_A_IF_A_A_opLA_LAq, OP_RECUR_IF_A_A_IF_A_A_opL2A_L2Aq, OP_RECUR_IF_A_A_IF_A_A_opL3A_L3Aq,
      OP_RECUR_IF_A_A_IF_A_L2A_opA_L2Aq, OP_RECUR_COND_A_A_A_A_opA_L2Aq,
      OP_RECUR_COND_A_A_A_L2A_LopA_L2Aq, OP_RECUR_AND_A_OR_A_L2A_L2A,

      NUM_OPS};

#define is_tc_op(Op) ((Op >= OP_TC_AND_A_OR_A_LA) && (Op <= OP_TC_CASE_L3A))
#define is_safe_c_op(op)            ((op >= OP_SAFE_C_NC) && (op < OP_THUNK))
#define is_safe_closure_op(op)      ((op >= OP_SAFE_CLOSURE_S) && (op < OP_ANY_CLOSURE_3P))
#define is_safe_closure_star_op(op) ((op >= OP_SAFE_CLOSURE_STAR_A) && (op < OP_C_SS))
#define is_unknown_op(op)           ((op >= OP_UNKNOWN) && (op <= OP_UNKNOWN_NP))

#endif /* S7_INTERNAL_H */
