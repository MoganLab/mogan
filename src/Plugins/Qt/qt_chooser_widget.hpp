/******************************************************************************
 * MODULE     : qt_chooser_widget.hpp
 * DESCRIPTION: File chooser widget, native and otherwise
 * COPYRIGHT  : (C) 2008  Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_CHOOSER_WIDGET_HPP
#define QT_CHOOSER_WIDGET_HPP

#include "qt_utilities.hpp"
#include "qt_widget.hpp"

/*!
  A file/directory chooser dialog, using native dialogs where available.
  See @link widget.cpp @endlink for an explanation of send(), query(),
  read(), etc.
 */
class qt_chooser_widget_rep : public qt_widget_rep {
protected:
  command cmd;       //!< Scheme closure to execute when the file is chosen
  command quit;      //!< Execute when the dialog closes.
  string  type;      //!< File types to filter in the dialog
  string  prompt;    //!< Is this a "Save" dialog?
  string  win_title; //!< Set by plain_window_widget()

  string directory; //!< Set this property sending SLOT_DIRECTORY to this widget
  coord2 position;  //!< Set this property sending SLOT_POSITION to this widget
  coord2 size;      //!< Set this property sending SLOT_SIZE to this widget
  string file;      //!< Set this property sending SLOT_FILE to this widget

  // QString nameFilter;    //!< For use in QFileDialog::setNameFilter()
  QStringList nameFilters;   //!< For use in QFileDialog::setNameFilters()
  QString     defaultSuffix; //!< For use in QFileDialog::setDefaultSuffix()

public:
  qt_chooser_widget_rep (command, string, string);

  virtual void     send (slot s, blackbox val);
  virtual blackbox query (slot s, int type_id);
  virtual widget   read (slot s, blackbox index);
  virtual widget   plain_window_widget (string s, command q, int b);

  bool set_type (const string& _type);
  void perform_dialog ();
};

/*! @brief 另存为建议名改写与目标格式判定：.ts → .stem，.tm → .tmu
 *         （继任格式引导，任务 1279）
 *  @param file 建议文件名，命中 .ts/.tm 时被就地改写为默认后缀
 *  @return 目标默认后缀（"stem"/"tmu"），空串表示非引导场景；非空时最终文件名
 *          需按所选过滤器后缀规范（chooser_normalize_suffix），为 "stem" 时
 *          过滤器收窄为 stem/ts/tmu 三项（chooser_stem_save_as_filters）
 *  @note 纯函数，供单测；不触发 scheme 调用
 */
string chooser_save_as_target (string& file);

/*! @brief .ts/.stem 另存的过滤器：stem（默认）在前、ts（兼容）居中、
 *         tmu（互转，任务 1297）在后
 *  @note 纯函数，供单测；translate 在未加载字典时返回原文，不依赖 scheme
 */
QStringList chooser_stem_save_as_filters ();

/*! @brief 从名称过滤器的括号内容解析首个后缀（如 "TS files (*.ts)" → "ts"），
 *         无括号或无 `*.xxx` 模式时返回空串
 *  @note 纯函数，供单测
 */
QString chooser_filter_first_suffix (const QString& filter);

/*! @brief 保存确认后的最终文件名：去掉最后一个分隔符后的旧后缀，接上指定后缀；
 *         path 非文件路径（无 `/` 或以 `/` 结尾）或后缀为空时原样返回
 *  @note 纯函数，供单测
 */
QString chooser_normalize_suffix (const QString& path, const QString& suffix);

#endif // QT_CHOOSER_WIDGET_HPP
