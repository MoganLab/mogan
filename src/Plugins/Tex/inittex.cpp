
/******************************************************************************
 * MODULE     : inittex.cpp
 * DESCRIPTION: initialize conversion from and to TeX
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Tex/convert_tex.hpp"
#include "Tex/tex.hpp"
#include "rel_hashmap.hpp"
#include "scheme.hpp"

static string
paper_opts_func (string s) {
  return as_string (call ("latex-paper-opts", s));
}

static string
paper_type_func (string s) {
  return as_string (call ("latex-paper-type", s));
}

static string
latex_type_func (string s) {
  return as_string (call ("latex-type", s));
}

static int
latex_arity_func (string s) {
  return as_int (call ("latex-arity", s));
}

hashfunc<string, string> paper_std_opts (paper_opts_func, "undefined");
hashfunc<string, string> paper_std_type (paper_type_func, "undefined");
hashfunc<string, string> latex_std_type (latex_type_func, "undefined");
hashfunc<string, int>    latex_std_arity (latex_arity_func, 0);

static array<string> empty_array_string;

rel_hashmap<string, string>        command_type ("undefined");
rel_hashmap<string, int>           command_arity (0);
rel_hashmap<string, array<string>> command_def (empty_array_string);

string
paper_opts (string s) {
  return paper_std_opts[s];
}

string
paper_type (string s) {
  return paper_std_type[s];
}

string
latex_type (string s) {
  if (command_type->contains (s)) return command_type[s];
  else return latex_std_type[s];
}

int
latex_arity (string s) {
  if (command_arity->contains (s)) return command_arity[s];
  else return latex_std_arity[s];
}

#ifdef QTTEXMACS
#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>

static QDialog* latex_progress_dialog= nullptr;
static int      latex_progress_total = 0;
#endif

void
latex_progress_start (int total) {
#ifdef QTTEXMACS
  if (QApplication::instance () &&
      qobject_cast<QApplication*> (QApplication::instance ())) {
    if (latex_progress_dialog) {
      latex_progress_dialog->close ();
      delete latex_progress_dialog;
      latex_progress_dialog= nullptr;
    }
    QWidget* main_window= QApplication::activeWindow ();
    latex_progress_total= total;

    QDialog* dlg= new QDialog (main_window, Qt::Sheet);
    dlg->setWindowTitle ("LaTeX Export");
    dlg->setMinimumWidth (400);
    dlg->setWindowModality (Qt::WindowModal);

    QVBoxLayout* layout= new QVBoxLayout (dlg);
    QLabel*      label = new QLabel ("Exporting LaTeX...", dlg);
    label->setAlignment (Qt::AlignCenter);
    layout->addWidget (label);

    QProgressBar* bar= new QProgressBar (dlg);
    bar->setRange (0, 100);
    bar->setValue (0);
    bar->setTextVisible (true);
    bar->setMinimumHeight (20);
    bar->setStyleSheet ("QProgressBar { border: 1px solid grey; border-radius: "
                        "5px; text-align: center; background-color: #f0f0f0; } "
                        "QProgressBar::chunk { background-color: #3498db; }");
    layout->addWidget (bar);

    QHBoxLayout* btnLayout= new QHBoxLayout ();
    QPushButton* btn      = new QPushButton ("Cancel", dlg);
    btnLayout->addStretch ();
    btnLayout->addWidget (btn);
    layout->addLayout (btnLayout);

    QObject::connect (btn, &QPushButton::clicked, dlg, &QDialog::reject);

    dlg->show ();
    dlg->repaint ();
    QCoreApplication::processEvents ();
    QThread::msleep (50); // 给 Qt 50ms 充分的时间完成第一帧渲染

    latex_progress_dialog= dlg;
  }
#else
  (void) total;
#endif
}

void
latex_progress_update (int current) {
#ifdef QTTEXMACS
  if (latex_progress_dialog) {
    QProgressBar* bar= latex_progress_dialog->findChild<QProgressBar*> ();
    if (bar) {
      int target= 100;
      if (latex_progress_total > 0) {
        target= (current * 100) / latex_progress_total;
      }
      int prev= bar->value ();
      if (target > prev) {
        int step= (target - prev) / 10;
        if (step < 1) step= 1;
        for (int val= prev + step; val <= target; val+= step) {
          bar->setValue (val);
          bar->repaint ();
          QCoreApplication::processEvents ();
          QThread::msleep (15);
        }
      }
      bar->setValue (target);
      bar->repaint ();
    }
    latex_progress_dialog->repaint ();
    QCoreApplication::processEvents ();
  }
#else
  (void) current;
#endif
}

void
latex_progress_end () {
#ifdef QTTEXMACS
  if (latex_progress_dialog) {
    latex_progress_dialog->close ();
    delete latex_progress_dialog;
    latex_progress_dialog= nullptr;
  }
#endif
}
