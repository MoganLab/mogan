
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : autosave.scm
;; DESCRIPTION : (liii autosave) library: backup path policy and retention
;; COPYRIGHT   : (C) 2026 Mogan Developers
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (liii autosave)
  (export autosave-keep-max
    autosave-home
    autosave-dir
    autosave-target-path
    ensure-parent-dir
    autosave-prune-dir
    document->string
  ) ;export
  (import (liii base)
    (liii error)
    (liii list)
    (liii os)
    (liii path)
    (liii sort)
    (liii string)
    (srfi srfi-13)
    (srfi srfi-19)
    (scheme base)
  ) ;import
  (begin

    ;; ; 每个 doc-id 目录最多保留的备份份数
    (define autosave-keep-max 10)

    ;; ; 主目录:取自环境变量 TEXMACS_HOME_PATH
    (define (autosave-home)
      (path-from-env "TEXMACS_HOME_PATH")
    ) ;define

    ;; ; 某文档对应的备份目录:$HOME/system/backup/<doc-id>
    (define (autosave-dir doc-id)
      (path-join (autosave-home) "system" "backup" doc-id)
    ) ;define

    ;; ; 单次备份的目标文件路径:<dir>/<yyyymmdd>_<hhmmss>.tmu
    (define (autosave-target-path doc-id)
      (let* ((date (current-date)) (stamp (date->string date "~Y~m~d_~H~M~S")))
        (path-join (autosave-dir doc-id) (string-append stamp ".tmu"))
      ) ;let*
    ) ;define

    ;; ; 确保从 home 到 doc-id 目录的目录链全部存在
    ;; ; 返回 #t 表示所有目录都已就绪,#f 表示失败
    (define (ensure-dir path)
      (when (not (path-exists? path))
        (mkdir (path->string path))
      ) ;when
      (and (path-exists? path) (path-dir? path))
    ) ;define

    (define (ensure-parent-dir doc-id)
      (let ((home (autosave-home))
            (system-dir (path-join (autosave-home) "system"))
            (backup-dir (path-join (autosave-home) "system" "backup"))
            (doc-dir (autosave-dir doc-id))
           ) ;
        (and home
          (ensure-dir home)
          (ensure-dir system-dir)
          (ensure-dir backup-dir)
          (ensure-dir doc-dir)
        ) ;and
      ) ;let
    ) ;define

    ;; ; 从 (document "...") stree 提取 JSON 字符串;若已是字符串则直接返回
    (define (document->string doc)
      (cond ((and (pair? doc) (eq? (car doc) 'document) (= (length doc) 2)) (cadr doc))
            ((string? doc) doc)
            (else "")
      ) ;cond
    ) ;define

    ;; ; 把目录内 .tmu 裁剪到 < autosave-keep-max 份
    ;; ; 按文件名字典序删最老的(时间戳文件名天然单调)
    ;; ; 目录不存在时直接返回 #t(无操作)
    ;; ; 返回 #t;异常向上传递
    (define (autosave-prune-dir dir)
      (if (not (path-exists? dir))
        #t
        (let* ((entries (vector->list (path-list dir)))
               (tmus (filter (lambda (n) (string-suffix? ".tmu" n)) entries))
               (excess (max 0 (- (length tmus) (- autosave-keep-max 1))))
              ) ;
          (when (> excess 0)
            (for-each (lambda (n) (path-unlink (path-join dir n) #t))
              (take (list-sort string<? tmus) excess)
            ) ;for-each
          ) ;when
          #t
        ) ;let*
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
