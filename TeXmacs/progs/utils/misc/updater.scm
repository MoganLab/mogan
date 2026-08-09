
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
    (with n
      (get-preference "updater:interval")
      (when (string-number? n)
        (updater-set-interval (string->number n))
        (updater-check-background)
        (when (> (string->number n) 0)
          (updater-scheduled-check)
        ) ;when
      ) ;when
    ) ;with
  ) ;when
) ;tm-define

(define-preferences ("updater:interval" "null" noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 状态显示辅助
;;
;; 纯 scheme 辅助函数优先 tm-define 导出（rootlet 全局可见），供 preferences
;; 面板跨模块调用（preferences-tools.scm 的 preferences-qml-current-value）。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 更新器状态整型 -> 显示名。
;; st: tm_updater 状态值（0 idle / 1 available / 2 downloading /
;;      3 ready / 4 applying / 5 failed）
;; 返回对应状态名，未知状态返回 "unknown"。
;; 无 checking：检查是查询操作，进行中由 updater-running? 表达。

(tm-define (updater-state-name st)
  (cond ((== st 0) "idle")
        ((== st 1) "available")
        ((== st 2) "downloading")
        ((== st 3) "ready")
        ((== st 4) "applying")
        ((== st 5) "failed")
        (else "unknown")
  ) ;cond
) ;tm-define

;; 更新状态显示串（纯展示，无网络）。
;; 非更新器平台返回空串；有可用版本的状态（available/downloading/ready）追加
;; 版本号（形如 "available · 1.2.3"），其余状态只显示状态名。

(tm-define (updater-status-string)
  (if (use-plugin-updater?)
    (let ((st (updater-state)))
      (if (or (== st 1) (== st 2) (== st 3))
        ;; 版本来自 Velopack（UTF-8 字节），scheme 字面量也按 UTF-8 存；
        ;; PreferencesBridge 对值统一 cork_to_utf8，故这里整体归一为 Cork。
        (string-append (updater-state-name st)
          (utf8->cork " · ")
          (utf8->cork (updater-available-version))
        ) ;string-append
        (updater-state-name st)
      ) ;if
    ) ;let
    ""
  ) ;if
) ;tm-define

;; 可用版本显示串。
;; 仅当存在真实更新（available/ready）时返回版本号，否则空串——避免一次
;; 「无更新」检查结束后残留上一次的过期版本号。

(tm-define (updater-available-version-display)
  (if (use-plugin-updater?)
    (let ((st (updater-state)))
      (if (or (== st 1) (== st 3)) (utf8->cork (updater-available-version)) "")
    ) ;let
    ""
  ) ;if
) ;tm-define

;; 前台主动检查更新（非更新器平台返回 #f）。
;; 返回是否已启动检查。

(tm-define (updater-check-now)
  (if (use-plugin-updater?) (updater-check-foreground) #f)
) ;tm-define

;; 下载已可用更新（非更新器平台或未到 available 状态返回 #f）。
;; 返回是否已启动下载。

(tm-define (updater-download-update)
  (if (use-plugin-updater?) (updater-download) #f)
) ;tm-define

;; 应用已就绪更新（成功后进程退出并安装；未就绪时返回 #f）。
;; 返回是否已触发应用。

(tm-define (updater-apply-update)
  (if (use-plugin-updater?)
    (with ok
      (updater-apply)
      ;; apply 成功意味着更新器进程已在后台等待本进程退出；走正常退出通道
      ;; (safely-quit-TeXmacs) 完成保存提示与 on-exit 清理后退出，更新器接管。
      (when ok
        (safely-quit-TeXmacs)
      ) ;when
      ok
    ) ;with
    #f
  ) ;if
) ;tm-define

;; 定时检查循环：每 10 分钟自查一次，按 updater:interval 偏好决定是否真正触发
;; 后台检查（距上次检查超过 interval 小时才检查）。
;; 由 updater-initialize 在 interval>0 时启动；每次执行后重新排定下一次
;; （delayed :idle 600000），形成周期循环。guard 保证仅更新器平台进入。

(tm-define (updater-scheduled-check)
  (when (use-plugin-updater?)
    (with interval
      (get-preference "updater:interval")
      (when (and (string-number? interval) (> (string->number interval) 0))
        (when (> (- (current-time) (updater-last-check)) (* (string->number interval) 3600))
          (updater-check-background)
        ) ;when
      ) ;when
    ) ;with
    (delayed (:idle 600000) (updater-scheduled-check))
  ) ;when
) ;tm-define

;; 重启以更新：更新已就绪（state==3，即 READY）时弹确认框，确认后触发
;; updater-apply-update（进程退出并安装）。其余状态为 no-op。

(tm-define (updater-maybe-restart-to-update)
  (when (use-plugin-updater?)
    (when (== (updater-state) 3)
      (with choice
        (cpp-confirm-restart (translate "Update ready")
          (translate "An update is ready to install. Restart Mogan now?")
        ) ;cpp-confirm-restart
        (when (== choice "restart")
          (updater-apply-update)
        ) ;when
      ) ;with
    ) ;when
  ) ;when
) ;tm-define
