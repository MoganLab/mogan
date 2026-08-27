
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

;; update-channel 的缺省值注册(不经 define-preferences 的 notify 机制,纯默认表)。
;; 首选项 combo 经 get-pretty-preference 读值,未注册缺省时未写值会显示
;; "default" 而非 "stable"。
(when (not (ahash-ref preferences-default "update-channel"))
  (ahash-set! preferences-default "update-channel" "stable")
) ;when

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

;; ---- 下载中间态弹窗 ----
;; 切换通道确认后、检查尚未开始时即无条件打开非阻塞弹窗
;; (cpp-updater-dialog-open → run_modal_qml_dialog 的 setModal+show,不嵌套事件
;; 循环,delayed 轮询照常跑),只显示无限转圈 + 文案,无进度条。打开时机固定在
;; updater-switch-channel 的确认之后,不再依赖状态机读到 AVAILABLE(2) 或
;; DOWNLOADING(3)——第二次切换时 auto-download-loop 可能已抢先触发下载,切换链
;; 的 poll 只看到 READY(4),若等到状态 2/3 才开窗就会错过弹窗。弹窗幂等由 C++
;; g_updater_dialog_host 保证,重复调用 no-op。就绪(4)/空闲(0)/失败(6)/超时均
;; 关闭;失败(6)关窗后另弹阻塞确认提示失败(带错误码)。下载中不可取消(QML
;; onCancel 覆盖为 no-op)——切通道的下载是强制步骤,关窗只丢反馈,下载本身
;; 照常,没有取消路径。

(define (updater-switch-dialog-cleanup)
  (cpp-updater-dialog-close)
) ;define

;; 无条件打开下载中间态弹窗(幂等由 C++ g_updater_dialog_host 保证,重复调用
;; no-op)。文案走 translate,字典命中 zh_CN "正在下载更新,请稍候..."(见
;; zh_CN.scm)。

(define (updater-switch-dialog-open)
  (cpp-updater-dialog-open (translate "Downloading the update..."))
) ;define

;; 失败提示:阻塞确认弹窗,单 OK 按钮(Esc/X 关闭),带 Velopack 错误码。下载已
;; 结束、链路终止,exec 阻塞安全(同 READY 重启确认)。zh_CN 文案"更新检查失败:"
;; 见 zh_CN.scm。

(define (updater-notify-failure)
  (cpp-confirm-question (string-append (translate "Update check failed: ") (updater-error-code))
    (list (translate "OK"))
  ) ;cpp-confirm-question
) ;define

;; 切换链轮询:检查(1)/应用(5)继续等;可用(2)兜底开弹窗并自动触发下载;下载
;; 中(3)再兜底开一次弹窗(幂等,C++ 侧去重)——弹窗已在切换确认时打开,这里只
;; 防 auto-download-loop 抢先导致 poll 进链后状态直接是 DOWNLOADING 的时序;
;; 就绪(4)关弹窗 + 询问是否立即重启,确认则 apply(内部 safely-quit-TeXmacs
;; 退出,更新器接管重启),取消则提示下次启动自动应用;空闲(0)即目标通道暂无
;; 可用版本;失败(6)关窗 + 弹阻塞确认提示失败。ticks 上限防御轮询空转。

(define (updater-switch-chain-poll ticks)
  (with st
    (updater-state)
    (cond ((== st 2)
           (updater-switch-dialog-open)
           (updater-download-update)
           (delayed (:pause 1000) (updater-switch-chain-poll ticks))
          ) ;
          ;; 下载中:弹窗已在切换确认时打开(此处再开一次兜底,防 auto-download-loop
          ;; 抢先触发下载、poll 直接看到 DOWNLOADING 而错过 st==2;C++ 侧幂等,不会
          ;; 重复弹)。无进度推进。ticks 不递增(下载全量包(266MB)可能远超 10 分钟,
          ;; 递增会在下载完成前超时退出,poll 失去 READY 后触发重启的机会——见 0518
          ;; 切换不自动重启修复)。
          ((== st 3)
           (updater-switch-dialog-open)
           (delayed (:pause 1000) (updater-switch-chain-poll ticks))
          ) ;
          ;; 就绪:下载已完成,关中间态弹窗,用阻塞确认问是否立即重启——此时 poll 已
          ;; 无继续轮询的必要(下载结束),exec 阻塞安全。确认则 apply(成功后
          ;; safely-quit-TeXmacs 退出,更新器接管重启;失败 state→FAILED,delayed
          ;; 让 poll 读到 FAILED 报错);取消则本次不 apply、链路结束——READY 状态
          ;; 不会自动前进,继续轮询会每秒重弹确认;下次启动由 VelopackApp 自动应用。
          ((== st 4)
           (updater-switch-dialog-cleanup)
           (if (updater-question (translate "The update is ready. Restart now to apply it?")
                 (translate "Restart")
               ) ;updater-question
             (begin
               (updater-apply-update)
               (delayed (:pause 1000) (updater-switch-chain-poll ticks))
             ) ;begin
             (set-message "The update will be applied the next time you start the application"
               "Update channel"
             ) ;set-message
           ) ;if
          ) ;
          ((== st 0)
           (updater-switch-dialog-cleanup)
           (set-message "Channel switched; the next release on this channel will be offered"
             "Update channel"
           ) ;set-message
          ) ;
          ;; 失败:关中间态弹窗,再弹阻塞确认提示失败(带错误码)。下载已结束、链路
          ;; 终止,exec 阻塞安全(同 READY 重启确认)。
          ((== st 6) (updater-switch-dialog-cleanup) (updater-notify-failure))
          ;; 检查(1)/应用(5)是进行中状态,不消耗 ticks:ticks 只防御状态异常卡死
          ;; (停在 0/2/4/6 之外且不前进)。
          ((or (== st 1) (== st 5))
           (delayed (:pause 1000) (updater-switch-chain-poll ticks))
          ) ;
          ((< ticks 600) (delayed (:pause 1000) (updater-switch-chain-poll (+ ticks 1))))
          (else (updater-switch-dialog-cleanup)
            (set-message "Timed out waiting for the update check" "Update channel")
          ) ;else
    ) ;cond
  ) ;with
) ;define

;; 启动切换链:checkInBackground 返回 #f 说明已有检查/下载在进行(其快照
;; 可能仍是旧通道),等它结束再重新触发,保证链上的检查用的是新通道快照。
;; 弹窗由 updater-switch-channel 在确认后打开,与链入口无关;超时(等旧任务
;; 超过 600s)说明链路没能进入新通道的检查,此时关弹窗并提示。

(define (updater-switch-chain-start ticks)
  (if (updater-check-background)
    (updater-switch-chain-poll 0)
    (if (< ticks 600)
      (delayed (:pause 1000) (updater-switch-chain-start (+ ticks 1)))
      (begin
        (updater-switch-dialog-cleanup)
        (set-message "Timed out waiting for the previous update task" "Update channel")
      ) ;begin
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
          ;; 确认后立即无条件打开中间态弹窗:第二次切换时 auto-download-loop 可能
          ;; 已抢先触发下载,链 poll 只看到 READY 就开不了窗,固定在此打开最可靠。
          (updater-switch-dialog-open)
          (updater-switch-chain-start 0)
        ) ;when
      ) ;when
    ) ;with
  ) ;when
) ;tm-define
