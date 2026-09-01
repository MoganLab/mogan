#include "s7_internal.h"
#include "s7_scheme_read.h"
#include "s7_ctables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------- local macros (mirrors of s7.c internals, via s7_internal.h types) -------- */

#ifndef port_file
  #define port_file(p)                    port_port(p)->file
#endif
#ifndef port_data
  #define port_data(p)                    (T_Prt(p))->object.prt.data
#endif
#ifndef port_data_size
  #define port_data_size(p)               (T_Prt(p))->object.prt.size
#endif
#ifndef port_position
  #define port_position(p)                (T_Prt(p))->object.prt.point
#endif
#ifndef port_block
  #define port_block(p)                   (T_Prt(p))->object.prt.block
#endif
#ifndef port_data_block
  #define port_data_block(p)              port_port(p)->block
#endif
#ifndef port_type
  #define port_type(p)                    port_port(p)->ptype
#endif
#ifndef port_line_number
  #define port_line_number(p)             port_port(p)->line_number
#endif
#ifndef port_file_number
  #define port_file_number(p)             port_port(p)->file_number
#endif
#ifndef port_filename
  #define port_filename(p)                port_port(p)->filename
#endif
#ifndef port_filename_length
  #define port_filename_length(p)         port_port(p)->filename_length
#endif
#ifndef port_needs_free
  #define port_needs_free(p)              port_port(p)->needs_free
#endif
#ifndef port_set_closed
  #define port_set_closed(p, Val)         port_port(p)->is_closed = Val
#endif
#ifndef port_set_string_or_function
  #define port_set_string_or_function(p, S) port_port(p)->orig_str = S
#endif
#ifndef port_input_function
  #define port_input_function(p)          port_port(p)->input_function
#endif
#ifndef port_is_closed
  #define port_is_closed(p)               port_port(p)->is_closed
#endif
#ifndef port_read_character
  #define port_read_character(p)          port_port(p)->pf->read_character
#endif
#ifndef port_read_line
  #define port_read_line(p)               port_port(p)->pf->read_line
#endif
#ifndef port_read_white_space
  #define port_read_white_space(p)        port_port(p)->pf->read_white_space
#endif

#ifndef is_input_port
  #define is_input_port(p)                (type(p) == T_INPUT_PORT)
#endif
#ifndef is_string_port
  #define is_string_port(p)               (port_type(p) == string_port)
#endif
#ifndef is_function_port
  #define is_function_port(p)             (port_type(p) == function_port)
#endif
#ifndef is_file_port
  #define is_file_port(p)                 (port_type(p) == file_port)
#endif
#ifndef is_character
  #define is_character(p)                 (type(p) == T_CHARACTER)
#endif
#ifndef character
  #define character(p)                    (T_Chr(p))->object.chr.c
#endif
#ifndef is_multiple_value
  #define is_multiple_value(p)            has_low_type_bit(T_Exs(p), T_MULTIPLE_VALUE)
#endif
#ifndef clear_multiple_value
  #define clear_multiple_value(p)         clear_low_type_bit(T_Pair(p), T_MULTIPLE_VALUE)
#endif
#ifndef is_white_space
  #define is_white_space(C)               white_space[C]
#endif
#ifndef is_eof
  #define is_eof(p)                       ((T_Ext(p)) == eof_object)
#endif
#ifndef current_input_port
  #define current_input_port(Sc)          T_Pri(Sc->input_port)
#endif

#ifndef push_stack_direct
  #define push_stack_direct(Sc, Op) \
    do { \
        Sc->cur_op = Op; \
        memcpy((void *)(Sc->stack_end), (void *)Sc, 4 * sizeof(s7_pointer)); \
        Sc->stack_end += 4; \
    } while (0)
#endif
#ifndef stack_top_op
  #define stack_top_op(Sc)                ((opcode_t)T_Op(Sc->stack_end[-1]))
#endif

#ifndef clamp_length
  #define clamp_length(NLen, Len)         (((NLen) < (Len)) ? (NLen) : (Len))
#endif

#ifndef SYMBOL_OK
  #define SYMBOL_OK true
#endif
#ifndef WITH_OVERFLOW_ERROR
  #define WITH_OVERFLOW_ERROR true
#endif

#ifndef add_input_port
  #define add_input_port(sc, p)           add_to_gc_list(sc, sc->input_ports, p)
#endif

/* s7.c exports (not in a shared header: s7_liii_string.c defines its own same-named helpers) */
s7_pointer method_or_bust(s7_scheme *sc, s7_pointer obj, s7_pointer method, s7_pointer args, s7_pointer typ, int32_t num);
s7_pointer method_or_bust_p(s7_scheme *sc, s7_pointer obj, s7_pointer method, s7_pointer typ);
s7_pointer method_or_bust_pp(s7_scheme *sc, s7_pointer obj, s7_pointer method, s7_pointer x1, s7_pointer x2, s7_pointer typ, int32_t num);

#define declare_jump_info() bool old_longjmp; setjmp_loc_t old_jump_loc; jump_loc_t jump_loc; Jmp_Buf *old_goto_start; Jmp_Buf new_goto_start

#define store_jump_info(Sc)			\
  do {						\
      old_longjmp = Sc->longjmp_ok;		\
      old_jump_loc = Sc->setjmp_loc;		\
      old_goto_start = Sc->goto_start;		\
  } while (0)

#define restore_jump_info(Sc)			\
  do {						\
    Sc->longjmp_ok = old_longjmp;		\
    Sc->setjmp_loc = old_jump_loc;		\
    Sc->goto_start = old_goto_start;		\
    if ((jump_loc == error_jump) &&		\
	(Sc->longjmp_ok))			\
      LongJmp(*(Sc->goto_start), error_jump);	\
  } while (0)

#define set_jump_info(Sc, Tag)			\
  do {						\
    Sc->longjmp_ok = true;			\
    Sc->setjmp_loc = Tag;			\
    jump_loc = (jump_loc_t)SetJmp(new_goto_start, 1);	\
    Sc->goto_start = &new_goto_start;		\
  } while (0)


/* -------- read character functions -------- */

int32_t file_read_char(s7_scheme *sc, s7_pointer port)
{
  int32_t c = fgetc(port_file(port));
  if ((c == (int32_t)'\n') && (!s7i_is_loader_port(port))) port_line_number(port)++;
  return(c);
}

int32_t function_read_char(s7_scheme *sc, s7_pointer port)
{
  const s7_pointer result = (*(port_input_function(port)))(sc, S7_READ_CHAR, port);
  if (is_eof(result)) return(EOF);
  if (!is_character(result))          /* port_input_function might return some non-character */
    {
      if (is_multiple_value(result))
	{
	  clear_multiple_value(result);
	  s7i_error_nr(sc, sc->bad_result_symbol, s7i_set_elist_2(sc, s7i_wrap_string(sc, "input-function-port read-char returned: ~S", 42), result));
	}
      s7i_error_nr(sc, sc->wrong_type_arg_symbol, s7i_set_elist_2(sc, s7i_wrap_string(sc, "input-function-port read-char returned: ~S", 42), result));
    }
  return((int32_t)character(result));    /* kinda nutty -- we return chars[this] in g_read_char! */
}

int32_t string_read_char(s7_scheme *sc, s7_pointer port)
{
  uint8_t c;
  if (port_data_size(port) <= port_position(port)) return(EOF);
  c = (uint8_t)port_data(port)[port_position(port)++]; /* port_string_length is 0 if no port string, port_data is uint8_t* */
  if ((c == (uint8_t)'\n') && (!s7i_is_loader_port(port))) port_line_number(port)++;
  return(c);
}

int32_t output_read_char(s7_scheme *sc, s7_pointer port) /* not reachable I think */
{
  sole_arg_wrong_type_error_nr(sc, sc->read_char_symbol, port, s7i_an_input_port_string_obj());
  return(0);
}

int32_t closed_port_read_char(s7_scheme *sc, s7_pointer port)
{
  sole_arg_wrong_type_error_nr(sc, sc->read_char_symbol, port, s7i_an_open_input_port_string_obj());
  return(0);
}


/* -------- read line functions -------- */

s7_pointer output_read_line(s7_scheme *sc, s7_pointer port, bool with_eol) /* not reachable I think */
{
  sole_arg_wrong_type_error_nr(sc, sc->read_line_symbol, port, s7i_an_input_port_string_obj());
  return(NULL);
}

s7_pointer closed_port_read_line(s7_scheme *sc, s7_pointer port, bool with_eol)
{
  sole_arg_wrong_type_error_nr(sc, sc->read_line_symbol, port, s7i_an_open_input_port_string_obj());
  return(NULL);
}

s7_pointer function_read_line(s7_scheme *sc, s7_pointer port, bool with_eol)
{
  s7_pointer result = (*(port_input_function(port)))(sc, S7_READ_LINE, port);
  if (is_multiple_value(result))
    {
      clear_multiple_value(result);
      s7i_error_nr(sc, sc->bad_result_symbol, s7i_set_elist_2(sc, s7i_wrap_string(sc, "input-function-port read-line returned: ~S", 42), result));
    }
  return(result);
}

s7_pointer stdin_read_line(s7_scheme *sc, s7_pointer port, bool with_eol)
{
  if (!sc->read_line_buf)
    {
      sc->read_line_buf_size = 1024;
      sc->read_line_buf = (char *)malloc(sc->read_line_buf_size);
    }
  if (fgets(sc->read_line_buf, sc->read_line_buf_size, stdin))
    return(s7_make_string(sc, sc->read_line_buf)); /* fgets adds the trailing '\0' */
  return(eof_object);
}

s7_pointer file_read_line(s7_scheme *sc, s7_pointer port, bool with_eol)
{
  /* read into read_line_buf concatenating reads until newline found.  string is read_line_buf to pos-of-newline.
   *   reset file position to reflect newline pos.
   */
  int32_t reads = 0;
  char *str;
  s7_int read_size;
  if (!sc->read_line_buf)
    {
      sc->read_line_buf_size = 1024;
      sc->read_line_buf = (char *)malloc(sc->read_line_buf_size);
    }
  read_size = sc->read_line_buf_size;
  str = fgets(sc->read_line_buf, read_size, port_file(port)); /* reads size-1 at most, EOF and newline also terminate read */
  if (!str) return(eof_object);                               /* EOF or error with no char read */

  while (true)
    {
      s7_int cur_size;
      char *buf;
      const char *snew = strchr(sc->read_line_buf, (int)'\n'); /* or maybe just strlen + end-of-string=newline */
      if (snew)
	{
	  s7_int pos = (s7_int)(snew - sc->read_line_buf);
	  port_line_number(port)++;
	  return(s7i_make_string_with_length(sc, sc->read_line_buf, (with_eol) ? (pos + 1) : pos));
	}
      reads++;
      cur_size = strlen(sc->read_line_buf);
      if ((cur_size + reads) < read_size) /* end of data, no newline */
	return(s7i_make_string_with_length(sc, sc->read_line_buf, cur_size));

      /* need more data */
      sc->read_line_buf_size *= 2;
      sc->read_line_buf = (char *)realloc(sc->read_line_buf, sc->read_line_buf_size);
      buf = (char *)(sc->read_line_buf + cur_size);
      str = fgets(buf, read_size, port_file(port));
      if (!str) return(eof_object);
      read_size = sc->read_line_buf_size;
    }
  return(eof_object);
}

s7_pointer string_read_line(s7_scheme *sc, s7_pointer port, bool with_eol)
{
  s7_int i;
  const char *port_str = (const char *)port_data(port);
  const s7_int port_start = port_position(port);
  const char *start = port_str + port_start;
  const char *cur = (const char *)strchr(start, (int)'\n'); /* this can run off the end making valgrind unhappy, but I think it's innocuous */
  if (cur)
    {
      s7_int len;
      port_line_number(port)++;
      i = cur - port_str;
      port_position(port) = i + 1;
      len = ((with_eol) ? i + 1 : i) - port_start;
      if (len == 0) return(s7i_nil_string());
      return(s7i_make_string_with_length(sc, start, len));
    }
  i = port_data_size(port);
  port_position(port) = i;
  if (i <= port_start)         /* the < part can happen -- if not caught we try to create a string of length - 1 -> segfault */
    return(eof_object);
  return(s7i_make_string_with_length(sc, start, i - port_start));
}


/* -------- skip to newline readers -------- */

token_t file_read_semicolon(s7_scheme *sc, s7_pointer port)
{
  int32_t c;
  do (c = fgetc(port_file(port))); while ((c != '\n') && (c != EOF));
  port_line_number(port)++;
  return((c == EOF) ? token_eof : s7i_token(sc));
}

token_t string_read_semicolon(s7_scheme *sc, s7_pointer port)
{
  const char *str = (const char *)(port_data(port) + port_position(port));
  const char *orig_str = strchr(str, (int)'\n');
  if (!orig_str)
    {
      port_position(port) = port_data_size(port);
      return(token_eof);
    }
  port_position(port) += (orig_str - str + 1); /* + 1 because strchr leaves orig_str pointing at the newline */
  port_line_number(port)++;
  return(s7i_token(sc));
}


/* -------- white space readers -------- */

int32_t file_read_white_space(s7_scheme *sc, s7_pointer port)
{
  int32_t c;
  while (is_white_space(c = fgetc(port_file(port))))
    if (c == '\n')
      port_line_number(port)++;
  return(c);
}

int32_t terminated_string_read_white_space(s7_scheme *sc, s7_pointer port)
{
  const uint8_t *str = (const uint8_t *)(port_data(port) + port_position(port));
  uint8_t c;
  /* here we know we have null termination and white_space[#\null] is false */
  while (white_space[c = *str++]) /* 255 is not -1 = EOF */
    if (c == '\n')
      port_line_number(port)++;
  port_position(port) = (c) ? str - port_data(port) : port_data_size(port);
  return((int32_t)c);
}


/* -------- name readers -------- */
#define BASE_10 10

static s7_pointer file_read_name_or_sharp(s7_scheme *sc, s7_pointer port, bool atom_case)
{
  int32_t c;
  s7_int i = 1;  /* sc->strbuf[0] has the first char of the string we're reading */
  do {
    c = fgetc(port_file(port)); /* might return EOF */
    if (c == '\n')
      port_line_number(port)++;

    sc->strbuf[i++] = (unsigned char)c;
    if (i >= sc->strbuf_size)
      s7i_resize_strbuf(sc, i);
  } while ((c != EOF) && (char_ok_in_a_name[c]));

  if ((i == 2) &&
      (sc->strbuf[0] == '\\'))
    sc->strbuf[2] = '\0';
  else
    {
      if (c != EOF)
	{
	  if (c == '\n')
	    port_line_number(port)--;
	  ungetc(c, port_file(port));
	}
      sc->strbuf[i - 1] = '\0';
    }
  if (atom_case)
    return(make_atom(sc, sc->strbuf, BASE_10, SYMBOL_OK, WITH_OVERFLOW_ERROR));
  return(s7i_make_sharp_constant(sc, sc->strbuf, WITH_OVERFLOW_ERROR, port, true));
}

s7_pointer file_read_name(s7_scheme *sc, s7_pointer port)  {return(file_read_name_or_sharp(sc, port, true));}
s7_pointer file_read_sharp(s7_scheme *sc, s7_pointer port) {return(file_read_name_or_sharp(sc, port, false));}

s7_pointer string_read_name_no_free(s7_scheme *sc, s7_pointer port)
{
  /* sc->strbuf[0] has the first char of the string we're reading */
  const uint8_t *str = (uint8_t *)(port_data(port) + port_position(port));

  if (char_ok_in_a_name[*str])
    {
      s7_int k;
      const uint8_t *orig_str = str - 1;
      str++;
      while (char_ok_in_a_name[*str]) str++;
      k = str - orig_str;
      if (*str != 0)
	port_position(port) += (k - 1);
      else port_position(port) = port_data_size(port);
      /* this is equivalent to:
       *    str = strpbrk(str, "(); \"\t\r\n");
       *    if (!str) {k = strlen(orig_str); str = (char *)(orig_str + k);} else k = str - orig_str;
       * but slightly faster.
       */
      if (!number_table[*orig_str])
	return(s7i_make_symbol_with_length(sc, (const char *)orig_str, k));

      /* eval_c_string string is a constant so we can't set and unset the token's end char */
      if ((k + 1) >= sc->strbuf_size)
	s7i_resize_strbuf(sc, k + 1);
      memcpy((void *)(sc->strbuf), (void *)orig_str, k);
      sc->strbuf[k] = '\0';
      return(make_atom(sc, sc->strbuf, BASE_10, SYMBOL_OK, WITH_OVERFLOW_ERROR));
    }
  {
    s7_pointer result = sc->singletons[(uint8_t)(sc->strbuf[0])];
    if (!result)
      {
	sc->strbuf[1] = '\0';
	result = s7_make_symbol(sc, sc->strbuf);
	sc->singletons[(uint8_t)(sc->strbuf[0])] = result;
      }
    return(result);
  }
}

s7_pointer string_read_sharp(s7_scheme *sc, s7_pointer port)
{
  /* sc->strbuf[0] has the first char of the string we're reading.
   *   since a *#readers* function might want to get further input, we can't mess with the input even when it is otherwise safe
   */
  char *str = (char *)(port_data(port) + port_position(port));
  if (char_ok_in_a_name[(uint8_t)*str])
    {
      s7_int k;
      const char *orig_str = (char *)(str - 1);
      str++;
      while (char_ok_in_a_name[(uint8_t)(*str)]) {str++;}
      k = str - orig_str;
      port_position(port) += (k - 1);
      if ((k + 1) >= sc->strbuf_size)
	s7i_resize_strbuf(sc, k + 1);
      memcpy((void *)(sc->strbuf), (void *)orig_str, k);
      sc->strbuf[k] = '\0';
      return(s7i_make_sharp_constant(sc, sc->strbuf, WITH_OVERFLOW_ERROR, port, true));
    }
  if (sc->strbuf[0] == 'f') return(sc->F);
  if (sc->strbuf[0] == 't') return(sc->T);
  if (sc->strbuf[0] == '\\')
    {
      /* must be from #\( and friends -- a character that happens to be not ok-in-a-name */
      sc->strbuf[1] = str[0];
      sc->strbuf[2] = '\0';
      port_position(port)++;
    }
  else sc->strbuf[1] = '\0';
  return(s7i_make_sharp_constant(sc, sc->strbuf, WITH_OVERFLOW_ERROR, port, true));
}

s7_pointer string_read_name(s7_scheme *sc, s7_pointer port)
{
  /* port_string was allocated (and read from a file) so we can mess with it directly */
  s7_pointer result;
  uint8_t *str = (uint8_t *)(port_data(port) + port_position(port));
  if (char_ok_in_a_name[*str])
    {
      s7_int k;
      uint8_t endc;
      const uint8_t *orig_str = str - 1;
      str++;
      while (char_ok_in_a_name[*str]) str++;
      k = str - orig_str;
      port_position(port) += (k - 1);
      if (!number_table[*orig_str])
	return(s7i_make_symbol_with_length(sc, (const char *)orig_str, k));
      endc = *str;
      *str = 0; /* temp end for make_atom */
      result = make_atom(sc, (char *)orig_str, BASE_10, SYMBOL_OK, WITH_OVERFLOW_ERROR);
      *str = endc;
      return(result);
    }
  result = sc->singletons[(uint8_t)(sc->strbuf[0])];
  if (!result)
    {
      sc->strbuf[1] = '\0';
      result = s7_make_symbol(sc, sc->strbuf);
      sc->singletons[(uint8_t)(sc->strbuf[0])] = result;
    }
  return(result);
}


/* -------- file port creation -------- */

s7_pointer read_file(s7_scheme *sc, FILE *fp, const char *name, s7_int max_size, const char *caller)
{
  s7_pointer port;
  s7_int size;
  block_t *b = s7i_mallocate_port(sc);
  new_cell(sc, port, T_INPUT_PORT);
  gc_protect_via_stack(sc, port);
  port_block(port) = b;
  port_port(port) = (port_t *)s7i_block_data(b);
  port_set_closed(port, false);
  port_set_string_or_function(port, sc->nil);
  port_filename_length(port) = s7i_safe_strlen(name);
  s7i_port_set_filename(sc, port, name, port_filename_length(port));
  port_line_number(port) = 1;  /* first line is numbered 1 */
  port_file_number(port) = 0;
  add_input_port(sc, port);

#if MS_WINDOWS
  /* MS C's fseek/ftell truncate large files: use the 64-bit-safe variants */
  if ((_fseeki64(fp, 0, SEEK_END) != 0) ||
      ((size = _ftelli64(fp)) < 0))
    size = 0;
  rewind(fp);
#else
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  rewind(fp);
#endif
  /* pseudo files (under /proc for example) have size=0, but we can read them, so don't assume a 0 length file is empty */
  if ((size > 0) &&   /* if (size != 0) we get (open-input-file "/dev/tty") -> (open "/dev/tty") read 0 bytes of an expected -1? */
      ((max_size < 0) || (size < max_size))) /* load uses max_size = -1 */
    {
      block_t *block = s7i_mallocate(sc, size + 2);
      uint8_t *content = (uint8_t *)(s7i_block_data(block));
      const size_t bytes = fread(content, sizeof(uint8_t), size, fp);
      if (bytes != (size_t)size)
	{
	  /* in MS Windows text mode, CRLF -> LF translation makes the read size smaller than the file size */
	  if (ferror(fp) || (bytes == 0))
	    {
	      if (s7i_current_output_port(sc) != sc->F)
		{
		  char tmp[256];
		  int32_t len = snprintf(tmp, 256, "(%s \"%s\") read %ld bytes of an expected %" ld64 "?", caller, name, (long)bytes, size);
		  port_write_string(s7i_current_output_port(sc))(sc, tmp, clamp_length(len, 256), s7i_current_output_port(sc));
		}
	    }
	  size = bytes;
	}
      content[size] = '\0';
      content[size + 1] = '\0';
      fclose(fp);

      port_file(port) = NULL; /* make valgrind happy */
      port_type(port) = string_port;
      port_data(port) = content;
      port_data_block(port) = block;
      port_data_size(port) = size;
      port_position(port) = 0;
      port_needs_free(port) = true;
      port_port(port)->pf = s7i_input_string_functions_1();
    }
  else
    {
      port_file(port) = fp;
      port_type(port) = file_port;
      port_data(port) = NULL;
      port_data_block(port) = NULL;
      port_data_size(port) = 0;
      port_position(port) = 0;
      port_needs_free(port) = false;
      port_port(port)->pf = s7i_input_file_functions();
    }
  unstack_gc_protect(sc);
  return(port);
}


/* -------- current-input-port handling -------- */

s7_pointer input_port_if_not_loading(s7_scheme *sc)
{
  const s7_pointer port = current_input_port(sc);
  int32_t c;
  if (!s7i_is_loader_port(port)) /* this flag is turned off by the reader macros, so we aren't in that context */
    return(port);
  c = port_read_white_space(port)(sc, port);
  if (c > 0)            /* we can get either EOF or NULL at the end */
    {
      s7i_backchar(c, port);
      return(NULL);
    }
  return(sc->standard_input);
}

s7_pointer s7i_input_port_if_not_loading(s7_scheme *sc)
{
  return(input_port_if_not_loading(sc));
}

s7_pointer s7i_port_read_line(s7_scheme *sc, s7_pointer port, bool with_eol)
{
  return(port_read_line(port)(sc, port, with_eol));
}


/* -------------------------------- read-char -------------------------------- */
s7_pointer s7_read_char(s7_scheme *sc, s7_pointer port)
{
  int32_t c = port_read_character(port)(sc, port);
  return((c == EOF) ? eof_object : chars[c]);
}

s7_pointer g_read_char(s7_scheme *sc, s7_pointer args)
{
  #define H_read_char "(read-char (port (current-input-port))) returns the next character in the input port"
  #define Q_read_char s7_make_signature(sc, 2, s7_make_signature(sc, 2, sc->is_char_symbol, sc->is_eof_object_symbol), sc->is_input_port_symbol)

  s7_pointer port;
  if (is_pair(args))
    port = car(args);
  else
    {
      port = input_port_if_not_loading(sc);
      if (!port) return(eof_object);
    }
  if (!is_input_port(port))
    return(method_or_bust_p(sc, port, sc->read_char_symbol, s7i_an_input_port_string_obj()));
  return(chars[port_read_character(port)(sc, port)]);
}

s7_pointer read_char_p_p(s7_scheme *sc, s7_pointer port)
{
  if (!is_input_port(port))
    return(method_or_bust_p(sc, port, sc->read_char_symbol, s7i_an_input_port_string_obj()));
  return(chars[port_read_character(port)(sc, port)]);
}

s7_pointer g_read_char_1(s7_scheme *sc, s7_pointer args)
{
  s7_pointer port = car(args);
  if (!is_input_port(port))
    return(method_or_bust_p(sc, port, sc->read_char_symbol, s7i_an_input_port_string_obj()));
  return(chars[port_read_character(port)(sc, port)]);
}

s7_pointer read_char_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer unused_expr)
{
  return((args == 1) ? sc->read_char_1 : func);
}


/* -------------------------------- peek-char -------------------------------- */
s7_pointer s7_peek_char(s7_scheme *sc, s7_pointer port)
{
  int32_t c;              /* needs to be an int32_t so EOF=-1, but not 255 */
  if (is_string_port(port))
    return((port_data_size(port) <= port_position(port)) ? eof_object : chars[(uint8_t)port_data(port)[port_position(port)]]);
  c = port_read_character(port)(sc, port);
  if (c == EOF) return(eof_object);
  s7i_backchar(c, port);
  return(chars[c]);
}

s7_pointer g_peek_char(s7_scheme *sc, s7_pointer args)
{
  #define H_peek_char "(peek-char (port (current-input-port))) returns the next character in the input port, but does not remove it from the input stream"
  #define Q_peek_char s7_make_signature(sc, 2, s7_make_signature(sc, 2, sc->is_char_symbol, sc->is_eof_object_symbol), sc->is_input_port_symbol)

  s7_pointer result;
  const s7_pointer port = (is_pair(args)) ? car(args) : current_input_port(sc);
  if (!is_input_port(port))
    return(method_or_bust_p(sc, port, sc->peek_char_symbol, s7i_an_input_port_string_obj()));
  if (port_is_closed(port))
    sole_arg_wrong_type_error_nr(sc, sc->peek_char_symbol, port, s7i_an_open_input_port_string_obj());
  if (!is_function_port(port))
    return(s7_peek_char(sc, port));

  result = (*(port_input_function(port)))(sc, S7_PEEK_CHAR, port);
  if (is_multiple_value(result))
    {
      clear_multiple_value(result);
      s7i_error_nr(sc, sc->bad_result_symbol,
	       s7i_set_elist_2(sc, s7i_wrap_string(sc, "input-function-port peek-char returned multiple values: ~S", 58), result));
    }
  if (!is_character(result))
    s7i_error_nr(sc, sc->wrong_type_arg_symbol,
	     s7i_set_elist_2(sc, s7i_wrap_string(sc, "input-function-port peek-char returned: ~S", 42), result));
  return(result);
}


/* -------------------------------- read-byte -------------------------------- */
s7_pointer g_read_byte(s7_scheme *sc, s7_pointer args)
{
  #define H_read_byte "(read-byte (port (current-input-port))): reads a byte from the input port"
  #define Q_read_byte s7_make_signature(sc, 2, s7_make_signature(sc, 2, sc->is_byte_symbol, sc->is_eof_object_symbol), sc->is_input_port_symbol)

  s7_pointer port;
  int32_t c;
  if (is_pair(args))
    port = car(args);
  else
    {
      port = input_port_if_not_loading(sc);
      if (!port) return(eof_object);
    }
  if (!is_input_port(port))
    return(method_or_bust_p(sc, port, sc->read_byte_symbol, s7i_an_input_port_string_obj()));
  if (port_is_closed(port))          /* avoid reporting caller here as read-char */
    sole_arg_wrong_type_error_nr(sc, sc->read_byte_symbol, port, s7i_an_open_input_port_string_obj());
  c = port_read_character(port)(sc, port);
  return((c == EOF) ? eof_object : s7i_small_int(c));
}


/* -------------------------------- read-line -------------------------------- */
/* g_read_line is now implemented in s7_scheme_base.c */

s7_pointer read_line_p_pp(s7_scheme *sc, s7_pointer port, s7_pointer with_eol)
{
  if (!is_input_port(port))
    return(method_or_bust_pp(sc, port, sc->read_line_symbol, port, with_eol, s7i_an_input_port_string_obj(), 1));
  if (!is_boolean(with_eol))
    s7_wrong_type_arg_error(sc, "read-line", 2, with_eol, s7i_a_boolean_string());
  return(port_read_line(port)(sc, port, with_eol != sc->F));
}

s7_pointer read_line_p_p(s7_scheme *sc, s7_pointer port)
{
  if (!is_input_port(port))
    return(method_or_bust_p(sc, port, sc->read_line_symbol, s7i_an_input_port_string_obj()));
  return(port_read_line(port)(sc, port, false)); /* with_eol default is #f */
}


/* -------------------------------- read-string -------------------------------- */
#define READ_STRING_LINE_NUMBERS 0 /* 1 adds port-line-number support to read-string, doubling the time it takes */

s7_pointer g_read_string(s7_scheme *sc, s7_pointer args)
{
  /* read-chars would be a better name -- read-string could mean CL-style read-from-string (like eval-string)
   *   similarly read-bytes could return a byte-vector (rather than r7rs's read-bytevector)
   *   and write-string -> write-chars, write-bytevector -> write-bytes.
   * should this worry about newlines?  read-char and read-line keep port-line-number up to date,
   *   but here we'd need to scan the new string (via strchr?) or xor with \n\n\n\n... up to 8-at-a-time, and count zeros.
   */
  #define H_read_string "(read-string k port) reads k characters from port into a new string and returns it."
  #define Q_read_string s7_make_signature(sc, 3, \
                          s7_make_signature(sc, 2, sc->is_string_symbol, sc->is_eof_object_symbol), \
                          sc->is_integer_symbol, sc->is_input_port_symbol)
  const s7_pointer k = car(args);
  s7_pointer port, str;
  s7_int nchars;
  uint8_t *str_chars;

  if (!s7_is_integer(k))
    return(method_or_bust(sc, k, sc->read_string_symbol, args, sc->type_names[T_INTEGER], 1));
  nchars = s7i_integer_clamped_if_gmp(sc, k);
  if (nchars < 0)
    s7_out_of_range_error(sc, "read-string", 1, k, "it is negative");
  if (nchars > sc->max_string_length)
    s7i_error_nr(sc, sc->out_of_range_symbol,
	     s7i_set_elist_3(sc, s7i_wrap_string(sc, "read-string first argument ~D is greater than (*s7* 'max-string-length), ~D", 75),
			 s7i_wrap_integer(sc, nchars), s7i_wrap_integer(sc, sc->max_string_length)));
  if (!is_null(cdr(args)))
    port = cadr(args);
  else
    {
      port = input_port_if_not_loading(sc);
      if (!port) return(eof_object);
    }
  if (!is_input_port(port))
    return(method_or_bust_pp(sc, port, sc->read_string_symbol, k, port, s7i_an_input_port_string_obj(), 2));
  if (port_is_closed(port))
    sole_arg_wrong_type_error_nr(sc, sc->read_string_symbol, port, s7i_an_open_input_port_string_obj());

  str = s7i_make_empty_string(sc, nchars, '\0');
  if (nchars == 0) return(str);
  str_chars = (uint8_t *)s7i_string_value(str);
  if (is_string_port(port))
    {
      const s7_int pos = port_position(port);
      const s7_int end = port_data_size(port);
      s7_int len = end - pos;
      if (len > nchars) len = nchars;
      if (len <= 0) return(eof_object);
      memcpy((void *)str_chars, (void *)(port_data(port) + pos), len);
      s7i_set_string_length(str, len);
      str_chars[len] = '\0';
      port_position(port) += len;
#if READ_STRING_LINE_NUMBERS
      for (s7_int i = 0; i < len; i++) if (str_chars[i] == '\n') port_line_number(port)++;
#endif
      return(str);
    }
  if (is_file_port(port))
    {
      const s7_int len = (s7_int)fread((void *)str_chars, 1, nchars, port_file(port));
      str_chars[len] = '\0';
      s7i_set_string_length(str, len);
#if READ_STRING_LINE_NUMBERS
      for (s7_int i = 0; i < len; i++) if (str_chars[i] == '\n') port_line_number(port)++;
#endif
      return(str);
    }
  for (s7_int i = 0; i < nchars; i++)
    {
      const int32_t c = port_read_character(port)(sc, port);
      if (c == EOF)
	{
	  if (i == 0)
	    return(eof_object);
	  s7i_set_string_length(str, i);
	  return(str);
	}
      str_chars[i] = (uint8_t)c;
#if READ_STRING_LINE_NUMBERS
      if (c == '\n') port_line_number(port)++;
#endif
    }
  return(str);
}


/* -------------------------------- read -------------------------------- */
s7_pointer s7_read(s7_scheme *sc, s7_pointer port)
{
  if (!is_input_port(port))
    sole_arg_wrong_type_error_nr(sc, sc->read_symbol, port, s7i_an_input_port_string_obj());
  {
    const s7_pointer old_let = sc->curlet;
    declare_jump_info();
    set_curlet(sc, sc->rootlet);
    push_input_port(sc, port);
    store_jump_info(sc);
    set_jump_info(sc, read_set_jump);
    if (jump_loc != no_jump)
      {
	if (jump_loc != error_jump)
	  s7i_eval(sc, sc->cur_op);
      }
    else
      {
	push_stack_no_let_no_code(sc, OP_BARRIER, port);
	push_stack_direct(sc, OP_EVAL_DONE);
	s7i_eval(sc, OP_READ_INTERNAL);
	if (sc->tok == token_eof)
	  sc->value = eof_object;
	if ((sc->cur_op == OP_EVAL_DONE) && /* pushed above */
	    (stack_top_op(sc) == OP_BARRIER))
	  pop_stack(sc);
      }
    pop_input_port(sc);
    set_curlet(sc, old_let);
    restore_jump_info(sc);
    return(sc->value);
  }
}

s7_pointer g_read(s7_scheme *sc, s7_pointer args)
{
  #define H_read "(read (port (current-input-port))) returns the next object in the input port, or #<eof> at the end"
  #define Q_read s7_make_signature(sc, 2, sc->T, sc->is_input_port_symbol)

  s7_pointer port;
  if (is_pair(args))
    port = car(args);
  else
    {
      port = input_port_if_not_loading(sc);
      if (!port) return(eof_object);
    }
  if (!is_input_port(port))
    return(method_or_bust_p(sc, port, sc->read_symbol, s7i_an_input_port_string_obj()));

  if (is_function_port(port))
    {
      s7_pointer result = (*(port_input_function(port)))(sc, S7_READ, port);
      if (is_multiple_value(result))
	{
	  clear_multiple_value(result);
	  s7i_error_nr(sc, sc->bad_result_symbol, s7i_set_elist_2(sc, s7i_wrap_string(sc, "input-function-port read returned: ~S", 37), result));
	}
      return(result);
    }
  if ((is_string_port(port)) &&
      (port_data_size(port) <= port_position(port)))
    return(eof_object);

  push_input_port(sc, port);
  push_stack_op_let(sc, OP_READ_DONE); /* this stops the internal read process so we only get one form */
  push_stack_op_let(sc, OP_READ_INTERNAL);
  return(port);
}
