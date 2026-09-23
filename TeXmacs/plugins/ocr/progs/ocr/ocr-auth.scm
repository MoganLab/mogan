;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ocr-auth.scm
;; DESCRIPTION : OCR 云识别的认证头与站点 provider
;; COPYRIGHT   : (C) 2026  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ocr ocr-auth) (:use (account liii)))

;; account/stem 系列符号属 GPL 层，goldfish 的 (liii ocr-network) 不可直接
;; 调用（协议隔离），故在本模块构造认证头与站点，经 set-ocr-auth-provider! 注入。

(tm-define (ocr-auth-headers)
  (let ((preview-cookie (stem-preview-cookie-header)))
    (append `((,"Authorization"
               . ,(string-append "Bearer " (account-load-token)))
              (,"Content-Type" . ,"application/json")
              (,"User-Agent" . ,(stem-user-agent))
              (,"X-Device-Id" . ,(stem-device-id)))
      (if (string-null? preview-cookie) '() (list (cons "Cookie" preview-cookie)))
    ) ;append
  ) ;let
) ;tm-define

(tm-define (ocr-auth-site) (current-stem-site))
