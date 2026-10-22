/******************************************************************************
 * MODULE     : qt_attachment_converter.cpp
 * DESCRIPTION: 文档类附件的无头自转换（→ PDF）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_attachment_converter.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>
#include <QUuid>

// 大文档编译耗时不可预估，超时须宽松；超时后按失败处理并清理临时目录
static const int kAttachmentConvertTimeoutMs= 120 * 1000;

/**
 * @brief 临时 PDF 所在目录：与附件插件的 extract/ 同级。
 *
 * 基准目录与插件 attachment-cache-base（tm-attachment.scm）同一约定：
 * 平台缓存根（macOS ~/Library/Caches），不含应用名层级——
 * QStandardPaths::CacheLocation 会追加应用名目录，产物会落到与插件
 * 缓存不同的树下，故这里按插件逻辑镜像推导。
 */
static QString
attachment_convert_dir () {
#if defined(Q_OS_MACOS)
  QString base= QDir::home ().filePath ("Library/Caches");
#elif defined(Q_OS_WIN)
  QString base= qEnvironmentVariable ("LOCALAPPDATA");
  if (base.isEmpty ()) base= QDir::home ().filePath ("AppData/Local");
#else
  QString base= qEnvironmentVariable ("XDG_CACHE_HOME");
  if (base.isEmpty ()) base= QDir::home ().filePath (".cache");
#endif
  return QDir (base).filePath ("Liiistem/attachment/convert");
}

void
AttachmentConverter::start (const QString& sourcePath) {
  settled_= false;

  // 每次转换一个唯一子目录：同名附件（不同路径）不会互相覆盖产物
  QString dir=
      QDir (attachment_convert_dir ())
          .filePath (QUuid::createUuid ().toString (QUuid::WithoutBraces));
  if (!QDir ().mkpath (dir)) {
    emit failed (QObject::tr ("Cannot create temporary directory"));
    deleteLater ();
    return;
  }

  // 输出沿用原文件名加 .pdf：chip 显示名、插件上传文件名与 %chat 协议
  // filename 三处自然一致，用户看到的是「原文档名.pdf」而非一串随机名
  QFileInfo fi (sourcePath);
  QString   outPath= QDir (dir).filePath (fi.completeBaseName () + ".pdf");

  QString self= QCoreApplication::applicationFilePath ();
  if (self.isEmpty ()) {
    QDir (dir).removeRecursively ();
    emit failed (QObject::tr ("Host executable not found"));
    deleteLater ();
    return;
  }

  process_= new QProcess (this);
  connect (process_, &QProcess::errorOccurred, this,
           [this, dir] (QProcess::ProcessError) {
             // 启动失败等：进程不会正常进入 finished，统一按失败收口
             finishWithFailure (dir, QObject::tr ("Cannot start converter"));
           });
  connect (process_, &QProcess::finished, this,
           [this, dir, outPath] (int code, QProcess::ExitStatus status) {
             if (settled_) return;
             if (status != QProcess::NormalExit || code != 0) {
               finishWithFailure (
                   dir, QObject::tr ("Converter exited with %1").arg (code));
               return;
             }
             // 退出码 0 不保证产物可用：导出失败时 scheme 只往日志写提示，
             // 故以产物存在且非空为准
             QFileInfo out (outPath);
             if (!out.exists () || out.size () == 0) {
               finishWithFailure (dir,
                                  QObject::tr ("Converter produced no output"));
               return;
             }
             // 成功：临时目录保留（产物要交给 ingest 读），随缓存清理回收；
             // 置 settled_ 后超时定时器自然失效，本体随事件循环删除
             settled_= true;
             settleProcess ();
             emit converted (outPath);
             deleteLater ();
           });

  // -c 在 init_texmacs.cpp 中自动开启 headless 并隐含 -q（转换末尾追加
  // (quit-TeXmacs)），此处不传 -q
  process_->start (self, QStringList () << "-c" << sourcePath << outPath);
  if (!process_->waitForStarted (5000)) {
    finishWithFailure (dir, QObject::tr ("Cannot start converter"));
    return;
  }
  // 不阻塞 UI：等进程自然退出，靠 finished 信号收口；process_ 非空即
  // 「尚未收口」（所有收口路径都会将其置空），settled 后本定时器随
  // 上下文对象销毁自动取消
  QTimer::singleShot (kAttachmentConvertTimeoutMs, this, [this, dir] () {
    if (process_) {
      process_->kill ();
      finishWithFailure (dir, QObject::tr ("Conversion timed out"));
    }
  });
}

void
AttachmentConverter::settleProcess () {
  // settled_ 已挡住所有后续信号，deleteLater 抑制未投递事件，无需 disconnect
  if (process_) {
    process_->deleteLater ();
    process_= nullptr;
  }
}

void
AttachmentConverter::finishWithFailure (const QString& tempDir,
                                        const QString& reason) {
  // 三条失败路径（启动失败/超时/退出码非零）可能竞态触发，只收口一次
  if (settled_) return;
  settled_= true;
  settleProcess ();
  QDir (tempDir).removeRecursively ();
  emit failed (reason);
  deleteLater ();
}