;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6238.scm
;; DESCRIPTION : 回归测试：渐进式排版分块状态机一致性（临时文档行重叠）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [6238] 新建临时（草稿）文档中连续排版时出现内容重复叠加（用户表现为
;;   行重叠，保存后重新打开恢复正常）。根因有二：
;;     1. typesetter_rep::typeset 在两次排版之间既无 progressive_advance
;;        也无 progressive_reset 时，分块边界陈旧：chunk_start 停留在旧值，
;;        doc_l 被重置为 [0,·) 后与已冻结页面重复叠加；
;;     2. 渐进排版完成后（空分块）根 bridge 缓存命中会重播上一分块的
;;        page items，若并入 doc_l 会把已冻结内容再次追加成新页。
;;   本测试用 70 段 paper/A4 文档（触发渐进路径）连续多次 update-forced，
;;   断言：
;;     a. 前沿单调推进至文末，pending 归假；
;;     b. 追加的 update-forced 不再膨胀页数/总高（修复前 5→7→9 递增）；
;;     c. 交互排版终态与「保存后重新打开」的排版结果完全一致。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 6238
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/utils/library/cursor.scm")

(define tm6238-npara 70)

(define (tm6238-para i)
  (string-append "第"
    (number->string i)
    "段：本实验利用电容充电模型和非线性最小二乘法，根据八组电压观测数据估计两个未知参数，"
    "拟合结果的决定系数为0.9868，进一步利用估计参数预测电压首次达到9.9V的时刻，"
    "四次预测结果分别为13.23秒、13.31秒、13.42秒与13.54秒，最大值与最小值相差0.31秒。"
  ) ;string-append
) ;define

(define (tm6238-pages)
  (get-page-count)
) ;define

(define (tm6238-height)
  (get-total-height #t)
) ;define

(tm-define (test_6238)
  (check-set-mode! 'report-failed)

  ;; 1. 新建草稿并逐段输入 70 段，paper/A4 介质触发渐进式排版（n >= 60）
  (define buf (new-buffer))
  (with-buffer buf
    (init-env "page-medium" "paper")
    (init-page-type "a4")
    (insert (tm6238-para 1))
    (let loop
      ((i 2))
      (when (<= i tm6238-npara)
        (insert-raw-return)
        (insert (tm6238-para i))
        (loop (+ i 1))
      ) ;when
    ) ;let
  ) ;with-buffer

  ;; 2. 首轮排版：只完成第一个分块，pending 为真
  (with-buffer buf (update-forced))
  (check (with-buffer buf (typeset-in-progress?)) => #t)

  ;; 3. 续排至文末：前沿推进，pending 归假
  (with-buffer buf (update-forced))
  (check (with-buffer buf (typeset-frontier)) => tm6238-npara)
  (check (with-buffer buf (typeset-in-progress?)) => #f)
  (define pages-final (with-buffer buf (tm6238-pages)))
  (define height-final (with-buffer buf (tm6238-height)))
  (check (> pages-final 0) => #t)

  ;; 4. 追加多次强制排版（无 advance / 无 reset 的重复 typeset）：
  ;;    修复前每轮都会把已冻结内容重复追加，页数 5→7→9 递增
  (with-buffer buf (update-forced))
  (with-buffer buf (update-forced))
  (check (with-buffer buf (tm6238-pages)) => pages-final)
  (check (with-buffer buf (tm6238-height)) => height-final)

  ;; 5. 保存后重新打开，排版补全后与交互终态完全一致
  (with-buffer buf (buffer-save buf))
  (cpp-buffer-close buf)
  (load-buffer buf)
  (with-buffer buf (update-forced))
  (with-buffer buf (update-forced))
  (check (with-buffer buf (typeset-in-progress?)) => #f)
  (check (with-buffer buf (tm6238-pages)) => pages-final)
  (check (with-buffer buf (tm6238-height)) => height-final)

  ;; 6. 清理测试产生的草稿文件
  (cpp-buffer-close buf)
  (when (url-exists? buf)
    (system-remove buf)
  ) ;when

  (check-report)
) ;tm-define
