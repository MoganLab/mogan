
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : updater.scm
;; DESCRIPTION : support utilities for tm_updater
;; COPYRIGHT   : (C) 2013 Miguel de Benito Delgado
;;               2019 modified by Gregoire Lecerf
;;               2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils misc updater))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preference management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (updater-initialize)
  (when (use-plugin-updater?)
    (updater-check-background)
    (updater-scheduled-check)
    (updater-auto-download-loop)
  ) ;when
) ;tm-define

(define-preferences ("updater:interval" "null" noop))

;; 下载已可用更新（非更新器平台或未到 available 状态返回 #f）。
;; 返回是否已启动下载。

(tm-define (updater-download-update)
  (if (use-plugin-updater?) (updater-download) #f)
) ;tm-define

;; 应用已就绪更新（未就绪时返回 #f）。
;; apply 成功意味着更新器进程已在后台等待本进程退出；走正常退出通道
;; (safely-quit-TeXmacs) 完成保存提示与 on-exit 清理后退出，更新器随即接管。
;; 返回是否已触发应用。

(tm-define (updater-apply-update)
  (if (use-plugin-updater?)
    (with ok (updater-apply) (when ok (safely-quit-TeXmacs)) ok)
    #f
  ) ;if
) ;tm-define

;; 定时检查循环：每 10 分钟自查一次，距上次检查超过 1 小时才真正触发后台检查。
;; 由 updater-initialize 启动；每次执行后重新排定下一次
;; （delayed :idle 600000），形成周期循环。guard 保证仅更新器平台进入。

(tm-define (updater-scheduled-check)
  (when (use-plugin-updater?)
    (when (> (- (current-time) (updater-last-check)) 3600)
      (updater-check-background)
    ) ;when
    (delayed (:idle 600000) (updater-scheduled-check))
  ) ;when
) ;tm-define

;; 自动下载监听循环：每秒轮询一次状态机，available（==2）时自动触发下载。
;; 下载就绪（==4）后不再动作（下次启动由 VelopackApp 自动应用）；失败（==6）
;; 时同样不动作，等下一次定时检查（1 小时）重新发现更新后再试。
;; 由 updater-initialize 启动；guard 保证仅更新器平台进入。

(tm-define (updater-auto-download-loop)
  (when (use-plugin-updater?)
    (when (== (updater-state) 2)
      (updater-download-update)
    ) ;when
    (delayed (:idle 1000) (updater-auto-download-loop))
  ) ;when
) ;tm-define
