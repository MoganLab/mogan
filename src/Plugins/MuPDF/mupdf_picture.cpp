
/******************************************************************************
 * MODULE     : mupdf_picture.cpp
 * DESCRIPTION: Picture objects for MuPDF
 * COPYRIGHT  : (C) 2022 Massimiliano Gubinelli, Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "mupdf_picture.hpp"

#include "editor.hpp"
#include "effect.hpp"
#include "file.hpp"
#include "image_files.hpp"
#include "new_view.hpp"
#include "qt_utilities.hpp"
#include "tm_debug.hpp"
#include "tm_url.hpp"
#include <QImage>

static const double MUPDF_PDF_SCALE= 4.0;

/******************************************************************************
 * Abstract mupdf pictures
 ******************************************************************************/

mupdf_picture_rep::mupdf_picture_rep (fz_pixmap* _pix, int ox2, int oy2)
    : pix (_pix), im (NULL), w (fz_pixmap_width (mupdf_context (), pix)),
      h (fz_pixmap_height (mupdf_context (), pix)), ox (ox2), oy (oy2) {
  fz_keep_pixmap (mupdf_context (), pix);
}

mupdf_picture_rep::~mupdf_picture_rep () {
  fz_drop_pixmap (mupdf_context (), pix);
  fz_drop_image (mupdf_context (), im);
}

picture_kind
mupdf_picture_rep::get_type () {
  return picture_native;
}
void*
mupdf_picture_rep::get_handle () {
  return (void*) this;
}

int
mupdf_picture_rep::get_width () {
  return w;
}
int
mupdf_picture_rep::get_height () {
  return h;
}
int
mupdf_picture_rep::get_origin_x () {
  return ox;
}
int
mupdf_picture_rep::get_origin_y () {
  return oy;
}
void
mupdf_picture_rep::set_origin (int ox2, int oy2) {
  ox= ox2;
  oy= oy2;
}

/**
 * @brief 读取指定像素颜色（内部坐标系）。
 * @param x 像素横坐标，范围为 [0, w)。
 * @param y 像素纵坐标，范围为 [0, h)。
 * @return ARGB 颜色值；若坐标越界或通道数非法，则返回 0。
 */
color
mupdf_picture_rep::internal_get_pixel (int x, int y) {
  // 防御性检查：避免空指针或越界坐标导致非法访问。
  if (pix == NULL) return 0;
  if (x < 0 || y < 0 || x >= w || y >= h) return 0;

  fz_context*    ctx    = mupdf_context ();
  unsigned char* samples= fz_pixmap_samples (ctx, pix);
  int            n      = pix->n;
  bool           alpha  = pix->alpha;
  int            stride = pix->stride;
  if (samples == NULL || stride <= 0) return 0;
  if (n <= 0) {
    static bool warned_n_le0_get= false;
    if (!warned_n_le0_get) {
      std_error << "mupdf_picture_rep::internal_get_pixel: invalid channel "
                << "count n=" << n << LF;
      warned_n_le0_get= true;
    }
    return 0;
  }
  if (n > 4) {
    static bool warned_n_gt4_get= false;
    if (!warned_n_gt4_get) {
      std_error << "mupdf_picture_rep::internal_get_pixel: unsupported "
                << "channel count n=" << n << LF;
      warned_n_gt4_get= true;
    }
    return 0;
  }
  int            row= h - 1 - y;
  unsigned char* p  = samples + row * stride + x * n;

  int r= 0, g= 0, b= 0, a= 255;
  if (n == 1) {
    r= g= b= p[0];
  }
  else if (n == 2) {
    r= g= b= p[0];
    a      = p[1];
  }
  else {
    r= p[0];
    g= p[1];
    b= p[2];
    if (alpha) a= p[n - 1];
  }

  if (a == 255 || !alpha) return rgb_color (r, g, b, 255);
  return rgbap_to_argb ((a << 24) + (b << 16) + (g << 8) + r);
}

/**
 * @brief 写入指定像素颜色（内部坐标系）。
 * @param x 像素横坐标，范围为 [0, w)。
 * @param y 像素纵坐标，范围为 [0, h)。
 * @param c 待写入颜色（ARGB）。
 */
void
mupdf_picture_rep::internal_set_pixel (int x, int y, color c) {
  // 防御性检查：避免空指针或越界坐标导致非法写入。
  if (pix == NULL) return;
  if (x < 0 || y < 0 || x >= w || y >= h) return;

  fz_context*    ctx    = mupdf_context ();
  unsigned char* samples= fz_pixmap_samples (ctx, pix);
  int            n      = pix->n;
  bool           alpha  = pix->alpha;
  int            stride = pix->stride;
  if (samples == NULL || stride <= 0) return;
  if (n <= 0) {
    static bool warned_n_le0_set= false;
    if (!warned_n_le0_set) {
      std_error << "mupdf_picture_rep::internal_set_pixel: invalid channel "
                << "count n=" << n << LF;
      warned_n_le0_set= true;
    }
    return;
  }
  if (n > 4) {
    static bool warned_n_gt4_set= false;
    if (!warned_n_gt4_set) {
      std_error << "mupdf_picture_rep::internal_set_pixel: unsupported "
                << "channel count n=" << n << LF;
      warned_n_gt4_set= true;
    }
    return;
  }
  int            row= h - 1 - y;
  unsigned char* p  = samples + row * stride + x * n;

  int r, g, b, a;
  get_rgb_color (c, r, g, b, a);

  if (alpha) {
    r= (r * a) / 255;
    g= (g * a) / 255;
    b= (b * a) / 255;
  }

  if (n == 1) {
    p[0]= (unsigned char) ((77 * r + 150 * g + 29 * b) >> 8);
    return;
  }

  if (n == 2) {
    p[0]= (unsigned char) ((77 * r + 150 * g + 29 * b) >> 8);
    p[1]= (unsigned char) a;
    return;
  }

  p[0]= (unsigned char) r;
  p[1]= (unsigned char) g;
  p[2]= (unsigned char) b;
  if (alpha) p[n - 1]= (unsigned char) a;
}

picture
mupdf_picture (fz_pixmap* _pix, int ox, int oy) {
  return (picture) tm_new<mupdf_picture_rep, fz_pixmap*, int, int> (_pix, ox,
                                                                    oy);
}

picture
as_mupdf_picture (picture pic) {
  if (pic->get_type () == picture_native) return pic;
  fz_pixmap* pix=
      fz_new_pixmap (mupdf_context (), fz_device_rgb (mupdf_context ()),
                     pic->get_width (), pic->get_height (), NULL, 1);
  picture ret= mupdf_picture (pix, pic->get_origin_x (), pic->get_origin_y ());
  fz_drop_pixmap (mupdf_context (), pix);
  ret->copy_from (pic); // FIXME: is this inefficient???
  return ret;
}

#ifdef USE_MUPDF_RENDERER
picture
as_native_picture (picture pict) {
  return as_mupdf_picture (pict);
}

picture
native_picture (int w, int h, int ox, int oy) {
  fz_pixmap* pix= fz_new_pixmap (
      mupdf_context (), fz_device_rgb (mupdf_context ()), w, h, NULL, 1);
  fz_clear_pixmap_with_value (mupdf_context (), pix, 255); // white background
  picture p= mupdf_picture (pix, ox, oy);
  fz_drop_pixmap (mupdf_context (), pix);
  return p;
}
#endif

/******************************************************************************
 * Rendering on images
 ******************************************************************************/

class mupdf_image_renderer_rep : public mupdf_renderer_rep {
public:
  picture pict;

public:
  mupdf_image_renderer_rep (picture pict, double zoom);
  void* get_data_handle ();
};

mupdf_image_renderer_rep::mupdf_image_renderer_rep (picture p, double zoom)
    : mupdf_renderer_rep (), pict (p) {
  zoomf  = zoom;
  shrinkf= (int) tm_round (std_shrinkf / zoomf);
  pixel  = (SI) tm_round ((std_shrinkf * PIXEL) / zoomf);
  thicken= (shrinkf >> 1) * PIXEL;

  int pw = p->get_width ();
  int ph = p->get_height ();
  int pox= p->get_origin_x ();
  int poy= p->get_origin_y ();

  ox= pox * pixel;
  oy= poy * pixel;
  /*
  cx1= 0;
  cy1= 0;
  cx2= pw * pixel;
  cy2= ph * pixel;
  */
  cx1= 0;
  cy1= -ph * pixel;
  cx2= pw * pixel;
  cy2= 0;

  mupdf_picture_rep* handle= (mupdf_picture_rep*) pict->get_handle ();
  begin (handle->pix);
}

void*
mupdf_image_renderer_rep::get_data_handle () {
  return (void*) this;
}

#ifdef USE_MUPDF_RENDERER
renderer
picture_renderer (picture p, double zoomf) {
  return (renderer) tm_new<mupdf_image_renderer_rep> (p, zoomf);
}
#endif

/******************************************************************************
 * Loading pictures
 ******************************************************************************/

fz_pixmap*
mupdf_load_pdf_image (fz_buffer* pdf_data, fz_matrix scale) {
  fz_context*   ctx   = mupdf_context ();
  pdf_document* doc   = NULL;
  pdf_page*     page  = NULL;
  fz_pixmap*    pix   = NULL;
  fz_stream*    stream= NULL;
  fz_try (ctx) {
    stream        = fz_open_buffer (ctx, pdf_data);
    doc           = pdf_open_document_with_stream (ctx, stream);
    int page_count= pdf_count_pages (ctx, doc);
    if (page_count > 0) {
      page= pdf_load_page (ctx, doc, 0);
      pix = pdf_new_pixmap_from_page_with_usage (
          ctx, page, scale, fz_device_rgb (ctx), 0, "View", FZ_TRIM_BOX);
    }
  }
  fz_catch (ctx) { fz_report_error (ctx); }
  pdf_drop_page (ctx, page);
  pdf_drop_document (ctx, doc);
  fz_drop_stream (ctx, stream);
  return pix;
}

fz_image*
mupdf_load_image (url u) {
  fz_context* ctx   = mupdf_context ();
  fz_image*   im    = NULL;
  fz_buffer*  buffer= mupdf_read_from_url (u);
  if (buffer == NULL) {
    return NULL;
  }
  string    suf= suffix (u);
  fz_matrix ctm= fz_scale (MUPDF_PDF_SCALE, MUPDF_PDF_SCALE);
  if (suf == "svg") {
    fz_xml_doc* xml_doc= NULL;
    fz_xml*     xml    = NULL;
    fz_image*   tmp_im = NULL;
    fz_pixmap*  tmp_pix= NULL;
    fz_try (ctx) {
      xml_doc= fz_parse_xml (ctx, buffer, 0);
      xml    = fz_xml_find (xml_doc, "svg");
      tmp_im = fz_new_image_from_svg_xml (ctx, xml_doc, xml, NULL, NULL);
      tmp_pix= fz_get_pixmap_from_image (ctx, tmp_im, NULL, &ctm, NULL, NULL);
      im     = fz_new_image_from_pixmap (ctx, tmp_pix, NULL);
    }
    fz_catch (ctx) { fz_report_error (ctx); }
    fz_drop_xml (ctx, xml);
    fz_drop_xml (ctx, xml_doc);
    fz_drop_image (ctx, tmp_im);
    fz_drop_pixmap (ctx, tmp_pix);
  }
  else if (suf == "xpm") {
    // try to load higher definition png equivalent if available
    url png_equiv= glue (unglue (u, 4), "_x4.png");
    if (exists (png_equiv)) {
      fz_drop_buffer (ctx, buffer);
      return mupdf_load_image (png_equiv);
    }
    png_equiv= glue (unglue (u, 4), "_x2.png");
    if (exists (png_equiv)) {
      fz_drop_buffer (ctx, buffer);
      return mupdf_load_image (png_equiv);
    }
    png_equiv= glue (unglue (u, 4), ".png");
    if (exists (png_equiv)) {
      fz_drop_buffer (ctx, buffer);
      return mupdf_load_image (png_equiv);
    }
    // ok, try to load the xpm finally
    picture    xp = as_mupdf_picture (load_xpm (u));
    fz_pixmap* pix= ((mupdf_picture_rep*) xp->get_handle ())->pix;
    im            = fz_new_image_from_pixmap (ctx, pix, NULL);
  }
  else if (suf == "pdf") {
    fz_pixmap* pix= mupdf_load_pdf_image (buffer, ctm);
    if (pix != NULL) {
      im= fz_new_image_from_pixmap (mupdf_context (), pix, NULL);
      fz_drop_pixmap (mupdf_context (), pix);
    }
  }
  else if (suf == "webp") {
    // Use Qt to load WebP images since MuPDF doesn't support WebP
    QImage qimage (utf8_to_qstring (concretize (u)));
    if (!qimage.isNull ()) {
      // prefer Format_RGBA8888 when available (Qt 5.5+): memory layout is
      // R,G,B,A
      int          width         = qimage.width ();
      int          height        = qimage.height ();
      fz_pixmap*   pix           = NULL;
      const uchar* src_data      = NULL;
      int          bytes_per_line= 0;

      // 使用 Qt 的预乘 (premultiplied) RGBA 格式，既正确处理
      // alpha，又能提高性能。 Qt 6.8 支持
      // QImage::Format_RGBA8888_Premultiplied，可直接按行 memcpy。
      QImage img=
          qimage.convertToFormat (QImage::Format_RGBA8888_Premultiplied);
      src_data      = img.constBits ();
      bytes_per_line= img.bytesPerLine ();
      pix= fz_new_pixmap (ctx, fz_device_rgb (ctx), width, height, NULL, 1);
      unsigned char* dst_data= fz_pixmap_samples (ctx, pix);

      for (int y= 0; y < height; ++y) {
        const uchar*   src_row= src_data + y * bytes_per_line;
        unsigned char* dst_row= dst_data + y * width * 4;
        memcpy (dst_row, src_row, width * 4);
      }

      im= fz_new_image_from_pixmap (ctx, pix, NULL);
      fz_drop_pixmap (ctx, pix);

      if (DEBUG_CONVERT)
        debug_convert << "Successfully loaded WebP image via Qt: " << width
                      << " x " << height << LF;
    }
    else {
      if (DEBUG_CONVERT)
        debug_convert << "Failed to load WebP image via Qt" << LF;
    }
  }
  else {
    // Othre format.
    fz_try (ctx) { im= fz_new_image_from_buffer (ctx, buffer); }
    fz_catch (ctx) { fz_report_error (ctx); }
  }
  fz_drop_buffer (ctx, buffer);
  if (im == NULL) {
    // attempt to convert to png
    url temp= url_temp (".png");
    image_to_png (u, temp, 0, 0);
    if (exists (temp)) {
      c_string path (as_string (temp));
      fz_try (ctx) { im= fz_new_image_from_file (ctx, path); }
      fz_catch (ctx) { fz_report_error (ctx); }
      remove (temp);
    }
  }
  return im;
}

fz_pixmap*
mupdf_load_pixmap (url u, int w, int h, tree eff, SI pixel) {
  fz_context* ctx= mupdf_context ();
  fz_image*   im = mupdf_load_image (u);

  // Error Handling
  if (im == NULL) {
    return NULL;
  }

  fz_pixmap* pix= fz_get_pixmap_from_image (ctx, im, NULL, NULL, NULL, NULL);
  // Scaling
  if (im->w != w || im->h != h) {
    fz_pixmap* scaled= fz_scale_pixmap (ctx, pix, 0, 0, w, h, NULL);
    fz_drop_pixmap (ctx, pix);
    pix= scaled;
  }
  fz_drop_image (ctx, im); // we do not need it anymore

  // Build effect
  if (eff != "") {
    effect         e  = build_effect (eff);
    picture        src= mupdf_picture (pix, 0, 0);
    array<picture> a;
    a << src;
    picture            pic = e->apply (a, pixel);
    picture            dest= as_mupdf_picture (pic);
    mupdf_picture_rep* rep = (mupdf_picture_rep*) dest->get_handle ();
    fz_pixmap*         tpix= rep->pix;
    fz_drop_pixmap (ctx, pix);
    pix= tpix;
  }
  return pix;
}

/******************************************************************************
 * Image size
 ******************************************************************************/

// Return size in cm
inline double
get_real_size_from_dpi (int px, int dpi) {
  double res= (double) px / dpi;            // to inch
  res       = res * 2.54;                   // to cm
  res= ((int) (res * 100.0 + 0.5)) / 100.0; // round to two decimal places
  return res;
}

static void
format_picsize_string (index_type px, index_type dpi, int& w, int& h,
                       string* out_wcm_pointer, string* out_hcm_pointer) {
  SI     cm  = get_current_editor ()->as_length ("1cm");
  SI     pt  = get_current_editor ()->as_length ("1pt");
  SI     par = get_current_editor ()->as_length ("1par");
  double dwcm= get_real_size_from_dpi (px.x1, dpi.x1);
  double dhcm= get_real_size_from_dpi (px.x2, dpi.x2);
  w          = dwcm * cm / pt;
  h          = dhcm * cm / pt;

  bool is_exceed_page= w * pt > par;
  if (out_wcm_pointer != NULL) {
    *out_wcm_pointer= is_exceed_page ? "0.8par" : as_string (dwcm) * "cm";
  }
  if (out_hcm_pointer != NULL) {
    *out_hcm_pointer= is_exceed_page ? as_string (0.8 / w * h) * "par"
                                     : as_string (dhcm) * "cm";
  }
}

bool
mupdf_supports (string extension) {
  if (fz_lookup_image_type (c_string (extension)) != FZ_IMAGE_UNKNOWN) {
    return true;
  }
  if (extension == "pdf" || extension == "jpg" || extension == "tif" ||
      extension == "svg" || extension == "webp") {
    return true;
  }
  return false;
}

bool
mupdf_normal_image_size (url image, int& w, int& h, string* out_wcm_pointer,
                         string* out_hcm_pointer) {
  if (DEBUG_CONVERT) debug_convert << "mupdf_normal_image_size :" << LF;
  fz_context* ctx= mupdf_context ();
  fz_image*   im = NULL;
  fz_buffer*  buf= mupdf_read_from_url (image);
  if (buf != NULL) {
    fz_try (ctx) { im= fz_new_image_from_buffer (ctx, buf); }
    fz_catch (ctx) fz_report_error (ctx);
    fz_drop_buffer (ctx, buf);
  }
  if (im == NULL) {
    // Check if it's a WebP file and use Qt to get the size
    string suf= suffix (image);
    if (suf == "webp") {
      QImage qimage (utf8_to_qstring (concretize (image)));
      if (!qimage.isNull ()) {
        index_type px (qimage.width (), qimage.height ());
        index_type dpi (72, 72); // Default DPI for WebP
        format_picsize_string (px, dpi, w, h, out_wcm_pointer, out_hcm_pointer);
        if (DEBUG_CONVERT)
          debug_convert << "mupdf_normal_image_size (WebP via Qt): " << w
                        << " x " << h << LF;
        return true;
      }
      else {
        if (DEBUG_CONVERT)
          debug_convert << "Failed to get WebP image size via Qt" << LF;
      }
    }

    convert_error << "Cannot read image file '" << image << "'"
                  << " in mupdf_normal_image_size" << LF;
    w= 35;
    h= 35;
    return false;
  }
  else {
    index_type px (im->w, im->h);
    index_type dpi (im->xres, im->yres);
    format_picsize_string (px, dpi, w, h, out_wcm_pointer, out_hcm_pointer);
    fz_drop_image (ctx, im);
    if (DEBUG_CONVERT)
      debug_convert << "mupdf_normal_image_size (pt): " << w << " x " << h
                    << LF;
    return true;
  }
}

bool
mupdf_pdf_image_size (url image, int& w, int& h, string* out_wcm_pointer,
                      string* out_hcm_pointer) {
  /* 计算 PDF 图片尺寸：优先不渲染整张
   * pixmap，而是读取页面边界并基于缩放因子计算像素尺寸。 */
  fz_context* ctx    = mupdf_context ();
  fz_buffer*  buf    = mupdf_read_from_url (image);
  fz_pixmap*  im     = NULL;
  bool        success= false;
  if (buf != NULL) {
    /* 先尝试轻量级路径：读取页面边界并使用与渲染一致的缩放因子计算像素尺寸。
      这样在许多情况下无需渲染整页 pixmap（在 HiDPI
      或大页面上会占用大量内存）。*/
    double        scale_factor= MUPDF_PDF_SCALE;
    fz_stream*    stream      = NULL;
    pdf_document* doc         = NULL;
    pdf_page*     page        = NULL;
    fz_rect       bounds;
    bool          bounds_succeeded= false;

    fz_try (ctx) {
      stream        = fz_open_buffer (ctx, buf);
      doc           = pdf_open_document_with_stream (ctx, stream);
      int page_count= pdf_count_pages (ctx, doc);
      if (page_count > 0) {
        page= pdf_load_page (ctx, doc, 0);
        /* fz_bound_page 返回 PDF 页面在用户坐标系（以点/pt 为单位）中的矩形。
          此处将页面尺寸与渲染使用的缩放因子相乘以得到像素尺寸。 */
        bounds          = fz_bound_page (ctx, (fz_page*) page);
        double w_pts    = bounds.x1 - bounds.x0;
        double h_pts    = bounds.y1 - bounds.y0;
        int    px_w     = (int) tm_round (w_pts * scale_factor);
        int    px_h     = (int) tm_round (h_pts * scale_factor);
        int    dpi_val  = (int) tm_round (72.0 * scale_factor);
        bounds_succeeded= true;
        index_type px (px_w, px_h);
        index_type dpi (dpi_val, dpi_val);
        format_picsize_string (px, dpi, w, h, out_wcm_pointer, out_hcm_pointer);
      }
    }
    fz_catch (ctx) {
      /* 如果解析、读取边界等任一步骤失败，则退回到原来的实现：
         将第一页渲染为 pixmap 并从 pixmap 获取像素尺寸。 */
      fz_report_error (ctx);
      im= mupdf_load_pdf_image (buf, fz_scale (scale_factor, scale_factor));
    }

    pdf_drop_page (ctx, page);
    pdf_drop_document (ctx, doc);
    fz_drop_stream (ctx, stream);
    fz_drop_buffer (ctx, buf);
    /* 如果通过 page bounds 成功计算到像素尺寸，设置 success=true
       但不立刻返回（统一在函数末尾返回），这样保持函数风格一致。 */
    if (bounds_succeeded) {
      success= true;
    }
    /* bounds 未成功时，若 catch 路径渲染得到 pixmap（im != NULL）则
       仍会在下面分支里使用该 pixmap 计算尺寸；否则进入默认错误处理。 */
  }
  if (!success) {
    if (im == NULL) {
      /* 无法通过 bounds 或渲染读取到尺寸，返回默认尺寸并表示失败 */
      convert_error << "Cannot read image file '" << image << "'"
                    << " in mupdf_pdf_image_size" << LF;
      w      = 35;
      h      = 35;
      success= false;
    }
    else {
      /* 渲染路径成功：从 pixmap 获取尺寸 */
      index_type px (im->w, im->h);
      index_type dpi (72, 72);
      format_picsize_string (px, dpi, w, h, out_wcm_pointer, out_hcm_pointer);
      fz_drop_pixmap (ctx, im);
      success= true;
    }
  }

  return success;
}

string
mupdf_load_and_parse_image (const char* path, int& w, int& h, string extension,
                            string* out_wcm_pointer, string* out_hcm_pointer) {
  fz_context*    ctx= mupdf_context ();
  fz_buffer*     buf= NULL;
  unsigned char* data;
  fz_try (ctx) { buf= fz_read_file (ctx, path); }
  fz_catch (ctx) {
    fz_report_error (ctx);
    buf= fz_new_buffer (ctx, 0);
  }
  int    len= fz_buffer_storage (ctx, buf, &data);
  string res (reinterpret_cast<const char*> (data), len);
  fz_drop_buffer (ctx, buf);
  url image= url_ramdisc (res) * extension;
  if (extension == "pdf") {
    mupdf_pdf_image_size (image, w, h, out_wcm_pointer, out_hcm_pointer);
  }
  else if (extension == "eps" || extension == "ps" || extension == "svg") {
    w= 0;
    h= 0;
  }
  else if (extension == "webp") {
    // For WebP files, use Qt to get the image size directly from the file
    QImage qimage (path);
    if (!qimage.isNull ()) {
      index_type px (qimage.width (), qimage.height ());
      index_type dpi (72, 72); // Default DPI for WebP
      format_picsize_string (px, dpi, w, h, out_wcm_pointer, out_hcm_pointer);
      if (DEBUG_CONVERT)
        debug_convert << "mupdf_load_and_parse_image (WebP via Qt): " << w
                      << " x " << h << LF;
    }
    else {
      // Fallback to default size if Qt fails
      w= 35;
      h= 35;
      if (DEBUG_CONVERT)
        debug_convert << "Failed to get WebP image size via Qt, using default"
                      << LF;
    }
  }
  else {
    mupdf_normal_image_size (image, w, h, out_wcm_pointer, out_hcm_pointer);
  }

  return res;
}

void
mupdf_pretty_image_size (url image, string& w, string& h) {
  int    ww, hh;
  string suf= suffix (image);
  if (suf == "pdf") {
    mupdf_pdf_image_size (image, ww, hh, &w, &h);
  }
  else if (suf != "svg") {
    mupdf_normal_image_size (image, ww, hh, &w, &h);
  }
}

#ifdef USE_MUPDF_RENDERER
picture
load_picture (url u, int w, int h, tree eff, int pixel) {
  fz_pixmap* pix= mupdf_load_pixmap (u, w, h, eff, pixel);
  if (pix == NULL) return error_picture (w, h);
  picture p= mupdf_picture (pix, 0, 0);
  fz_drop_pixmap (mupdf_context (), pix);
  return p;
}

void
save_picture (url dest, picture p) {
  if (suffix (dest) != "png") {
    cout << "TeXmacs] warning: cannot save " << concretize (dest)
         << ", format not supported\n";
    return;
  }
  picture            q   = as_mupdf_picture (p);
  mupdf_picture_rep* pict= (mupdf_picture_rep*) q->get_handle ();
  if (exists (dest)) remove (dest);
  c_string   path= concretize (dest);
  fz_output* out = fz_new_output_with_path (mupdf_context (), path, 0);
  fz_write_pixmap_as_png (mupdf_context (), out, pict->pix);
  fz_close_output (mupdf_context (), out);
  fz_drop_output (mupdf_context (), out);
}
#endif
