
/******************************************************************************
 * MODULE     : pdf_image.cpp
 * DESCRIPTION: PDF Image on Hummus
 * COPYRIGHT  : (C) 2012 Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "pdf_image.hpp"
#include "PDFWriter/InputByteArrayStream.h"
#include "PDFWriter/OutputStreamTraits.h"
#include "PDFWriter/PDFDocumentCopyingContext.h"
#include "PDFWriter/PDFFormXObject.h"
#include "PDFWriter/PDFImageXObject.h"
#include "PDFWriter/PDFPageInput.h"
#include "PDFWriter/PDFStream.h"
#include "PDFWriter/ProcsetResourcesConstants.h"
#include "PDFWriter/XObjectContentContext.h"
#include "file.hpp"
#include "scheme.hpp"
#include "tm_debug.hpp"
#include "tm_url.hpp"
#include "url.hpp"

#ifdef QTTEXMACS
#include "Qt/qt_utilities.hpp"
#include <QImage>
#endif

static const std::string scType   = "Type";
static const std::string scXObject= "XObject";
static const std::string scSubType= "Subtype";

static const std::string scImage           = "Image";
static const std::string scWidth           = "Width";
static const std::string scHeight          = "Height";
static const std::string scColorSpace      = "ColorSpace";
static const std::string scDeviceGray      = "DeviceGray";
static const std::string scDeviceRGB       = "DeviceRGB";
static const std::string scDeviceCMYK      = "DeviceCMYK";
static const std::string scDecode          = "Decode";
static const std::string scBitsPerComponent= "BitsPerComponent";
static const std::string scFilter          = "Filter";
static const std::string scDCTDecode       = "DCTDecode";
static const std::string scLength          = "Length";

void pdf_image_info (url image, int& w, int& h, PDFRectangle& cropBox,
                     double (&tMat)[6], PDFPageInput& pageInput);

static void
create_pdf_image_raw (PDFWriter& pdfw, string raw_data, SI width, SI height,
                      ObjectIDType imageXObjectID) {

  PDFImageXObject* imageXObject= NULL;
  // EStatusCode status = PDFHummus::eSuccess;

  ObjectsContext& objectsContext= pdfw.GetObjectsContext ();
  objectsContext.StartNewIndirectObject (imageXObjectID);
  do {
    {
      // write stream dictionary
      DictionaryContext* imageContext= objectsContext.StartDictionary ();
      // type
      imageContext->WriteKey (scType);
      imageContext->WriteNameValue (scXObject);
      // subtype
      imageContext->WriteKey (scSubType);
      imageContext->WriteNameValue (scImage);
      // Width
      imageContext->WriteKey (scWidth);
      imageContext->WriteIntegerValue (width);
      // Height
      imageContext->WriteKey (scHeight);
      imageContext->WriteIntegerValue (height);
      // Bits Per Component
      imageContext->WriteKey (scBitsPerComponent);
      imageContext->WriteIntegerValue (8);
      // Color Space and Decode Array if necessary
      imageContext->WriteKey (scColorSpace);
      imageContext->WriteNameValue (scDeviceGray);
      // Length
      imageContext->WriteKey ("Length");
      imageContext->WriteIntegerValue (N (raw_data));
      objectsContext.EndDictionary (imageContext);
    }
    {
      // write stream
      objectsContext.WriteKeyword ("stream");
      {
        c_string buf (raw_data);
        objectsContext.StartFreeContext ()->Write ((unsigned char*) (char*) buf,
                                                   N (raw_data));
        objectsContext.EndFreeContext ();
      }
      objectsContext.EndLine ();
      objectsContext.WriteKeyword ("endstream");
    }
    objectsContext.EndIndirectObject ();
    imageXObject= new PDFImageXObject (imageXObjectID, KProcsetImageB);
  } while (false);

  if (imageXObject == NULL)
    convert_error << "pdf_hummus, failed to include glyph" << LF;
  else delete imageXObject;
}

#ifdef QTTEXMACS
void
qt_image_data (url image, int& w, int& h, string& data, string& mask) {
  // debug_convert << "in qt_image_data"<<LF;
  QImage im (utf8_to_qstring (concretize (image)));
  if (im.isNull ()) {
    convert_error << "Cannot read image file '" << image << "'"
                  << " in qt_image_data" << LF;
    return;
  }
  w    = im.width ();
  h    = im.height ();
  data = string ((w * h) * 3);
  mask = string (w * h);
  int i= 0, j= 0, k= 0, l= 0;
  for (j= 0; j < im.height (); j++) {
    for (i= 0; i < im.width (); i++) {
      QRgb p   = im.pixel (i, j);
      data[l++]= qRed (p);
      data[l++]= qGreen (p);
      data[l++]= qBlue (p);
      mask[k++]= qAlpha (p);
    }
  }
}
#endif

bool
pdf_image_rep::flush_jpg (PDFWriter& pdfw, url image) {
  c_string         f (concretize (image));
  DocumentContext& documentContext= pdfw.GetDocumentContext ();
  PDFImageXObject* imageXObject=
      documentContext.CreateImageXObjectFromJPGFile ((char*) f);
  if ((void*) imageXObject == NULL) {
    convert_error << "pdf_hummus, failed to include JPG file " << image << LF;
    return false;
  }
  PDFFormXObject* xobjectForm=
      pdfw.StartFormXObject (PDFRectangle (0, 0, w, h), id);
  XObjectContentContext* xobjectContentContext=
      xobjectForm->GetContentContext ();
  xobjectContentContext->q ();
  xobjectContentContext->cm (w, 0, 0, h, 0, 0);
  std::string pdfImageName=
      xobjectForm->GetResourcesDictionary ().AddImageXObjectMapping (
          imageXObject);
  xobjectContentContext->Do (pdfImageName);
  xobjectContentContext->Q ();
  EStatusCode status= pdfw.EndFormXObjectAndRelease (xobjectForm);
  delete imageXObject;
  return status == eSuccess;
}

#ifndef PDFHUMMUS_NO_PNG
bool
pdf_image_rep::flush_png (PDFWriter& pdfw, url image) {
  c_string        f (concretize (image));
  PNGImageHandler pngHandler;
  InputFile       file;
  if (file.OpenFile (std::string ((char*) f)) != PDFHummus::eSuccess) {
    convert_error << "pdf_hummus, failed to include PNG file " << image << LF;
    return false;
  }
  IByteReaderWithPosition* stream= file.GetInputStream ();
  DoubleAndDoublePair      dim   = pngHandler.ReadImageDimensions (stream);
  double                   iw= dim.first, ih= dim.second;

  stream->SetPosition (0);
  PDFFormXObject* formXObject= pdfw.CreateFormXObjectFromPNGStream (stream);
  if ((void*) formXObject == NULL) {
    convert_error << "pdf_hummus, failed to create Form from PNG stream" << LF;
    return false;
  }
  PDFFormXObject* xobjectForm=
      pdfw.StartFormXObject (PDFRectangle (0, 0, w, h), id);
  XObjectContentContext* xobjectContentContext=
      xobjectForm->GetContentContext ();
  xobjectContentContext->q ();
  xobjectContentContext->cm (w / iw, 0, 0, h / ih, 0, 0);
  std::string pdfImageName=
      xobjectForm->GetResourcesDictionary ().AddFormXObjectMapping (
          formXObject->GetObjectID ());
  xobjectContentContext->Do (pdfImageName);
  xobjectContentContext->Q ();
  EStatusCode status= pdfw.EndFormXObjectAndRelease (xobjectForm);
  delete formXObject;
  return status == eSuccess;
}
#endif

static string
judge_picture_format (url image) {
  string data;
  if (is_ramdisc (image)) {
    // 在访问树节点子元素前进行边界检查
    // ramdisc URL 结构：concat(root("ramdisc", data), filename)
    if (N (image->t) < 2 || N (image[1]->t) < 3) {
      data= string ();
    }
    else {
      data= as_string (image[1][2]);
    }
  }
  else {
    load_string (concretize_url (image), data, false);
  }
  if (N (data) > 8) {
    if (data[0] == '\xff' && data[1] == '\xd8') {
      return "jpeg";
    }
    if (!strncmp (data.begin (), "\x89PNG\x0d\x0a\x1a\x0a", 8)) {
      return "png";
    }
    if (!strncmp (data.begin (), "%PDF-", 5)) {
      return "pdf";
    }
    return "unknown";
  }
  return "invalid";
}

void
pdf_image_rep::flush (PDFWriter& pdfw) {
  url name= resolve (u);
  if (is_none (name)) name= "$TEXMACS_PATH/misc/pixmaps/unknown.png";

  url    temp;
  string s= judge_picture_format (u);
  // debug_convert << "flushing :" << name << " type: " << s << LF;
  if (s == "pdf") {
    temp= name;
    name= url_none ();
  }
  else {
    temp= url_temp ("pdf");
    // first try to work out inclusion using our own tools
    // note that we have to return since flush_raster and flush_jpg
    // already build the appopriate Form XObject into the PDF

    if (s == "jpeg")
      if (flush_jpg (pdfw, name)) return;
#ifndef PDFHUMMUS_NO_PNG
    if (s == "png")
      if (flush_png (pdfw, name)) return;
#endif
    // other formats we generate a pdf (with available converters) that we'll
    // embbed
    image_to_pdf (name, temp, w, h, 300);
    // the 300 dpi setting is the maximum dpi of raster images that will be
    // generated: images that are to dense will de downsampled to keep file
    // small (other are not up-sampled) dpi DOES NOT apply for vector images
    // that we know how to handle : eps, svg(if inkscape present)
    //
    // TODO: make the max dpi setting smarter (printer resolution, preference
    // ...)
  }
  EStatusCode      status= PDFHummus::eFailure;
  DocumentContext& dc    = pdfw.GetDocumentContext ();

  char*                      _temp= as_charp (concretize (temp));
  PDFDocumentCopyingContext* copyingContext=
      pdfw.CreatePDFCopyingContext (_temp);
  if (copyingContext) {
    PDFPageInput pageInput (
        copyingContext->GetSourceDocumentParser (),
        copyingContext->GetSourceDocumentParser ()->ParsePage (0));
    EPDFVersion version=
        (EPDFVersion) (int) (copyingContext->GetSourceDocumentParser ()
                                 ->GetPDFLevel () *
                             10);
    // if (version > ePDFVersion)
    //   convert_warning << "\"" << _temp << "\" has version "
    //                   << ((double) version) / 10 << "." << LF
    //                   << "But current PDF version has been set to "
    //                   << ((double) ePDFVersion) / 10
    //                   << " (see the preference menu)." << LF;
    double       tMat[6]= {1, 0, 0, 1, 0, 0};
    PDFRectangle cropBox (0, 0, 0, 0);
    int          neww, newh;
    pdf_image_info (temp, neww, newh, cropBox, tMat, pageInput);
    // debug_convert << "new w,h :" << neww << " "<< newh << LF;
    if ((neww != w) && (neww != 0)) {
      // image size changed, resize it to what we wanted
      double r= w / (double) neww;
      int    j;
      for (j= 0; j < 6; j++)
        tMat[j]*= r;
    }
    PDFFormXObject* form= dc.StartFormXObject (cropBox, id, tMat, true);
    status              = copyingContext->MergePDFPageToFormXObject (form, 0);
    if (status == eSuccess) pdfw.EndFormXObjectAndRelease (form);
    delete copyingContext;
  }
  if (!is_none (name)) remove (temp);

  if (status == PDFHummus::eFailure) {
    convert_error << "pdf_hummus, failed to include image file: " << temp << LF;
  }
}

bool
pdf_image_rep::flush_for_pattern (PDFWriter& pdfw) {
  string data, smask;
  int    iw= 0, ih= 0;
#ifdef QTTEXMACS
  qt_image_data (u, iw, ih, data, smask);
#else
  convert_error << "pdf_image_rep::flush_for_pattern: cannot export pattern "
                << u << "  to PDF" << LF;
#endif
  if ((iw == 0) || (ih == 0)) return false;

  ObjectsContext& objectsContext= pdfw.GetObjectsContext ();
  objectsContext.StartNewIndirectObject (id);

  DictionaryContext* imageContext= objectsContext.StartDictionary ();
  imageContext->WriteKey (scType);
  imageContext->WriteNameValue (scXObject);
  imageContext->WriteKey (scSubType);
  imageContext->WriteNameValue (scImage);
  imageContext->WriteKey (scWidth);
  imageContext->WriteIntegerValue (iw);
  imageContext->WriteKey (scHeight);
  imageContext->WriteIntegerValue (ih);
  imageContext->WriteKey (scBitsPerComponent);
  imageContext->WriteIntegerValue (8);
  imageContext->WriteKey (scColorSpace);
  imageContext->WriteNameValue (scDeviceRGB);
  ObjectIDType smaskId=
      objectsContext.GetInDirectObjectsRegistry ().AllocateNewObjectID ();
  imageContext->WriteKey ("SMask");
  imageContext->WriteNewObjectReferenceValue (smaskId);
  PDFStream* imageStream= objectsContext.StartPDFStream (imageContext, true);
  OutputStreamTraits   outputTraits (imageStream->GetWriteStream ());
  c_string             buf (data);
  InputByteArrayStream reader ((IOBasicTypes::Byte*) (char*) buf, N (data));
  EStatusCode          status= outputTraits.CopyToOutputStream (&reader);
  if (status != PDFHummus::eSuccess) {
    delete imageStream;
    return false;
  }
  objectsContext.EndPDFStream (imageStream); // It does EndIndirectObject();
  delete imageStream;

  objectsContext.StartNewIndirectObject (smaskId);
  DictionaryContext* smaskContext= objectsContext.StartDictionary ();
  smaskContext->WriteKey (scType);
  smaskContext->WriteNameValue (scXObject);
  smaskContext->WriteKey (scSubType);
  smaskContext->WriteNameValue (scImage);
  smaskContext->WriteKey (scWidth);
  smaskContext->WriteIntegerValue (iw);
  smaskContext->WriteKey (scHeight);
  smaskContext->WriteIntegerValue (ih);
  smaskContext->WriteKey (scBitsPerComponent);
  smaskContext->WriteIntegerValue (8);
  smaskContext->WriteKey (scColorSpace);
  smaskContext->WriteNameValue (scDeviceGray);
  PDFStream* smaskStream= objectsContext.StartPDFStream (smaskContext, true);
  OutputStreamTraits   smaskOutputTraits (smaskStream->GetWriteStream ());
  c_string             buf_smask (smask);
  InputByteArrayStream smaskReader ((IOBasicTypes::Byte*) (char*) buf_smask,
                                    N (smask));
  status= smaskOutputTraits.CopyToOutputStream (&smaskReader);
  if (status != PDFHummus::eSuccess) {
    delete smaskStream;
    return false;
  }
  objectsContext.EndPDFStream (smaskStream); // It does EndIndirectObject();
  delete smaskStream;
  return true;
}

void
pdf_raw_image_rep::flush (PDFWriter& pdfw) {
  // debug_convert << "flushing :" << id << LF;
  create_pdf_image_raw (pdfw, data, w, h, id);
}

void
hummus_pdf_image_size (url image, int& w, int& h) {
  url resolved_image= concretize_url (image);
  if (is_none (resolved_image)) {
    io_error << "Failed to concretize " << image << LF;
    return;
  }

  PDFParser* parser= new PDFParser ();
  InputFile  pdfFile;
  c_string   f (as_string (resolved_image));
  pdfFile.OpenFile ((char*) f);
  EStatusCode status= parser->StartPDFParsing (pdfFile.GetInputStream ());
  if (status != PDFHummus::eFailure) {
    PDFPageInput pageInput (parser, parser->ParsePage (0));
    double       tMat[6]= {1, 0, 0, 1, 0, 0};
    PDFRectangle cropBox (0, 0, 0, 0);
    pdf_image_info (image, w, h, cropBox, tMat, pageInput);
    delete (parser);
  }
  else {
    convert_error << "pdf_hummus, failed to get image size for: "
                  << resolved_image << LF << "resolved from " << image << LF;
    w= h= 0;
  }
}

void
pdf_image_info (url image, int& w, int& h, PDFRectangle& cropBox,
                double (&tMat)[6], PDFPageInput& pageInput) {
  int rot= (pageInput.GetRotate ()) % 360;
  if (rot < 0) rot+= 360;
  cropBox              = pageInput.GetCropBox ();
  PDFRectangle mediaBox= pageInput.GetMediaBox ();
  if (!(cropBox.LowerLeftX >= mediaBox.LowerLeftX &&
        cropBox.LowerLeftY >= mediaBox.LowerLeftY &&
        cropBox.UpperRightX <= mediaBox.UpperRightX &&
        cropBox.UpperRightY <= mediaBox.UpperRightY))
    convert_warning << "pdf_image_info, " << image << ": "
                    << "cropbox not included in mediabox\n";
  w= cropBox.UpperRightX - cropBox.LowerLeftX;
  h= cropBox.UpperRightY - cropBox.LowerLeftY;
  if (DEBUG_CONVERT) {
    debug_convert << "hummus_pdf_image_info:" << LF << "image =" << image << LF
                  << "crop box={" << cropBox.LowerLeftX << ", "
                  << cropBox.UpperRightX << ", " << cropBox.LowerLeftY << ", "
                  << cropBox.UpperRightY << "}" << LF << "w,h={" << w << ", "
                  << h << "}" << LF;
  }
  int z;
  switch (rot) {
  case 0:
    tMat[0]= 1;
    tMat[1]= 0;
    tMat[2]= 0;
    tMat[3]= 1;
    tMat[4]= -cropBox.LowerLeftX;
    tMat[5]= -cropBox.LowerLeftY;
    break;
  case 90:
    tMat[0]= 0;
    tMat[1]= -1;
    tMat[2]= 1;
    tMat[3]= 0;
    tMat[4]= -cropBox.LowerLeftY;
    tMat[5]= cropBox.UpperRightX;
    z      = w;
    w      = h;
    h      = z;
    break;
  case 180:
    tMat[0]= -1;
    tMat[1]= 0;
    tMat[2]= 0;
    tMat[3]= -1;
    tMat[4]= cropBox.UpperRightX;
    tMat[5]= cropBox.UpperRightY;
    break;
  case 270:
    tMat[0]= 0;
    tMat[1]= 1;
    tMat[2]= -1;
    tMat[3]= 0;
    tMat[4]= cropBox.UpperRightY;
    tMat[5]= -cropBox.LowerLeftX;
    z      = w;
    w      = h;
    h      = z;
    break;
  default:
    convert_error << "unexpected rotate()=" << rot << " in image " << image
                  << LF;
  }
  if (DEBUG_CONVERT)
    debug_convert << "degrees image rotated :" << rot << LF << "dx,dy={"
                  << tMat[4] << ", " << tMat[5] << "}" << LF;
}
