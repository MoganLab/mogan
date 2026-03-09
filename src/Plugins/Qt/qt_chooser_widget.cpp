/******************************************************************************
 * MODULE     : qt_chooser_widget.cpp
 * DESCRIPTION: File chooser widget, native and otherwise
 * COPYRIGHT  : (C) 2008  Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_chooser_widget.hpp"
#include "analyze.hpp"
#include "convert.hpp"
#include "converter.hpp"
#include "dictionary.hpp"
#include "editor.hpp"
#include "image_files.hpp"
#include "message.hpp"
#include "new_view.hpp" // get_current_editor()
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "widget.hpp"
#ifdef USE_MUPDF_RENDERER
#include "mupdf_picture.hpp"
#endif

#include <QByteArray>
#include <QDebug>
#include <QFileDialog>
#include <QString>
#include <QStringList>

#include <moebius/data/scheme.hpp>

using moebius::data::scm_quote;

/*!
  \param _cmd  Scheme closure to execute after the dialog is closed.
  \param _type What kind of dialog to show. Can be one of "image", "directory",
               or any of the supported file formats: "texmacs", "postscript",
               etc. See perform_dialog()
 */
qt_chooser_widget_rep::qt_chooser_widget_rep (command _cmd, string _type,
                                              string _prompt)
    : qt_widget_rep (file_chooser), cmd (_cmd), prompt (_prompt),
      position (coord2 (0, 0)), size (coord2 (100, 100)), file ("") {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_chooser_widget_rep::qt_chooser_widget_rep type=\""
                  << type << "\" prompt=\"" << prompt << "\"" << LF;
  if (!set_type (_type)) set_type ("generic");
}

void
qt_chooser_widget_rep::send (slot s, blackbox val) {
  switch (s) {
  case SLOT_VISIBILITY: {
    check_type<bool> (val, s);
    bool flag= open_box<bool> (val);
    (void) flag;
    NOT_IMPLEMENTED ("qt_chooser_widget::SLOT_VISIBILITY");
  } break;
  case SLOT_SIZE:
    check_type<coord2> (val, s);
    size= open_box<coord2> (val);
    break;
  case SLOT_POSITION:
    check_type<coord2> (val, s);
    position= open_box<coord2> (val);
    break;
  case SLOT_KEYBOARD_FOCUS:
    check_type<bool> (val, s);
    perform_dialog ();
    break;
  case SLOT_STRING_INPUT:
    check_type<string> (val, s);
    if (DEBUG_QT_WIDGETS)
      debug_widgets << "\tString input: " << open_box<string> (val) << LF;
    NOT_IMPLEMENTED ("qt_chooser_widget::SLOT_STRING_INPUT");
    break;
  case SLOT_INPUT_TYPE:
    check_type<string> (val, s);
    set_type (open_box<string> (val));
    break;
  case SLOT_FILE:
    // send_string (THIS, "file", val);
    check_type<string> (val, s);
    if (DEBUG_QT_WIDGETS)
      debug_widgets << "\tFile: " << open_box<string> (val) << LF;
    file= open_box<string> (val);
    break;
  case SLOT_DIRECTORY:
    check_type<string> (val, s);
    directory= open_box<string> (val);
    directory= as_string (url_pwd () * url_system (directory));
    break;

  default:
    qt_widget_rep::send (s, val);
  }
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_chooser_widget_rep: sent " << slot_name (s)
                  << "\t\tto widget\t" << type_as_string () << LF;
}

blackbox
qt_chooser_widget_rep::query (slot s, int type_id) {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_chooser_widget_rep::query " << slot_name (s) << LF;
  switch (s) {
  case SLOT_POSITION: {
    check_type_id<coord2> (type_id, s);
    return close_box<coord2> (position);
  }
  case SLOT_SIZE: {
    check_type_id<coord2> (type_id, s);
    return close_box<coord2> (size);
  }
  case SLOT_STRING_INPUT: {
    check_type_id<string> (type_id, s);
    if (DEBUG_QT_WIDGETS) debug_widgets << "\tString: " << file << LF;
    return close_box<string> (file);
  }
  default:
    return qt_widget_rep::query (s, type_id);
  }
}

widget
qt_chooser_widget_rep::read (slot s, blackbox index) {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "qt_chooser_widget_rep::read " << slot_name (s) << LF;
  switch (s) {
  case SLOT_WINDOW:
    check_type_void (index, s);
    return this;
  case SLOT_FORM_FIELD:
    check_type<int> (index, s);
    return this;
  case SLOT_FILE:
    check_type_void (index, s);
    return this;
  case SLOT_DIRECTORY:
    check_type_void (index, s);
    return this;
  default:
    return qt_widget_rep::read (s, index);
  }
}

/*!
 @note: name is a unique identifier for the window, but for this widget we
 identify it with the window title. This is not always the case.
 */
widget
qt_chooser_widget_rep::plain_window_widget (string s, command q, int b) {
  (void) b;
  win_title= s;
  quit     = q;
  return this;
}

bool
qt_chooser_widget_rep::set_type (const string& _type) {
  QString mainNameFilter;
  if (_type == "directory") {
    type= _type;
    return true;
  }

  if (format_exists (_type)) {
    mainNameFilter= to_qstring (
        translate (as_string (call ("format-get-name", _type)) * " file"));
  }
  else if (_type == "image") {
    mainNameFilter= to_qstring (translate ("Image file"));
  }
  else if (_type == "action_open") {
    mainNameFilter= to_qstring (translate ("STEM files"));
  }
  else if (_type == "action_save_as") {
    mainNameFilter= to_qstring (translate ("STEM files"));
  }
  else if (_type == "action_include") {
    mainNameFilter= to_qstring (translate ("STEM files for include"));
  }
  else if (_type == "text") {
    mainNameFilter= to_qstring (translate ("Plain text files"));
  }
  else if (_type == "generic") {
    mainNameFilter= to_qstring (translate ("All files"));
  }

  else {
    if (DEBUG_STD)
      debug_widgets << "qt_chooser_widget: IGNORING unknown format " << _type
                    << LF;
    return false;
  }
  if (_type == "image") {
    mainNameFilter+=
        " (*.jpg *.jpeg *.jpe *.png *.bmp *.tif *.tiff *.svg *.pdf *.webp)";
    nameFilters << mainNameFilter;
    nameFilters << to_qstring (translate ("JPEG File InterChange Format") *
                               " (*.jpg *.jpeg *.jpe)");
    nameFilters << to_qstring (translate ("Portable Network Graphics") *
                               " (*.png)");
    nameFilters << to_qstring (translate ("Windows Bitmap") * " (*.bmp)");
    nameFilters << to_qstring (translate ("Tag Image File Format") *
                               " (*.tif *tiff)");
    nameFilters << to_qstring (translate ("Scalable Vector Graphics") *
                               " (*.svg)");
    nameFilters << to_qstring (translate ("Portable Document Format") *
                               " (*.pdf)");
    nameFilters << to_qstring (translate ("WebP Image Format") * " (*.webp)");
  }
  else if (_type == "text") {
    mainNameFilter+= " (*.txt)";
    nameFilters << mainNameFilter;
    nameFilters << to_qstring (translate ("All Format") * " (*)");
  }
  else if (_type == "generic") {
    nameFilters << to_qstring (translate ("All Format") * " (*)");
  }
  else if (_type == "action_open") {
    mainNameFilter+= " (*.tmu *.tm *.ts *.tp)";
    //" (*.scala *.sc *.sbt *.pants *.ltx *.sty *.cls *.tex *.bib *.rawbib *.jl
    //*.js *.java *.sld *.ss *.tmu *.txt *.py *.json *.html *.hh *.cpp *cc *hpp
    //*.scm *.elv *.md *.sh *.csv)"
    nameFilters << mainNameFilter;
    nameFilters << to_qstring (translate ("Data files") * " (*.json *.csv)");
    nameFilters << to_qstring (translate ("Text files") * " (*.md *.txt)");
    nameFilters << to_qstring (translate ("C++ files") *
                               " (*.hh *.cc *.cpp *hpp)");
    nameFilters << to_qstring (translate ("Python files") * " (*.py *.pants)");
    nameFilters << to_qstring (translate ("Script files") *
                               " (*.sh *.elv *.py *.js)");
    nameFilters << to_qstring (translate ("Scala files") *
                               " (*.sbt *.sc *.scala)");
    nameFilters << to_qstring (translate ("Julia files") * " (*.jl)");
    nameFilters << to_qstring (translate ("Java files") * " (*.java)");
    nameFilters << to_qstring (translate ("Scheme files") *
                               " (*.ss *.sld *.scm)");
    nameFilters << to_qstring (translate ("Latex files") *
                               " (*.ltx *.sty *.tex *.bib *.rawbib *.cls)");
    nameFilters << to_qstring (translate ("Web files") *
                               " (*.html *.xhtml *.htm)");
  }
  else if (_type == "action_save_as") {
    mainNameFilter+= " (*.tmu)";
    nameFilters << mainNameFilter;
    nameFilters << to_qstring (translate ("TM files") * " (*.tm)");
  }
  else if (_type == "action_include") {
    mainNameFilter+= " (*.tmu *.tm)";
    nameFilters << mainNameFilter;
    nameFilters << to_qstring (translate ("TMU files") * " (*.tmu)");
    nameFilters << to_qstring (translate ("TM files") * " (*.tm)");
  }
  else {
    mainNameFilter+= " (";
    object        ret     = call ("format-get-suffixes*", _type);
    array<object> suffixes= as_array_object (ret);
    if (N (suffixes) > 1) defaultSuffix= to_qstring (as_string (suffixes[1]));
    for (int i= 1; i < N (suffixes); ++i)
      mainNameFilter+= " *." + to_qstring (as_string (suffixes[i]));
    mainNameFilter+= " )";
    nameFilters << mainNameFilter;
    for (int i= 1; i < N (suffixes); ++i)
      nameFilters << to_qstring (as_string (suffixes[i])) + "(*." +
                         to_qstring (as_string (suffixes[i])) + ")";
  }

  type= _type;
  return true;
}

/*! Actually displays the dialog with all the options set.
 * Uses a native dialog on Mac/Win and opens a custom dialog with image preview
 * for other platforms.
 */
void
qt_chooser_widget_rep::perform_dialog () {
  QString  caption= to_qstring (win_title);
  c_string tmp (directory * "/" * file);
  QString  path= QString::fromUtf8 (&tmp[0]);

  QFileDialog* dialog= new QFileDialog (this->as_qwidget (), caption, path);

  dialog->setViewMode (QFileDialog::Detail);
  if (type == "directory") dialog->setFileMode (QFileDialog::Directory);
  else if (type == "image" && prompt == "")
    // check non saving mode just in case we support it
    dialog->setFileMode (QFileDialog::ExistingFile);
  else if (prompt != "")
    // Save dialogs
    dialog->setFileMode (QFileDialog::AnyFile);
  else
    // Open dialogs, 仅允许选择已存在的文件（不允许选择文件夹）
    dialog->setFileMode (QFileDialog::ExistingFile);

  // set label text for accept: Save or Open
  if (prompt != "") {
    string text= prompt;
    if (ends (text, ":")) text= text (0, N (text) - 1);
    if (ends (text, " as")) text= text (0, N (text) - 3);
    dialog->setAcceptMode (QFileDialog::AcceptSave);
    dialog->setLabelText (QFileDialog::Accept, to_qstring (translate (text)));
  }
  else {
    dialog->setLabelText (QFileDialog::Accept, to_qstring (translate ("Open")));
  }

#if (QT_VERSION >= 0x040400)
  if (type != "directory") {
    // QStringList filters;
    // if (nameFilter != "") filters << nameFilter;
    // filters << to_qstring (translate ("All files (*)"));
    // nameFilters << to_qstring (translate ("All files (*)"));
    dialog->setNameFilters (nameFilters);
  }
#endif

  dialog->setLabelText (QFileDialog::Reject, to_qstring (translate ("Cancel")));
  dialog->setLabelText (QFileDialog::FileName,
                        to_qstring (translate ("File name")));
  dialog->setLabelText (QFileDialog::FileType,
                        to_qstring (translate ("File type")));
  dialog->setLabelText (QFileDialog::LookIn,
                        to_qstring (translate ("Directory")));

#ifdef OS_MACOS
  // If the current directory is root, we should reset it to home
  if ((dialog->directory ()).isRoot ()) {
    dialog->setDirectory (QDir::home ());
  }
#endif

  dialog->updateGeometry ();
  QSize  sz = dialog->sizeHint ();
  QPoint pos= to_qpoint (position);
  QRect  r;

  r.setSize (sz);
  r.moveCenter (pos);
  dialog->setGeometry (r);

  QStringList fileNames;
  file= "#f";
  if (dialog->exec ()) {
    QString selectedFilter= dialog->selectedNameFilter ();
    if (selectedFilter.contains ("TMU files")) {
      defaultSuffix= "tmu";
    }
    else if (selectedFilter.contains ("TM files")) {
      defaultSuffix= "tm";
    }
    fileNames= dialog->selectedFiles ();
    if (fileNames.count () > 0) {
      QString imqstring= fileNames.first ();
      // QTBUG-59401: QFileDialog::setDefaultSuffix doesn't work when file path
      // contains a dot
      if (!defaultSuffix.isEmpty () && imqstring.contains (QLatin1Char ('/')) &&
          !imqstring.endsWith (QLatin1Char ('/')) &&
          imqstring.indexOf (QLatin1Char ('.'),
                             imqstring.lastIndexOf (QLatin1Char ('/'))) == -1) {
        imqstring= imqstring + QLatin1Char ('.') + defaultSuffix;
      }
      if (!defaultSuffix.isEmpty () &&
          (defaultSuffix == "tm" || defaultSuffix == "tmu") &&
          imqstring.contains (QLatin1Char ('/')) &&
          !imqstring.endsWith (QLatin1Char ('/')) &&
          imqstring.indexOf (QLatin1Char ('.'),
                             imqstring.lastIndexOf (QLatin1Char ('/'))) != -1) {
        QString rightSuffix= QLatin1Char ('.') + defaultSuffix;
        if (!imqstring.endsWith (rightSuffix)) {
          imqstring= imqstring + rightSuffix;
        }
      }
      string imname= from_qstring_utf8 (imqstring);
      file         = "(system->url " * scm_quote (imname) * ")";
      if (type == "image") {
        /*
        QPixmap pic (fileNames.first()); // Qt can't eps & pdf in windows.
        string params;
        // HACK: which value should we choose here?
        //On other platforms we call image_size (u,  w,  h) which returns size
        in pt units. int ww = get_current_editor()->get_page_width (false) /
        PIXEL; int  w = pic.width (); int  h = pic.height (); string unit= "pt";
        if (w == 0)
          image_size (url_system (imname), w, h);
        if (w >= ww) {
          h= (int) ((((double) h) / ((double) w)) * ((double) ww));
          w= ww;
        }
        if (w > 0) {
          params << "\"" << as_string (w) << unit << "\" "
                 << "\"" << as_string (h) << unit << "\" "
                 << "\"" << "" << "\" "  // xps ??
                 << "\"" << "" << "\"";   // yps ??
        }
        else {
          params << "\"" << as_string (ww) << "px\" "
                 << "\"" << "" << "\" "
                 << "\"" << "" << "\" "  // xps ??
                 << "\"" << "" << "\"";   // yps ??
        }
        file = "(list " * file * params * ")";
        */
        url    u= url_system (imname);
        string w, h;
#ifdef USE_MUPDF_RENDERER
        mupdf_pretty_image_size (u, w, h);
#else
        qt_pretty_image_size (u, w, h);
#endif
        string params;
        params << "\"" << w << "\" "
               << "\"" << h << "\" "
               << "\""
               << ""
               << "\" " // xps ??
               << "\""
               << ""
               << "\""; // yps ??
        file= "(list " * file * " " * params * ")";
      }
    }
  }

  delete dialog;

  cmd ();
  if (!is_nil (quit)) quit ();
}
