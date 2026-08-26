
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

(tm-define (updater-initialize)
  (when (use-plugin-updater?)
    (updater-check-background)
    (updater-scheduled-check)
    (updater-auto-download-loop)
  ) ;when
) ;tm-define

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
;; （delayed :pause 600000），形成周期循环。guard 保证仅更新器平台进入。
;; 注意：不能用 :idle —— 本环境 delayed :idle 不触发（见 devel/0512.md）。

(tm-define (updater-scheduled-check)
  (when (use-plugin-updater?)
    (when (> (- (current-time) (updater-last-check)) 3600)
      (updater-check-background)
    ) ;when
    (delayed (:pause 600000) (updater-scheduled-check))
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
    (delayed (:pause 1000) (updater-auto-download-loop))
  ) ;when
) ;tm-define

;; ---- 更新通道(stable/beta)切换 ----
;; 首选项 update-channel:单值 "stable"/"beta"(缺省 stable),C++ 侧 tm_velopack
;; 以 ExplicitChannel 显式跟随该值(见 devel/0518.md)。切换走两次确认:
;; 第一次确认切换方向,第二次确认强制重启走 download+apply;任一步取消则
;; 什么都不动(首选项不写)。

(tm-define (updater-current-channel)
  (if (== (get-preference "update-channel") "beta") "beta" "stable")
) ;tm-define

;; 两按钮确认弹窗:确认返回 #t,取消(含 Esc/关闭)返回 #f。

(define (updater-question message ok-label)
  (== (cpp-confirm-question message (list (translate "Cancel") ok-label)) 1)
) ;define

(define (updater-channel-name channel)
  (if (== channel "beta") (translate "Beta") (translate "Stable"))
) ;define

;; 切换链轮询:检查(1)/下载(3)继续等;可用(2)自动触发下载;就绪(4)apply
;; (updater-apply-update 内部 safely-quit-TeXmacs 退出,更新器接管重启);空闲(0)
;; 即目标通道暂无可用版本;失败(6)报错。ticks 上限防御轮询空转。

(define (updater-switch-chain-poll ticks)
  (with st
    (updater-state)
    (cond ((== st 2)
           (updater-download-update)
           (delayed (:pause 1000) (updater-switch-chain-poll ticks))
          ) ;
          ((== st 4) (updater-apply-update))
          ((== st 0)
           (set-message "Channel switched; the next release on this channel will be offered"
             "Update channel"
           ) ;set-message
          ) ;
          ((== st 6)
           (set-message (string-append "Update check failed: " (updater-error-code))
             "Update channel"
           ) ;set-message
          ) ;
          ((< ticks 600) (delayed (:pause 1000) (updater-switch-chain-poll (+ ticks 1))))
          (else (set-message "Timed out waiting for the update check" "Update channel"))
    ) ;cond
  ) ;with
) ;define

;; 启动切换链:checkInBackground 返回 #f 说明已有检查/下载在进行(其快照
;; 可能仍是旧通道),等它结束再重新触发,保证链上的检查用的是新通道快照。

(define (updater-switch-chain-start ticks)
  (if (updater-check-background)
    (updater-switch-chain-poll 0)
    (if (< ticks 600)
      (delayed (:pause 1000) (updater-switch-chain-start (+ ticks 1)))
      (set-message "Timed out waiting for the previous update task" "Update channel")
    ) ;if
  ) ;if
) ;define

(tm-define (updater-switch-channel target)
  (when (and (use-plugin-updater?) (!= target (updater-current-channel)))
    (with prompt
      (if (== target "beta")
        (translate "Switch to the Beta update channel? Beta releases may be unstable.")
        (translate "Switch back to the Stable update channel? The latest stable version may be older than the current one."
        ) ;translate
      ) ;if
      (when (updater-question prompt (updater-channel-name target))
        (when (updater-question (translate "The application will check for updates on the new channel and restart to apply. Continue?"
                                ) ;translate
                (translate "Restart")
              ) ;updater-question
          (set-preference "update-channel" target)
          (save-preferences)
          (updater-switch-chain-start 0)
        ) ;when
      ) ;when
    ) ;with
  ) ;when
) ;tm-define
