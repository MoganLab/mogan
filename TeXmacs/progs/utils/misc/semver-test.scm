;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : semver-test.scm
;; DESCRIPTION : SemVer 2.0.0 规范解析与比对测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r semver-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/utils/misc/semver.scm")

(define (test-semver-clean)
  (check (semver-clean "2026.3.5") => "2026.3.5")
  (check (semver-clean "v2026.3.5") => "2026.3.5")
  (check (semver-clean "V2026.3.5") => "2026.3.5")
  (check (semver-clean "  v2026.3.5  ") => "2026.3.5")
  (check (semver-clean "\n\tv1.0.0-rc.1\r\n") => "1.0.0-rc.1")
  (check (semver-clean "valid") => "valid")
  (check (semver-clean "") => "")
  (check (semver-clean #f) => "")
) ;define

(define (test-semver-valid)
  (check (semver-valid? "2026.3.5") => #t)
  (check (semver-valid? "v2026.3.5") => #t)
  (check (semver-valid? "2026.3.2-rc.1") => #t)
  (check (semver-valid? "2026.3.2-rc.1+build.123") => #t)
  (check (semver-valid? "1.0.0-alpha") => #t)
  (check (semver-valid? "1.0.0-alpha.1") => #t)
  (check (semver-valid? "1.0.0-0.3.7") => #t)
  (check (semver-valid? "1.0.0-x.7.z.92") => #t)
  ;; 非法格式
  (check (semver-valid? "") => #f)
  (check (semver-valid? "abc") => #f)
  (check (semver-valid? "1.0.0-") => #f)
  (check (semver-valid? "1.0.0-rc.01") => #f)
  ;; 纯数字标识符不得含前导零
  (check (semver-valid? "01.2.3") => #f)
  ;; 核心段不得含前导零
) ;define

(define (test-semver-comparison-user-case)
  ;; 验证 2026.3.2-rc.1 绝不比 2026.3.5 新
  (check (semver<? "2026.3.2-rc.1" "2026.3.5") => #t)
  (check (semver>? "2026.3.2-rc.1" "2026.3.5") => #f)
  (check (semver>? "2026.3.5" "2026.3.2-rc.1") => #t)
  (check (semver<=? "2026.3.2-rc.1" "2026.3.5") => #t)
  (check (semver>=? "2026.3.5" "2026.3.2-rc.1") => #t)
) ;define

(define (test-semver-spec-examples)
  ;; SemVer 2.0.0 规范第 11 条示例链条：
  ;; 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0
  (check (semver<? "1.0.0-alpha" "1.0.0-alpha.1") => #t)
  (check (semver<? "1.0.0-alpha.1" "1.0.0-alpha.beta") => #t)
  (check (semver<? "1.0.0-alpha.beta" "1.0.0-beta") => #t)
  (check (semver<? "1.0.0-beta" "1.0.0-beta.2") => #t)
  (check (semver<? "1.0.0-beta.2" "1.0.0-beta.11") => #t)
  (check (semver<? "1.0.0-beta.11" "1.0.0-rc.1") => #t)
  (check (semver<? "1.0.0-rc.1" "1.0.0") => #t)

  ;; 核心段比较
  (check (semver<? "1.0.0" "2.0.0") => #t)
  (check (semver<? "2.0.0" "2.1.0") => #t)
  (check (semver<? "2.1.0" "2.1.1") => #t)
  (check (semver>? "2.1.1" "2.1.0") => #t)

  ;; 预发布纯数字标识符按数值大小比较（防止 rc.10 与 rc.2 字典序倒挂）
  (check (semver>? "2026.3.2-rc.10" "2026.3.2-rc.2") => #t)
  (check (semver<? "2026.3.2-rc.2" "2026.3.2-rc.10") => #t)

  ;; 构建元数据（Build metadata）在比对时不影响优先级
  (check (semver=? "1.0.0+20130313144700" "1.0.0+exp.sha.5114f85") => #t)
  (check (semver=? "2026.3.5+001" "2026.3.5+002") => #t)

  ;; 带前导 'v' 归一后相等
  (check (semver=? "v2026.3.5" "2026.3.5") => #t)
  (check (semver=? "V2026.3.5" "v2026.3.5") => #t)

  ;; 相同版本
  (check (semver=? "2026.3.5" "2026.3.5") => #t)
  (check (semver>=? "2026.3.5" "2026.3.5") => #t)
  (check (semver<=? "2026.3.5" "2026.3.5") => #t)
) ;define

(tm-define (regtest-semver)
  (test-semver-clean)
  (test-semver-valid)
  (test-semver-comparison-user-case)
  (test-semver-spec-examples)
  (check-report)
) ;tm-define
