
/******************************************************************************
 * MODULE     : gs_utilities.mm
 * DESCRIPTION: Utilities for Ghostscript
 * COPYRIGHT  : (C) 2010-2012 David Michel, Joris van der Hoeven, Denis Raux
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "gs_utilities.hpp"
#include "Pdf/pdf.hpp"
#include "analyze.hpp"
#include "file.hpp"
#include "image_files.hpp"
#include "lolly/system/subprocess.hpp"
#include "scheme.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"

// Quote a path for use in Ghostscript commands on macOS/Linux.
// Wraps in double quotes (to preserve spaces in GS option values like
// -sOutputFile=) and escapes $, backticks, ", and \ so that wordexp
// does not perform variable/command expansion on the result.
static string
gs_path_quote (string s) {
  string r= "\"";
  int    n= N (s);
  for (int i= 0; i < n; i++) {
    switch (s[i]) {
    case '$':
    case '`':
    case '"':
    case '\\':
      r << '\\' << s[i];
      break;
    default:
      r << s[i];
    }
  }
  r << "\"";
  return r;
}

static string
gs_executable () {
  eval ("(use-modules (binary gs))");
  url gs_url= as_url (eval ("(find-binary-gs)"));
  if (!is_none (gs_url)) {
    return as_string (gs_url);
  }
  else {
    return "gs";
  }
}

string
gs_prefix () {
  if (os_win ()) {
    return raw_quote (gs_executable ()) * string (" ");
  }
  else {
    return gs_executable () * string (" ");
  }
}

static double
gs_version () {
  static double version= 0;
  if (version == 0) {
    string cmd= gs_prefix () * " --version";
    string buf= var_eval_system (cmd);
    int    pos= 0;
    if (read_double (buf, pos, version)) {
      if (DEBUG_CONVERT) debug_convert << "gs version : " << buf << LF;
    }
    else convert_error << "Cannot determine gs version" << LF;
  }
  return version;
}

// eps2write available starting with gs  9.14 (2014-03-26)
// epswrite removed in gs 9.16 (2015-03-30)
string
eps_device () {
  static string dev; // no need to resolve each time
  if (dev == "") {
    string cmd= gs_prefix () * " --version";
    string buf= var_eval_system (cmd);
    double ver;
    int    pos= 0;
    if (read_double (buf, pos, ver)) {
      if (DEBUG_CONVERT) debug_convert << "gs version :" << buf << LF;
      if (ver >= 9.14) dev= "eps2write";
      else dev= "epswrite";
    }
    else convert_error << "Cannot determine gs version" << LF;
  }
  return copy (dev);
}

bool
gs_supports (url image) {
  string s= suffix (image);
  if (s == "ps" || s == "eps" || s == "pdf") return true;
  return false;
}

void
gs_image_size (url image, int& w_pt, int& h_pt) {
  bool ok;
  if (suffix (image) == "pdf") ok= gs_PDFimage_size (image, w_pt, h_pt);
  else {
    if (DEBUG_CONVERT) debug_convert << "gs eps image size :" << LF;
    int    x1, y1, x2, y2;
    string buf= ps_load (image, false);
    ok        = (N (buf) > 0);
    if (ok) {
      // try finding Bounding box in file:
      ok= ps_read_bbox (buf, x1, y1, x2, y2);
      if (!ok) {
        // bbox not found ask gs to compute one :
        string cmd= gs_prefix ();
        cmd << "-dQUIET -dNOPAUSE -dBATCH -dSAFER -sDEVICE=bbox ";
        // Note: bbox device does a "smart" job of finding the cropbox on its
        // own removing blank margins (*even* on eps files) this is ok if we are
        // reading a ps page
        //  real eps pages with proper bounding boxes have been recognized
        //  before this and will have their BoundingBox respected
        cmd << gs_path_quote (concretize (image));
        lolly::system::check_stderr (cmd, buf);
        if (DEBUG_CONVERT)
          debug_convert << "gs cmd :" << cmd << LF << "answer :" << buf;
        ok= ps_read_bbox (buf, x1, y1, x2, y2);
      }
      if (ok) {
        w_pt= x2 - x1;
        h_pt= y2 - y1;
        set_imgbox_cache (as_tree (image), w_pt, h_pt, x1, y1);
      }
    }
  }
  if (!ok) {
    convert_error << "Cannot read image file '" << image << "'"
                  << " in gs_image_size" << LF;
    w_pt= 0;
    h_pt= 0;
  }
}

void
gs_fix_bbox (url eps, int x1, int y1, int x2, int y2) {
  // used to restore appropriate bounding box of an eps file in case epswrite
  // spuriously changes it (see gs_to_eps)
  string outbuf, buf;
  int    inx1, iny1, inx2, iny2;
  bool   err= load_string (eps, buf, false);
  if (!err) {
    if (DEBUG_CONVERT) debug_convert << "fix_bbox input bbox : ";
    if (!ps_read_bbox (buf, inx1, iny1, inx2, iny2))
      return; // bbox not found... should not occur
    if (inx1 != x1 || iny1 != y1 || inx2 != x2 || iny2 != y2) {
      int pos= search_forwards ("%%BoundingBox:", buf);
      pos+= 14;
      outbuf << buf (0, pos) << " " << as_string (x1) << " " << as_string (y1)
             << " " << as_string (x2) << " " << as_string (y2) << "\n";
      skip_line (buf, pos);
      if (read (buf, pos, "%%HiResBoundingBox:")) skip_line (buf, pos);
      outbuf << buf (pos, N (buf));
      save_string (eps, outbuf, true);
      if (DEBUG_CONVERT)
        debug_convert << "restored bbox : "
                      << ps_read_bbox (outbuf, x1, y1, x2, y2) << LF;
    }
    set_imgbox_cache (as_tree (eps), x2 - x1, y2 - y1, x1, y1);
  }
}

bool
gs_PDFimage_size (url image, int& w_pt, int& h_pt) {
  if (DEBUG_CONVERT) debug_convert << "gs PDF image size :" << LF;
  string buf;
  string cmd= gs_prefix ();
  if (gs_version () >= 9.50)
    cmd << "--permit-file-read=" << gs_path_quote (concretize (image)) << " ";
  cmd << "-dNODISPLAY -q -sFile=";
  cmd << gs_path_quote (concretize (image));
  cmd << " pdf_info.ps";
  buf= eval_system (cmd);
  if (occurs ("Unrecoverable error", buf)) {
    cmd= gs_prefix ();
    if (gs_version () >= 9.50)
      cmd << "--permit-file-read=" << gs_path_quote (concretize (image)) << " ";
    cmd << "-dNODISPLAY -q -sFile=";
    cmd << gs_path_quote (concretize (image));
    cmd << " "
        << gs_path_quote (
               concretize (url ("$TEXMACS_PATH/misc/convert/pdf_info.ps")));
    buf= eval_system (cmd);
  }
  if (DEBUG_CONVERT)
    debug_convert << "gs cmd :" << cmd << LF << "answer :" << buf;
  // if CropBox is defined, then use it, else Mediabox
  string type= "CropBox";
  int    pos = search_forwards ("CropBox: [", buf);
  if (pos < 0) {
    type= "MediaBox";
    pos = search_forwards ("MediaBox: [", buf);
    if (pos < 0) {
      if (DEBUG_CONVERT) debug_convert << "CropBox|MediaBox not found" << LF;
      return false;
    }
  }
  bool   ok= read (buf, pos, type * ": [");
  double X1, Y1, X2, Y2;
  int    x1, y1, x2, y2;
  skip_spaces (buf, pos);
  ok= read_double (buf, pos, X1) && ok;
  x1= (int) floor (X1);
  skip_spaces (buf, pos);
  ok= read_double (buf, pos, Y1) && ok;
  y1= (int) floor (Y1);
  skip_spaces (buf, pos);
  ok= read_double (buf, pos, X2) && ok;
  x2= (int) ceil (X2);
  skip_spaces (buf, pos);
  ok= read_double (buf, pos, Y2) && ok;
  y2= (int) ceil (Y2);
  if (!ok) {
    if (DEBUG_CONVERT) debug_convert << "box dims not found" << LF;
    return false;
  }
  w_pt= x2 - x1;
  h_pt= y2 - y1;
  pos = search_forwards ("Rotate =", buf);
  ok  = read (buf, pos, "Rotate =");
  int rot;
  if (ok) {
    skip_spaces (buf, pos);
    ok= read_int (buf, pos, rot);
    if (ok) {
      rot= rot % 360;
      if (rot < 0) rot+= 360;
      if ((rot % 180) == 90) { // the image is rotated : swap axes lengths
        if (DEBUG_CONVERT) debug_convert << "Rotate =" << rot << LF;
        h_pt= x2 - x1;
        w_pt= y2 - y1;
      }
    }
    else {
      if (DEBUG_CONVERT) debug_convert << "Rotate not found" << LF;
      return false;
    }
  }
  if (DEBUG_CONVERT)
    debug_convert << type << " size =" << w_pt << " x " << h_pt << LF;
  return true;
}

void
gs_to_eps (url image,
           url eps) { // this should be used mostly for pdf->eps conversion.
  string cmd;
  int    bx1, by1, bx2, by2; // bounding box
  if (DEBUG_CONVERT) debug_convert << "gs_to_eps" << LF;
  cmd= gs_prefix ();
  cmd << "-dQUIET -dNOPAUSE -dBATCH -dSAFER ";
  cmd << "-sDEVICE=" << eps_device ();
  cmd << " -sOutputFile=" << gs_path_quote (concretize (eps)) << " ";
  if (suffix (image) == "pdf") {
    image_size (image, bx2, by2);
    bx1= by1= 0;
    cmd << "-dUseCropBox "
        << " -dDEVICEWIDTHPOINTS=" << as_string (bx2)
        << " -dDEVICEHEIGHTPOINTS=" << as_string (by2) << " "
        << gs_path_quote (concretize (image));
  }
  else {
    ps_bounding_box (image, bx1, by1, bx2, by2);
    cmd << " -dDEVICEWIDTHPOINTS=" << as_string (bx2 - bx1)
        << " -dDEVICEHEIGHTPOINTS=" << as_string (by2 - by1) << " ";
    // don't use -dEPSCrop which works incorrectly if (bx1 != 0 || by1 != 0)
    cmd << "-c \" " << as_string (-bx1) << " " << as_string (-by1)
        << " translate gsave \" " << gs_path_quote (concretize (image))
        << " -c \" grestore \"";
  }
  string ans= eval_system (cmd);
  if (DEBUG_CONVERT)
    debug_convert << cmd << LF << "answer :" << ans << LF << "eps generated? "
                  << exists (eps) << LF;
  // eps(2)write and bbox devices do a "smart" job of finding the boundingbox on
  // their own, possibly changing the original margins/aspect ratio defined by
  // the pdf CropBox|MediaBox here were restore the original size.
  gs_fix_bbox (eps, 0, 0, bx2 - bx1, by2 - by1);
}

void
gs_to_ps (url doc, url ps, bool landscape, double paper_h, double paper_w) {
  if (DEBUG_CONVERT) debug_convert << "gs_to_ps" << LF;
  string cmd= gs_prefix ();
  cmd << "-dQUIET -dNOPAUSE -dBATCH -dSAFER -sDEVICE=ps2write ";
  if (landscape)
    cmd << "-dDEVICEWIDTHPOINTS=" << as_string ((int) (28.36 * paper_h + 0.5))
        << " -dDEVICEHEIGHTPOINTS="
        << as_string ((int) (28.36 * paper_w + 0.5));
  else
    cmd << "-dDEVICEWIDTHPOINTS=" << as_string ((int) (28.36 * paper_w + 0.5))
        << " -dDEVICEHEIGHTPOINTS="
        << as_string ((int) (28.36 * paper_h + 0.5));

  cmd << " -sOutputFile=" << gs_path_quote (concretize (ps)) << " ";
  cmd << gs_path_quote (concretize (doc));
  cmd << " -c \"[ /Title (" << as_string (tail (ps)) << ") /DOCINFO pdfmark\" ";

  // NOTE: when converting from pdf to ps the title of the document is
  // incorrectly referring to the name of the temporary file
  // so we add some PS code to override the PS document title with
  // the name of the PS file.

  lolly::system::call (cmd);
  if (DEBUG_CONVERT)
    debug_convert << cmd << LF << "ps generated? " << exists (ps) << LF;
}

void
tm_gs (url image) {
  string cmd= gs_prefix ();
  cmd << "-q -sDEVICE=x11alpha -dBATCH -dNOPAUSE -dSAFER -dNOEPS ";
  cmd << gs_path_quote (concretize (image));
  lolly::system::call (cmd);
}
