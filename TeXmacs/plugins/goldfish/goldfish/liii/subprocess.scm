;;
;; Copyright (C) 2024 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;

(define-library (liii subprocess)
  (export run
    run-values
    run-either
    run-set!
    run-get
    run-allow!
    run-ban!
    run-unban!
    run-and
    run-or
    run-sequence
    run-pipe
    run-if
    run-when
  ) ;export
  (import (scheme base)
    (liii base)
    (liii either)
    (liii error)
    (liii hash-table)
    (liii list)
    (liii os)
    (liii path)
    (liii string)
  ) ;import
  (begin

    (define %run-registry (make-hash-table))
    (define %run-allow-list '())
    (define %run-ban-list '(rm))

    (define (%keyword-value key opts default)
      (let loop
        ((rest opts))
        (cond ((null? rest) default)
              ((and (pair? (cdr rest)) (eq? (car rest) key)) (cadr rest))
              ((null? (cdr rest)) default)
              (else (loop (cddr rest)))
        ) ;cond
      ) ;let
    ) ;define

    (define (%check-symbol-command sym)
      (when (memq sym %run-ban-list)
        (value-error (string-append "Symbol command '" (symbol->string sym) "' has been banned")
        ) ;value-error
      ) ;when
      (when (and (not (null? %run-allow-list)) (not (memq sym %run-allow-list)))
        (value-error (string-append "Symbol command '"
                       (symbol->string sym)
                       "' is not in the allow list"
                     ) ;string-append
        ) ;value-error
      ) ;when
    ) ;define

    (define (%check-string-command cmd)
      (let loop
        ((banned %run-ban-list))
        (when (pair? banned)
          (let ((sym-str (symbol->string (car banned))))
            (when (and (>= (string-length cmd) (string-length sym-str))
                    (string=? (substring cmd 0 (string-length sym-str)) sym-str)
                    (or (= (string-length cmd) (string-length sym-str))
                      (char=? (string-ref cmd (string-length sym-str)) #\space)
                    ) ;or
                  ) ;and
              (value-error (string-append "Command '" cmd "' has been banned"))
            ) ;when
          ) ;let
          (loop (cdr banned))
        ) ;when
      ) ;let
    ) ;define

    (define (%resolve-symbol-command sym args)
      (%check-symbol-command sym)
      (let ((val (hash-table-ref/default %run-registry sym #f)))
        (cond ((procedure? val) (cons 'lambda (cons val args)))
              ((string? val) (cons val args))
              ((path? val) (cons (path->string val) args))
              (else (cons (symbol->string sym) args))
        ) ;cond
      ) ;let
    ) ;define

    (define (%string-prefix? str prefix)
      (let ((len-str (string-length str)) (len-pre (string-length prefix)))
        (and (>= len-str len-pre) (string=? (substring str 0 len-pre) prefix))
      ) ;let
    ) ;define

    (define (%check-cwd-conflict command cwd)
      (when cwd
        (let ((has-cd? (cond ((string? command)
                              (or (%string-prefix? command "cd ") (string-contains? command " cd "))
                             ) ;
                             ((and (pair? command) (list? command))
                              (let ((first (car command)))
                                (or (and (string? first) (string=? first "cd")) (eq? first 'cd))
                              ) ;let
                             ) ;
                             (else #f)
                       ) ;cond
              ) ;has-cd?
             ) ;
          (when has-cd?
            (value-error "Cannot set :cwd when command explicitly contains cd")
          ) ;when
        ) ;let
      ) ;when
    ) ;define

    (define (%command->exec-spec command)
      (cond ((string? command) command)
            ((and (pair? command) (symbol? (car command)))
             (%resolve-symbol-command (car command) (cdr command))
            ) ;
            (else (value-error (let ((suggestion (if (and (pair? command) (string? (car command)))
                                                   (format #f "'(~a ...)" (string->symbol (car command)))
                                                   "'(xxx \"yyy\" \"zzz\")"
                                                 ) ;if
                                     ) ;suggestion
                                    ) ;
                                 (format #f
                                   "Command list must start with a symbol, e.g. ~a, got: ~a"
                                   suggestion
                                   (object->string command)
                                 ) ;format
                               ) ;let
                  ) ;value-error
            ) ;else
      ) ;cond
    ) ;define

    (define (run command . opts)
      (let ((cwd (%keyword-value :cwd opts #f))
            (env (%keyword-value :env opts #f))
            (input (%keyword-value :input opts #f))
            (timeout (%keyword-value :timeout opts #f))
            (stdout (%keyword-value :stdout opts #f))
            (stderr (%keyword-value :stderr opts #f))
            (stdin (%keyword-value :stdin opts #f))
            (cmd-spec (%command->exec-spec command))
            (orig-dir #f)
           ) ;
        (%check-cwd-conflict command cwd)
        (when (string? cmd-spec)
          (%check-string-command cmd-spec)
        ) ;when
        (when cwd
          (set! orig-dir (getcwd))
          (chdir cwd)
        ) ;when
        (let ((result (cond ((and (pair? cmd-spec) (eq? (car cmd-spec) 'lambda))
                             (apply (cadr cmd-spec) (cddr cmd-spec))
                             0
                            ) ;
                            ((or env input timeout stdout stderr stdin)
                             (let-values (((out err code)
                                           (run-values command
                                             :cwd
                                             cwd
                                             :env
                                             env
                                             :input
                                             input
                                             :timeout
                                             timeout
                                             :stdout
                                             stdout
                                             :stderr
                                             stderr
                                             :stdin
                                             stdin
                                           ) ;run-values
                                          ) ;
                                         ) ;
                               code
                             ) ;let-values
                            ) ;
                            ((pair? cmd-spec) (let-values (((out err code) (run-values command))) code))
                            (else (os-call cmd-spec))
                      ) ;cond
              ) ;result
             ) ;
          (when orig-dir
            (chdir orig-dir)
          ) ;when
          result
        ) ;let
      ) ;let
    ) ;define

    (define (run-set! symbol value)
      (hash-table-set! %run-registry symbol value)
    ) ;define

    (define (run-get symbol)
      (let ((val (hash-table-ref/default %run-registry symbol #f)))
        (cond ((not val) #f)
              ((string? val) (path val))
              (else val)
        ) ;cond
      ) ;let
    ) ;define

    (define (run-allow! symbol-or-list)
      (set! %run-allow-list
        (cond ((null? symbol-or-list) '())
              ((symbol? symbol-or-list) (list symbol-or-list))
              ((list? symbol-or-list) symbol-or-list)
              (else (type-error "(run-allow! symbol-or-list): must be symbol or list of symbols")
              ) ;else
        ) ;cond
      ) ;set!
    ) ;define

    (define (run-ban! symbol)
      (set! %run-ban-list (cons symbol %run-ban-list))
    ) ;define

    (define (run-unban! symbol)
      (set! %run-ban-list (filter (lambda (x) (not (eq? x symbol))) %run-ban-list))
    ) ;define

    (define (%valid-stdout? val)
      (or (not val)
        (eq? val 'capture)
        (eq? val 'discard)
        (eq? val 'inherit)
        (string? val)
      ) ;or
    ) ;define

    (define (%valid-stderr? val)
      (or (not val)
        (eq? val 'capture)
        (eq? val 'discard)
        (eq? val 'inherit)
        (eq? val 'stdout)
        (string? val)
      ) ;or
    ) ;define

    (define (run-values command . opts)
      (let ((cwd (%keyword-value :cwd opts #f))
            (env (%keyword-value :env opts #f))
            (input (%keyword-value :input opts #f))
            (timeout (%keyword-value :timeout opts #f))
            (stdout (%keyword-value :stdout opts #f))
            (stdout-mode (%keyword-value :stdout-mode opts #f))
            (stderr (%keyword-value :stderr opts #f))
            (stderr-mode (%keyword-value :stderr-mode opts #f))
            (stdin (%keyword-value :stdin opts #f))
            (cmd-spec (%command->exec-spec command))
            (orig-dir #f)
           ) ;
        (%check-cwd-conflict command cwd)
        (when (string? cmd-spec)
          (%check-string-command cmd-spec)
        ) ;when
        (unless (%valid-stdout? stdout)
          (value-error (format #f
                         "Invalid :stdout value: ~a, expected 'capture, 'discard, 'inherit, or string"
                         stdout
                       ) ;format
          ) ;value-error
        ) ;unless
        (unless (%valid-stderr? stderr)
          (value-error (format #f
                         "Invalid :stderr value: ~a, expected 'capture, 'discard, 'inherit, 'stdout, or string"
                         stderr
                       ) ;format
          ) ;value-error
        ) ;unless
        (when cwd
          (set! orig-dir (getcwd))
          (chdir cwd)
        ) ;when
        (let-values (((out err code)
                      (cond ((and (pair? cmd-spec) (eq? (car cmd-spec) 'lambda))
                             (apply (cadr cmd-spec) (cddr cmd-spec))
                             (values "" "" 0)
                            ) ;
                            ((pair? cmd-spec)
                             (g_subprocess-run-values cmd-spec
                               cwd
                               env
                               input
                               timeout
                               stdout
                               stdout-mode
                               stderr
                               stderr-mode
                               stdin
                             ) ;g_subprocess-run-values
                            ) ;
                            (else (g_subprocess-run-values cmd-spec
                                    cwd
                                    env
                                    input
                                    timeout
                                    stdout
                                    stdout-mode
                                    stderr
                                    stderr-mode
                                    stdin
                                  ) ;g_subprocess-run-values
                            ) ;else
                      ) ;cond
                     ) ;
                    ) ;
          (when orig-dir
            (chdir orig-dir)
          ) ;when
          (values out err code)
        ) ;let-values
      ) ;let
    ) ;define

    (define (run-either command . opts)
      (let ((has-stdout? (%keyword-value :stdout opts #f))
            (has-stderr? (%keyword-value :stderr opts #f))
           ) ;
        (let-values (((out err code)
                      (apply run-values
                        command
                        (append (if has-stdout? '() (list :stdout 'capture))
                          (if has-stderr? '() (list :stderr 'capture))
                          opts
                        ) ;append
                      ) ;apply
                     ) ;
                    ) ;
          (if (zero? code) (from-right out) (from-left (cons code err)))
        ) ;let-values
      ) ;let
    ) ;define

    (define (%symbol-keyword? sym)
      (and (symbol? sym)
        (let ((s (symbol->string sym)))
          (and (> (string-length s) 0) (char=? (string-ref s 0) #\:))
        ) ;let
      ) ;and
    ) ;define

    (define (%split-args args)
      (let loop
        ((rev (reverse args)) (opts '()))
        (cond ((null? rev) (values '() opts))
              ((and (pair? (cdr rev)) (%symbol-keyword? (cadr rev)))
               (loop (cddr rev) (append (list (cadr rev) (car rev)) opts))
              ) ;
              (else (values (reverse rev) opts))
        ) ;cond
      ) ;let
    ) ;define

    (define (run-and . args)
      (let-values (((commands opts) (%split-args args)))
        (let ((cwd (%keyword-value :cwd opts #f))
              (env (%keyword-value :env opts #f))
              (timeout (%keyword-value :timeout opts #f))
              (input (%keyword-value :input opts #f))
              (stdin (%keyword-value :stdin opts #f))
              (stdout (%keyword-value :stdout opts #f))
              (stderr (%keyword-value :stderr opts #f))
             ) ;
          (let loop
            ((cmds commands) (first? #t))
            (cond ((null? cmds) (from-right 0))
                  ((null? (cdr cmds))
                   (let-values (((out err code)
                                 (run-values (car cmds)
                                   :cwd
                                   cwd
                                   :env
                                   env
                                   :timeout
                                   timeout
                                   :input
                                   (if first? input #f)
                                   :stdin
                                   (if first? stdin #f)
                                   :stdout
                                   stdout
                                   :stderr
                                   stderr
                                 ) ;run-values
                                ) ;
                               ) ;
                     (if (zero? code) (from-right code) (from-left (list code (car cmds))))
                   ) ;let-values
                  ) ;
                  (else (let-values (((out err code)
                                      (run-values (car cmds)
                                        :cwd
                                        cwd
                                        :env
                                        env
                                        :timeout
                                        timeout
                                        :input
                                        (if first? input #f)
                                        :stdin
                                        (if first? stdin #f)
                                      ) ;run-values
                                     ) ;
                                    ) ;
                          (if (zero? code) (loop (cdr cmds) #f) (from-left (list code (car cmds))))
                        ) ;let-values
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;let-values
    ) ;define

    (define (run-or . args)
      (let-values (((commands opts) (%split-args args)))
        (let ((cwd (%keyword-value :cwd opts #f))
              (env (%keyword-value :env opts #f))
              (timeout (%keyword-value :timeout opts #f))
             ) ;
          (let loop
            ((cmds commands))
            (cond ((null? cmds) (from-right 0))
                  ((null? (cdr cmds))
                   (let-values (((out err code) (run-values (car cmds) :cwd cwd :env env :timeout timeout)))
                     (if (zero? code) (from-right code) (from-left (list code (car cmds))))
                   ) ;let-values
                  ) ;
                  (else (let-values (((out err code) (run-values (car cmds) :cwd cwd :env env :timeout timeout)))
                          (if (zero? code) (from-right 0) (loop (cdr cmds)))
                        ) ;let-values
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;let-values
    ) ;define

    (define (run-sequence . args)
      (let-values (((commands opts) (%split-args args)))
        (let ((cwd (%keyword-value :cwd opts #f))
              (env (%keyword-value :env opts #f))
              (timeout (%keyword-value :timeout opts #f))
             ) ;
          (let loop
            ((cmds commands) (last-code 0))
            (cond ((null? cmds)
                   (if (zero? last-code) (from-right last-code) (from-left last-code))
                  ) ;
                  (else (let-values (((out err code) (run-values (car cmds) :cwd cwd :env env :timeout timeout)))
                          (loop (cdr cmds) code)
                        ) ;let-values
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;let-values
    ) ;define

    (define (run-pipe . args)
      (let-values (((commands opts) (%split-args args)))
        (let ((cwd (%keyword-value :cwd opts #f))
              (env (%keyword-value :env opts #f))
              (timeout (%keyword-value :timeout opts #f))
              (stdin (%keyword-value :stdin opts #f))
             ) ;
          (let loop
            ((cmds commands) (input #f))
            (cond ((null? cmds) (from-right ""))
                  ((null? (cdr cmds))
                   (let-values (((out err code)
                                 (run-values (car cmds)
                                   :cwd
                                   cwd
                                   :env
                                   env
                                   :timeout
                                   timeout
                                   :input
                                   input
                                   :stdin
                                   stdin
                                   :stdout
                                   'capture
                                 ) ;run-values
                                ) ;
                               ) ;
                     (if (zero? code) (from-right out) (from-left (list code (car cmds))))
                   ) ;let-values
                  ) ;
                  (else (let-values (((out err code)
                                      (run-values (car cmds)
                                        :cwd
                                        cwd
                                        :env
                                        env
                                        :timeout
                                        timeout
                                        :input
                                        input
                                        :stdin
                                        stdin
                                        :stdout
                                        'capture
                                      ) ;run-values
                                     ) ;
                                    ) ;
                          (if (zero? code) (loop (cdr cmds) out) (from-left (list code (car cmds))))
                        ) ;let-values
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;let-values
    ) ;define

    (define (run-if condition then-cmd else-cmd)
      (let-values (((out err code) (run-values condition)))
        (if (zero? code)
          (let-values (((out err code) (run-values then-cmd)))
            (if (zero? code) (from-right code) (from-left (list code then-cmd)))
          ) ;let-values
          (let-values (((out err code) (run-values else-cmd)))
            (if (zero? code) (from-right code) (from-left (list code else-cmd)))
          ) ;let-values
        ) ;if
      ) ;let-values
    ) ;define

    (define (run-when condition command)
      (let-values (((out err code) (run-values condition)))
        (if (zero? code)
          (from-right 0)
          (let-values (((out err code) (run-values command)))
            (if (zero? code) (from-right code) (from-left (list code command)))
          ) ;let-values
        ) ;if
      ) ;let-values
    ) ;define

  ) ;begin
) ;define-library
