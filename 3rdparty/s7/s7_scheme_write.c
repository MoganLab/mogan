/* s7_scheme_write.c - write function implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_scheme_write.h"
#include "s7_internal_helpers.h"

#define IF_METHOD_EXISTS_RETURN_VALUE(Sc, Obj, Method_name, Args) \
  do { \
    if (s7i_has_active_methods(Sc, Obj)) { \
      s7_pointer _func_ = s7i_find_method_with_let(Sc, Obj, s7_make_symbol(Sc, Method_name)); \
      if (_func_ != s7_undefined(Sc)) \
        return s7_apply_function(Sc, _func_, Args); \
    } \
  } while (0)

/* -------------------------------- newline -------------------------------- */

void s7_newline(s7_scheme *sc, s7_pointer port)
{
  if (port != s7_f(sc))
    s7i_port_write_character(sc, (uint8_t)'\n', port);
}

s7_pointer g_newline(s7_scheme *sc, s7_pointer args)
{
  #define H_newline "(newline (port (current-output-port))) writes a carriage return to the port"
  #define Q_newline s7_make_signature(sc, 2, sc->is_char_symbol, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))

  const s7_pointer port = (s7_is_pair(args)) ? s7_car(args) : s7_current_output_port(sc);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return s7_make_character(sc, '\n');
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "newline", args);
      return s7_wrong_type_arg_error(sc, "newline", 1, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "newline", 1, port, "an open output port");
  s7_newline(sc, port);
  return s7_make_character(sc, '\n');
}

s7_pointer newline_p(s7_scheme *sc)
{
  s7_newline(sc, s7_current_output_port(sc));
  return s7_make_character(sc, '\n');
}

s7_pointer newline_p_p(s7_scheme *sc, s7_pointer port)
{
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return s7_make_character(sc, '\n');
      return s7i_method_or_bust_p(sc, port, "newline", "an output port");
    }
  s7_newline(sc, port);
  return s7_make_character(sc, '\n');
}


/* -------------------------------- write -------------------------------- */

s7_pointer s7_write(s7_scheme *sc, s7_pointer obj, s7_pointer port)
{
  if (port != s7_f(sc))
    {
      if (s7i_port_is_closed(port))
        return s7_wrong_type_arg_error(sc, "write", 2, port, "an open output port");
      s7i_object_out(sc, obj, port, S7I_P_WRITE);
    }
  return obj;
}

s7_pointer write_p_p(s7_scheme *sc, s7_pointer x)
{
  s7_pointer port = s7_current_output_port(sc);
  return (port == s7_f(sc)) ? x : s7i_object_out(sc, x, port, S7I_P_WRITE);
}

s7_pointer write_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer port)
{
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return x;
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "write", s7_cons(sc, x, s7_cons(sc, port, s7_nil(sc))));
      return s7_wrong_type_arg_error(sc, "write", 2, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "write", 2, port, "an open output port");
  return s7i_object_out(sc, x, port, S7I_P_WRITE);
}

s7_pointer g_write(s7_scheme *sc, s7_pointer args)
{
  #define H_write "(write obj (port (current-output-port))) writes (object->string obj) to the output port"
  #define Q_write s7_make_signature(sc, 3, sc->T, sc->T, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))
  IF_METHOD_EXISTS_RETURN_VALUE(sc, s7_car(args), "write", args);
  return write_p_pp(sc, s7_car(args), (s7_is_pair(s7_cdr(args))) ? s7_cadr(args) : s7_current_output_port(sc));
}

s7_pointer g_write_2(s7_scheme *sc, s7_pointer args)
{
  return write_p_pp(sc, s7_car(args), s7_cadr(args));
}


/* -------------------------------- display -------------------------------- */

s7_pointer s7_display(s7_scheme *sc, s7_pointer obj, s7_pointer port)
{
  if (port != s7_f(sc))
    {
      if (s7i_port_is_closed(port))
        return s7_wrong_type_arg_error(sc, "display", 2, port, "an open output port");
      s7i_object_out(sc, obj, port, S7I_P_DISPLAY);
    }
  return obj;
}

s7_pointer display_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer port)
{
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return x;
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "display", s7_cons(sc, x, s7_cons(sc, port, s7_nil(sc))));
      return s7_wrong_type_arg_error(sc, "display", 2, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "display", 2, port, "an open output port");
  IF_METHOD_EXISTS_RETURN_VALUE(sc, x, "display", s7_cons(sc, x, s7_cons(sc, port, s7_nil(sc))));
  return s7i_object_out(sc, x, port, S7I_P_DISPLAY);
}

s7_pointer g_display(s7_scheme *sc, s7_pointer args)
{
  #define H_display "(display obj (port (current-output-port))) prints obj"
  #define Q_display s7_make_signature(sc, 3, sc->T, sc->T, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))
  return display_p_pp(sc, s7_car(args), (s7_is_pair(s7_cdr(args))) ? s7_cadr(args) : s7_current_output_port(sc));
}

s7_pointer g_display_2(s7_scheme *sc, s7_pointer args)
{
  return display_p_pp(sc, s7_car(args), s7_cadr(args));
}

s7_pointer g_display_f(s7_scheme *sc, s7_pointer args)
{
  (void)sc;
  return s7_car(args);
}

s7_pointer display_p_p(s7_scheme *sc, s7_pointer x)
{
  s7_pointer port = s7_current_output_port(sc);
  if (port == s7_f(sc)) return x;
  IF_METHOD_EXISTS_RETURN_VALUE(sc, x, "display", s7_cons(sc, x, s7_nil(sc)));
  return s7i_object_out(sc, x, port, S7I_P_DISPLAY);
}


/* -------------------------------- write-char -------------------------------- */

s7_pointer s7_write_char(s7_scheme *sc, s7_pointer c, s7_pointer port)
{
  if (port != s7_f(sc))
    s7i_port_write_unicode_char(sc, s7_character(c), port);
  return c;
}

s7_pointer write_char_p_pp(s7_scheme *sc, s7_pointer c, s7_pointer port)
{
  if (!s7_is_character(c))
    return s7i_method_or_bust_pp(sc, c, "write-char", c, port, "a character", 1);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return c;
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "write-char", s7_cons(sc, c, s7_cons(sc, port, s7_nil(sc))));
      return s7_wrong_type_arg_error(sc, "write-char", 2, port, "an output port or #f");
    }
  s7i_port_write_unicode_char(sc, s7_character(c), port);
  return c;
}

s7_pointer write_char_p_p(s7_scheme *sc, s7_pointer c)
{
  if (!s7_is_character(c))
    return s7i_method_or_bust_p(sc, c, "write-char", "a character");
  s7_pointer port = s7_current_output_port(sc);
  if (port == s7_f(sc)) return c;
  s7i_port_write_unicode_char(sc, s7_character(c), port);
  return c;
}

s7_pointer g_write_char(s7_scheme *sc, s7_pointer args)
{
  #define H_write_char "(write-char char (port (current-output-port))) writes char to the output port"
  #define Q_write_char s7_make_signature(sc, 3, sc->is_char_symbol, sc->is_char_symbol, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))
  if (s7_is_null(sc, s7_cdr(args)))
    return write_char_p_p(sc, s7_car(args));
  return write_char_p_pp(sc, s7_car(args), (s7_is_pair(s7_cdr(args))) ? s7_cadr(args) : s7_current_output_port(sc));
}


/* -------------------------------- write-string -------------------------------- */

s7_pointer g_write_string(s7_scheme *sc, s7_pointer args)
{
  #define H_write_string "(write-string str port start end) writes str to port."
  #define Q_write_string s7_make_circular_signature(sc, 3, 4, \
                           sc->is_string_symbol, sc->is_string_symbol, \
                           s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol),\
                           sc->is_integer_symbol)
  const s7_pointer str = s7_car(args);
  s7_pointer port;
  s7_int start = 0, end;
  if (!s7_is_string(str))
    return s7i_method_or_bust(sc, str, "write-string", args, "a string", 1);
  end = s7_string_length(str);
  if (!s7_is_null(sc, s7_cdr(args)))
    {
      s7_pointer inds = s7_cddr(args);
      port = s7_cadr(args);
      if (!s7_is_null(sc, inds))
        {
          s7_pointer p = s7i_start_and_end(sc, s7_make_symbol(sc, "write-string"), args, 3, inds, &start, &end);
          if (!s7i_is_unused(sc, p)) return p;
        }}
  else port = s7_current_output_port(sc);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc))
        {
          s7_int len;
          if ((start == 0) && (end == s7_string_length(str)))
            return str;
          len = (s7_int)(end - start);
          return s7_make_string_with_length(sc, (const char *)(s7_string(str) + start), len);
        }
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "write-string", args);
      return s7_wrong_type_arg_error(sc, "write-string", 2, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "write-string", 2, port, "an open output port");
  if (start == end) return str;
  s7i_port_write_string(sc, (const char *)(s7_string(str) + start), (end - start), port);
  return str;
}

s7_pointer write_string_p_pp(s7_scheme *sc, s7_pointer str, s7_pointer port)
{
  if (!s7_is_string(str))
    return s7i_method_or_bust_pp(sc, str, "write-string", str, port, "a string", 1);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return str;
      return s7i_method_or_bust_pp(sc, port, "write-string", str, port, "an output port", 2);
    }
  if (s7_string_length(str) > 0)
    s7i_port_write_string(sc, s7_string(str), s7_string_length(str), port);
  return str;
}


/* -------------------------------- write-byte -------------------------------- */

s7_pointer g_write_byte(s7_scheme *sc, s7_pointer args)
{
  #define H_write_byte "(write-byte byte (port (current-output-port))): writes byte to the output port"
  #define Q_write_byte s7_make_signature(sc, 3, sc->is_byte_symbol, sc->is_byte_symbol, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))

  s7_pointer port;
  const s7_pointer b = s7_car(args);
  s7_int val;
  if (!s7_is_integer(b))
    return s7i_method_or_bust(sc, b, "write-byte", args, "an integer", 1);

  val = s7_integer(b);
  if ((val < 0) || (val > 255))
    return s7_wrong_type_arg_error(sc, "write-byte", 1, b, "an unsigned byte");

  port = (s7_is_pair(s7_cdr(args))) ? s7_cadr(args) : s7_current_output_port(sc);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return b;
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "write-byte", args);
      return s7_wrong_type_arg_error(sc, "write-byte", 2, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "write-byte", 2, port, "an open output port");

  s7i_port_write_character(sc, (uint8_t)val, port);
  return b;
}
