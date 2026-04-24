
/******************************************************************************
 * MODULE     : lazy_gui.cpp
 * DESCRIPTION: Lazy typesetting of GUI primitives
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Boxes/construct.hpp"
#include "Concat/canvas_properties.hpp"
#include "Format/format.hpp"
#include "Line/lazy_typeset.hpp"
#include "Line/lazy_vstream.hpp"
#include "Stack/stacker.hpp"
#include "analyze.hpp"

using namespace moebius;

box surround (edit_env env, box b, path ip, array<line_item> l,
              array<line_item> r, format fm);

/******************************************************************************
 * Canvases
 ******************************************************************************/

struct lazy_canvas_rep : public lazy_rep {
  canvas_properties props;
  lazy              par;

  lazy_canvas_rep (canvas_properties props2, lazy par2, path ip)
      : lazy_rep (LAZY_CANVAS, ip), props (props2), par (par2) {}
  inline operator tree () { return "Canvas"; }
  lazy   produce (lazy_type request, format fm);
  format query (lazy_type request, format fm);
};

struct lazy_canvas {
  EXTEND_NULL (lazy, lazy_canvas);
  inline lazy_canvas (canvas_properties props, lazy par, path ip)
      : rep (tm_new<lazy_canvas_rep> (props, par, ip)) {
    rep->ref_count= 1;
  }
};
EXTEND_NULL_CODE (lazy, lazy_canvas);

format
lazy_canvas_rep::query (lazy_type request, format fm) {
  if ((request == LAZY_BOX) && (fm->type == QUERY_VSTREAM_WIDTH)) {
    format       body_fm= par->query (request, fm);
    format_width fmw    = (format_width) body_fm;
    SI           width  = fmw->width;
    edit_env     env    = props->env;
    tree         old1   = env->local_begin (PAGE_MEDIUM, "papyrus");
    tree         old2   = env->local_begin (PAR_LEFT, "0tmpt");
    tree         old3   = env->local_begin (PAR_RIGHT, "0tmpt");
    tree         old4   = env->local_begin (PAR_MODE, "justify");
    tree         old5   = env->local_begin (PAR_NO_FIRST, "true");
    tree old6= env->local_begin (PAR_WIDTH, tree (TMLEN, as_string (width)));
    SI   x1, x2, scx;
    get_canvas_horizontal (props, 0, fmw->width, x1, x2, scx);
    env->local_end (PAR_WIDTH, old6);
    env->local_end (PAR_NO_FIRST, old5);
    env->local_end (PAR_MODE, old4);
    env->local_end (PAR_RIGHT, old3);
    env->local_end (PAR_LEFT, old2);
    env->local_end (PAGE_MEDIUM, old1);
    SI     delta= 0;
    string type = props->type;
    if (type != "plain") {
      SI hpad= props->hpadding;
      SI w   = props->bar_width;
      SI pad = props->bar_padding;
      SI bor = props->border;
      if (ends (type, "w") || ends (type, "e")) delta= max (0, w + pad);
      delta+= 2 * bor + 2 * hpad;
    }
    return make_format_width (x2 - x1 + delta);
  }
  return lazy_rep::query (request, fm);
}

lazy
lazy_canvas_rep::produce (lazy_type request, format fm) {
  if (request == type) return this;
  if (request == LAZY_VSTREAM || request == LAZY_BOX) {
    SI     delta= 0;
    string type = props->type;
    if (type != "plain") {
      SI hpad= props->hpadding;
      SI w   = props->bar_width;
      SI pad = props->bar_padding;
      SI bor = props->border;
      if (ends (type, "w") || ends (type, "e")) delta= max (0, w + pad);
      delta+= 2 * bor + 2 * hpad;
    }
    format bfm= fm;
    if (request == LAZY_VSTREAM) {
      format_vstream fvs= (format_vstream) fm;
      bfm               = make_format_width (fvs->width - delta);
    }
    box          b    = (box) par->produce (LAZY_BOX, bfm);
    format_width fmw  = (format_width) bfm;
    SI           width= fmw->width + delta;
    edit_env     env  = props->env;
    tree         old1 = env->local_begin (PAGE_MEDIUM, "papyrus");
    tree         old2 = env->local_begin (PAR_LEFT, "0tmpt");
    tree         old3 = env->local_begin (PAR_RIGHT, "0tmpt");
    tree         old4 = env->local_begin (PAR_MODE, "justify");
    tree         old5 = env->local_begin (PAR_NO_FIRST, "true");
    tree old6= env->local_begin (PAR_WIDTH, tree (TMLEN, as_string (width)));
    SI   x1, x2, scx;
    get_canvas_horizontal (props, b->x1, b->x2, x1, x2, scx);
    SI y1, y2, scy;
    get_canvas_vertical (props, b->y1, b->y2, y1, y2, scy);
    env->local_end (PAR_WIDTH, old6);
    env->local_end (PAR_NO_FIRST, old5);
    env->local_end (PAR_MODE, old4);
    env->local_end (PAR_RIGHT, old3);
    env->local_end (PAR_LEFT, old2);
    env->local_end (PAGE_MEDIUM, old1);
    path dip= (type == "plain" ? ip : decorate (ip));
    box  rb = clip_box (dip, b, x1, y1, x2, y2, props->xt, props->yt, scx, scy);
    if (type != "plain") rb= put_scroll_bars (props, rb, ip, b, scx, scy);
    if (request == LAZY_BOX) return make_lazy_box (rb);
    else {
      array<page_item> l;
      l << page_item (rb);
      return lazy_vstream (ip, "", l, stack_border ());
    }
  }
  return lazy_rep::produce (request, fm);
}

lazy
make_lazy_canvas (edit_env env, tree t, path ip) {
  canvas_properties props= get_canvas_properties (env, t);
  lazy              par  = make_lazy (env, t[6], descend (ip, 6));
  return lazy_canvas (props, par, ip);
}

/******************************************************************************
 * Ornaments
 ******************************************************************************/

struct lazy_ornament_rep : public lazy_rep {
  edit_env            env; // "current" environment
  lazy                par; // the ornamented body
  box                 xb;  // extra box
  ornament_parameters ps;  // parameters for the ornament
  lazy_ornament_rep (edit_env env2, lazy par2, box xb2, path ip,
                     ornament_parameters ps2)
      : lazy_rep (LAZY_ORNAMENT, ip), env (env2), par (par2), xb (xb2),
        ps (ps2) {}
  inline operator tree () { return "Ornament"; }
  lazy   produce (lazy_type request, format fm);
  format query (lazy_type request, format fm);
};

struct lazy_ornament {
  EXTEND_NULL (lazy, lazy_ornament);
  lazy_ornament (edit_env env, lazy par, box xb, path ip,
                 ornament_parameters ps)
      : rep (tm_new<lazy_ornament_rep> (env, par, xb, ip, ps)) {
    rep->ref_count= 1;
  }
};
EXTEND_NULL_CODE (lazy, lazy_ornament);

format
lazy_ornament_rep::query (lazy_type request, format fm) {
  if ((request == LAZY_BOX) && (fm->type == QUERY_VSTREAM_WIDTH)) {
    format       body_fm= par->query (request, fm);
    format_width fmw    = (format_width) body_fm;
    SI           dw     = ps->lpad + ps->rpad;
    return make_format_width (fmw->width + dw);
  }
  return lazy_rep::query (request, fm);
}

/**
 * @brief 生成 ornament 的延迟排版结果。
 *
 * 该函数根据请求类型返回加框内容对应的 box 或 vstream。对于
 * `LAZY_VSTREAM` 路径，除了生成外框 box 之外，还会重新请求正文的
 * vstream，并收集其内部 `page_item` 上附着的 `fl`。这样在 ornament
 * 将正文重新包装成新的外层 `page_item` 时，脚注等页面插入对象不会丢失。
 *
 * @param request 当前请求的延迟对象类型，支持 `LAZY_BOX` 和 `LAZY_VSTREAM`。
 * @param fm 当前排版格式；在 vstream/cell 场景下会用于推导正文可用宽度。
 * @return 生成后的延迟对象；若请求为 `LAZY_BOX` 则返回 box，否则返回携带
 *         附着 floats 的 vstream。
 */
lazy
lazy_ornament_rep::produce (lazy_type request, format fm) {
  if (request == type) return this;
  if (request == LAZY_VSTREAM || request == LAZY_BOX) {
    format bfm            = fm;
    SI     body_width     = 0;
    bool   have_body_width= false;
    if (request == LAZY_VSTREAM) {
      format_vstream fvs= (format_vstream) fm;
      SI             dw = ps->lpad + ps->rpad;
      bfm               = make_format_width (fvs->width - dw);
      body_width        = fvs->width - dw;
      have_body_width   = true;
    }
    else if (fm->type == FORMAT_CELL) {
      format_cell fc = (format_cell) fm;
      SI          dw = ps->lpad + ps->rpad;
      body_width     = fc->width - dw;
      have_body_width= true;
    }
    box         b= (box) par->produce (LAZY_BOX, bfm);
    array<lazy> fl;
    if (have_body_width) {
      lazy body=
          par->produce (LAZY_VSTREAM, make_format_vstream (body_width, 0, 0));
      fl= collect_attached_floats (((lazy_vstream) body)->l);
    }
    box hb= highlight_box (ip, b, xb, ps);
    // FIXME: this dirty hack ensures that shoving is correct
    hb= move_box (decorate (ip), hb, 1, 0);
    hb= move_box (decorate (ip), hb, -1, 0);
    // End dirty hack
    if (fm->type == FORMAT_VSTREAM) {
      format_vstream fs= (format_vstream) fm;
      hb               = surround (env, hb, ip, fs->before, fs->after, bfm);
    }
    if (request == LAZY_BOX) return make_lazy_box (hb);
    else {
      array<page_item> l;
      l << page_item (hb, fl);
      return lazy_vstream (ip, "", l, stack_border ());
    }
  }
  return lazy_rep::produce (request, fm);
}

lazy
make_lazy_ornament (edit_env env, tree t, path ip) {
  ornament_parameters ps = env->get_ornament_parameters ();
  lazy                par= make_lazy (env, t[0], descend (ip, 0));
  box                 xb;
  if (N (t) == 2) xb= typeset_as_concat (env, t[1], descend (ip, 1));
  return lazy_ornament (env, par, xb, ip, ps);
}

/******************************************************************************
 * Art boxes
 ******************************************************************************/

struct lazy_art_box_rep : public lazy_rep {
  edit_env           env; // "current" environment
  lazy               par; // the ornamented body
  art_box_parameters ps;  // parameters for the art_box
  lazy_art_box_rep (edit_env env2, lazy par2, path ip, art_box_parameters ps2)
      : lazy_rep (LAZY_ART_BOX, ip), env (env2), par (par2), ps (ps2) {}
  inline operator tree () { return "Art_Box"; }
  lazy   produce (lazy_type request, format fm);
  format query (lazy_type request, format fm);
};

struct lazy_art_box {
  EXTEND_NULL (lazy, lazy_art_box);
  lazy_art_box (edit_env env, lazy par, path ip, art_box_parameters ps)
      : rep (tm_new<lazy_art_box_rep> (env, par, ip, ps)) {
    rep->ref_count= 1;
  }
};
EXTEND_NULL_CODE (lazy, lazy_art_box);

format
lazy_art_box_rep::query (lazy_type request, format fm) {
  if ((request == LAZY_BOX) && (fm->type == QUERY_VSTREAM_WIDTH)) {
    format       body_fm= par->query (request, fm);
    format_width fmw    = (format_width) body_fm;
    SI           dw     = ps->lpad + ps->rpad;
    return make_format_width (fmw->width + dw);
  }
  return lazy_rep::query (request, fm);
}

/**
 * @brief 生成 art box 的延迟排版结果。
 *
 * 该函数与 `lazy_ornament_rep::produce` 类似，但外层包装使用 `art_box`。
 * 在 `LAZY_VSTREAM` 路径下，函数会先根据正文宽度重新生成内部 vstream，
 * 收集其中附着的 `fl`，再在构造外层 `page_item` 时一并挂回去，确保脚注、
 * 浮动对象等页面插入语义在 art box 包装后仍然保留。
 *
 * @param request 当前请求的延迟对象类型，支持 `LAZY_BOX` 和 `LAZY_VSTREAM`。
 * @param fm 当前排版格式；在 vstream/cell 场景下会用于推导正文可用宽度。
 * @return 生成后的延迟对象；若请求为 `LAZY_BOX` 则返回 box，否则返回携带
 *         附着 floats 的 vstream。
 */
lazy
lazy_art_box_rep::produce (lazy_type request, format fm) {
  if (request == type) return this;
  if (request == LAZY_VSTREAM || request == LAZY_BOX) {
    format bfm            = fm;
    SI     body_width     = 0;
    bool   have_body_width= false;
    if (request == LAZY_VSTREAM) {
      format_vstream fvs= (format_vstream) fm;
      SI             dw = ps->lpad + ps->rpad;
      bfm               = make_format_width (fvs->width - dw);
      body_width        = fvs->width - dw;
      have_body_width   = true;
    }
    else if (fm->type == FORMAT_CELL) {
      format_cell fc = (format_cell) fm;
      SI          dw = ps->lpad + ps->rpad;
      body_width     = fc->width - dw;
      have_body_width= true;
    }
    box         b= (box) par->produce (LAZY_BOX, bfm);
    array<lazy> fl;
    if (have_body_width) {
      lazy body=
          par->produce (LAZY_VSTREAM, make_format_vstream (body_width, 0, 0));
      fl= collect_attached_floats (((lazy_vstream) body)->l);
    }
    box hb= art_box (ip, b, ps);
    hb    = move_box (decorate (ip), hb, 0, b->y1 - ps->bpad);
    // FIXME: this dirty hack ensures that shoving is correct
    hb= move_box (decorate (ip), hb, 1, 0);
    hb= move_box (decorate (ip), hb, -1, 0);
    // End dirty hack
    if (fm->type == FORMAT_VSTREAM) {
      format_vstream fs= (format_vstream) fm;
      hb               = surround (env, hb, ip, fs->before, fs->after, bfm);
    }
    if (request == LAZY_BOX) return make_lazy_box (hb);
    else {
      array<page_item> l;
      l << page_item (hb, fl);
      return lazy_vstream (ip, "", l, stack_border ());
    }
  }
  return lazy_rep::produce (request, fm);
}

lazy
make_lazy_art_box (edit_env env, tree t, path ip) {
  art_box_parameters ps = env->get_art_box_parameters (t);
  lazy               par= make_lazy (env, t[0], descend (ip, 0));
  return lazy_art_box (env, par, ip, ps);
}
