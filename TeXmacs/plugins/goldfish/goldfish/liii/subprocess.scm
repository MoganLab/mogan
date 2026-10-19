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
  (export run run-either run-set! run-get run-allow! run-ban! run-unban! run-and
    run-or run-sequence run-pipe run-if run-when process? process-pid
    process-spawn process-alive? process-poll process-wait process-terminate
    process-kill process-kill-tree process-close process-close-stdin
    process-write process-write-stdin process-read process-read-line
    run-streaming process-last-activity-time process-idle-seconds
    make-process-error process-error? process-error-code process-error-stderr
    process-error-stdout process-error-command process-error-message
  ) ;export
  (import (scheme base)
    (scheme time)
    (liii base)
    (liii either)
    (liii error)
    (liii hash-table)
    (liii list)
    (liii os)
    (liii path)
    (liii string)
    (liii subprocess process-error)
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
            (when
              (and (>= (string-length cmd) (string-length sym-str))
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
        (let ((has-cd?
                (cond ((string? command)
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
            ((not (proper-list? command))
             (type-error (format #f
                           "Command must be a proper list or a string, got: ~a"
                           (object->string command)
                         ) ;format
             ) ;type-error
            ) ;
            ((and (pair? command) (symbol? (car command)))
             (%resolve-symbol-command (car command) (cdr command))
            ) ;
            ((and (pair? command) (string? (car command)))
             (%resolve-symbol-command (string->symbol (car command)) (cdr command))
            ) ;
            (else
              (value-error
                (let ((suggestion
                        (if (and (pair? command) (string? (car command)))
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
        (let ((result
                (cond
                 ((and (pair? cmd-spec) (eq? (car cmd-spec) 'lambda))
                  (apply (cadr cmd-spec) (cddr cmd-spec))
                  0
                 ) ;
                 ((or (pair? cmd-spec) env input timeout stdout stderr stdin)
                  (let-values (((out err code)
                                (%run-values command
                                  :cwd     cwd
                                  :env     env
                                  :input   input
                                  :timeout timeout
                                  :stdout  (or stdout 'inherit)
                                  :stderr  (or stderr 'inherit)
                                  :stdin   stdin
                                ) ;%run-values
                               ) ;
                              ) ;
                    code
                  ) ;let-values
                 ) ;
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

    (define (%valid-env? env)
      (or (not env)
        (and (proper-list? env)
          (every
            (lambda (x) (and (pair? x) (string? (car x)) (string? (cdr x))))
            env
          ) ;every
        ) ;and
      ) ;or
    ) ;define

    (define (%normalize-stream-spec caller kw val allowed-symbols default)
      (cond ((not val) default)
            ((path? val) (path->string val))
            ((string? val) val)
            ((symbol? val)
             (unless (memq val allowed-symbols)
               (value-error (format #f
                              "Invalid ~a value: ~a, expected one of ~a or a string/path"
                              kw val allowed-symbols
                            ) ;format
               ) ;value-error
             ) ;unless
             val
            ) ;
            (else (type-error (format #f
                                "~a: ~a must be a symbol, string, or path, got: ~a"
                                caller
                                kw
                                (object->string val)
                              ) ;format
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define (%normalize-cwd caller cwd)
      (cond ((not cwd) #f)
            ((path? cwd) (path->string cwd))
            ((string? cwd) cwd)
            (else (type-error (format #f "~a: :cwd must be a string or path" caller)))
      ) ;cond
    ) ;define

    (define (%run-values command . opts)
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
        (set! cwd (%normalize-cwd "run" cwd))
        (set! stdout
          (%normalize-stream-spec "run"
            :stdout stdout
            '(capture discard inherit tee)
            'capture
          ) ;%normalize-stream-spec
        ) ;set!
        (set! stderr
          (%normalize-stream-spec "run"
            :stderr stderr
            '(capture discard inherit tee stdout)
            'capture
          ) ;%normalize-stream-spec
        ) ;set!
        (set! stdin
          (%normalize-stream-spec "run" :stdin stdin '(inherit discard pipe null) #f)
        ) ;set!
        (unless (%valid-env? env)
          (type-error "run-values: :env must be a list of (key . value) string pairs")
        ) ;unless
        (when cwd
          (set! orig-dir (getcwd))
          (chdir cwd)
        ) ;when
        (let-values (((out err code)
                      (cond
                       ((and (pair? cmd-spec) (eq? (car cmd-spec) 'lambda))
                        (apply (cadr cmd-spec) (cddr cmd-spec))
                        (values "" "" 0)
                       ) ;
                       ((pair? cmd-spec)
                        (g_subprocess-run-values cmd-spec cwd env input timeout
                          stdout stdout-mode stderr stderr-mode stdin
                        ) ;g_subprocess-run-values
                       ) ;
                       (else (g_subprocess-run-values cmd-spec cwd env input
                               timeout stdout stdout-mode stderr stderr-mode
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
      (let-values (((out err code) (apply %run-values command opts)))
        (if (zero? code)
          (from-right out)
          (from-left (make-process-error code err out command))
        ) ;if
      ) ;let-values
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
                   (let ((res (run-either (car cmds)
                                :cwd     cwd
                                :env     env
                                :timeout timeout
                                :input   (if first? input #f)
                                :stdin   (if first? stdin #f)
                                :stdout  (or stdout 'inherit)
                                :stderr  (or stderr 'inherit)
                              ) ;run-either
                         ) ;res
                        ) ;
                     (either-map (lambda (_) 0) res)
                   ) ;let
                  ) ;
                  (else
                    (let ((res (run-either (car cmds)
                                 :cwd     cwd
                                 :env     env
                                 :timeout timeout
                                 :input   (if first? input #f)
                                 :stdin   (if first? stdin #f)
                                 :stdout  'inherit
                                 :stderr  'inherit
                               ) ;run-either
                          ) ;res
                         ) ;
                      (if (either-right? res) (loop (cdr cmds) #f) res)
                    ) ;let
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
                  (else
                    (let ((res (run-either (car cmds)
                                 :cwd     cwd
                                 :env     env
                                 :timeout timeout
                                 :stdout  'inherit
                                 :stderr  'inherit
                               ) ;run-either
                          ) ;res
                         ) ;
                      (cond ((either-right? res) (from-right 0))
                            ((null? (cdr cmds)) res)
                            (else (loop (cdr cmds)))
                      ) ;cond
                    ) ;let
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
            ((cmds commands) (last-res (from-right 0)))
            (if (null? cmds)
              last-res
              (let ((res (run-either (car cmds)
                           :cwd     cwd
                           :env     env
                           :timeout timeout
                           :stdout  'inherit
                           :stderr  'inherit
                         ) ;run-either
                    ) ;res
                   ) ;
                (loop (cdr cmds) (either-map (lambda (_) 0) res))
              ) ;let
            ) ;if
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
                   (run-either (car cmds)
                     :cwd     cwd
                     :env     env
                     :timeout timeout
                     :input   input
                     :stdin   stdin
                     :stdout  'capture
                   ) ;run-either
                  ) ;
                  (else
                    (let ((res (run-either (car cmds)
                                 :cwd     cwd
                                 :env     env
                                 :timeout timeout
                                 :input   input
                                 :stdin   stdin
                                 :stdout  'capture
                               ) ;run-either
                          ) ;res
                         ) ;
                      (if (either-right? res) (loop (cdr cmds) (to-right res)) res)
                    ) ;let
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;let-values
    ) ;define

    (define (run-if condition then-cmd else-cmd)
      (let ((chosen-cmd (if (zero? (run condition :stdout 'inherit :stderr 'inherit)) then-cmd else-cmd)
            ) ;chosen-cmd
           ) ;
        (let ((res (run-either chosen-cmd :stdout 'inherit :stderr 'inherit)))
          (either-map (lambda (_) 0) res)
        ) ;let
      ) ;let
    ) ;define

    (define (run-when condition command)
      (if (zero? (run condition :stdout 'inherit :stderr 'inherit))
        (from-right 0)
        (let ((res (run-either command :stdout 'inherit :stderr 'inherit)))
          (either-map (lambda (_) 0) res)
        ) ;let
      ) ;if
    ) ;define

    (define (process? obj)
      (g_process? obj)
    ) ;define

    (define (process-pid proc)
      (unless (process? proc)
        (type-error "process-pid: argument must be a process")
      ) ;unless
      (g_subprocess-pid proc)
    ) ;define

    (define (process-alive? proc)
      (unless (process? proc)
        (type-error "process-alive?: argument must be a process")
      ) ;unless
      (g_subprocess-alive? proc)
    ) ;define

    (define (process-poll proc)
      (unless (process? proc)
        (type-error "process-poll: argument must be a process")
      ) ;unless
      (g_subprocess-poll proc)
    ) ;define

    (define (process-wait proc . args)
      (unless (process? proc)
        (type-error "process-wait: first argument must be a process")
      ) ;unless
      (let ((timeout-ms (if (pair? args) (car args) -1)))
        (unless (integer? timeout-ms)
          (type-error "process-wait: timeout must be an integer")
        ) ;unless
        (g_subprocess-wait proc timeout-ms)
      ) ;let
    ) ;define

    (define (process-terminate proc)
      (unless (process? proc)
        (type-error "process-terminate: argument must be a process")
      ) ;unless
      (g_subprocess-terminate proc)
    ) ;define

    (define (process-kill proc)
      (unless (process? proc)
        (type-error "process-kill: argument must be a process")
      ) ;unless
      (g_subprocess-kill proc)
    ) ;define

    (define (process-kill-tree proc-or-pid . opts)
      (unless (or (process? proc-or-pid) (and (integer? proc-or-pid) (exact? proc-or-pid)))
        (type-error "process-kill-tree: argument must be a process or exact integer")
      ) ;unless
      (when (and (integer? proc-or-pid) (not (process? proc-or-pid)))
        (unless (> proc-or-pid 0)
          (value-error "process-kill-tree: pid must be a positive integer")
        ) ;unless
      ) ;when
      (let ((signal (%keyword-value :signal opts 'sigkill))
            (timeout (%keyword-value :timeout opts 3))
           ) ;
        (unless (symbol? signal)
          (type-error "process-kill-tree: :signal must be a symbol")
        ) ;unless
        (unless (or (eq? signal 'sigkill) (eq? signal 'sigterm))
          (value-error "process-kill-tree: :signal must be 'sigkill or 'sigterm")
        ) ;unless
        (unless (real? timeout)
          (type-error "process-kill-tree: :timeout must be a real number")
        ) ;unless
        (when (< timeout 0)
          (value-error "process-kill-tree: :timeout must be non-negative")
        ) ;when
        (if (and (process? proc-or-pid) (not (process-alive? proc-or-pid)))
          #t
          (let ((timeout-ms (exact (round (* timeout 1000)))))
            (g_subprocess-kill-tree proc-or-pid signal timeout-ms)
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (process-close proc)
      (unless (process? proc)
        (type-error "process-close: argument must be a process")
      ) ;unless
      (g_subprocess-close proc)
    ) ;define

    (define (process-close-stdin proc)
      (unless (process? proc)
        (type-error "process-close-stdin: argument must be a process")
      ) ;unless
      (g_subprocess-close-stdin proc)
    ) ;define

    (define (process-last-activity-time proc)
      (unless (process? proc)
        (type-error "process-last-activity-time: argument must be a process")
      ) ;unless
      (g_subprocess-last-activity-time proc)
    ) ;define

    (define (process-idle-seconds proc)
      (unless (process? proc)
        (type-error "process-idle-seconds: argument must be a process")
      ) ;unless
      (g_subprocess-idle-seconds proc)
    ) ;define

    (define (process-write-stdin proc data)
      (unless (process? proc)
        (type-error "process-write-stdin: first argument must be a process")
      ) ;unless
      (unless (or (string? data) (bytevector? data))
        (type-error "process-write-stdin: second argument must be a string or bytevector"
        ) ;type-error
      ) ;unless
      (g_subprocess-write proc data)
    ) ;define

    (define process-write process-write-stdin)

    (define (process-read proc . opts)
      (unless (process? proc)
        (type-error "process-read: first argument must be a process")
      ) ;unless
      (let ((max-bytes (%keyword-value :max-bytes opts 65536))
            (stream (%keyword-value :stream opts 'stdout))
           ) ;
        (unless (integer? max-bytes)
          (type-error "process-read: :max-bytes must be an integer")
        ) ;unless
        (unless (> max-bytes 0)
          (value-error "process-read: :max-bytes must be a positive integer")
        ) ;unless
        (unless (symbol? stream)
          (type-error "process-read: :stream must be a symbol")
        ) ;unless
        (unless (or (eq? stream 'stdout) (eq? stream 'stderr))
          (value-error "process-read: :stream must be 'stdout or 'stderr")
        ) ;unless
        (g_subprocess-read proc max-bytes stream)
      ) ;let
    ) ;define

    (define (process-read-line proc . opts)
      (unless (process? proc)
        (type-error "process-read-line: first argument must be a process")
      ) ;unless
      (let ((stream (%keyword-value :stream opts 'stdout)))
        (unless (symbol? stream)
          (type-error "process-read-line: :stream must be a symbol")
        ) ;unless
        (unless (or (eq? stream 'stdout) (eq? stream 'stderr))
          (value-error "process-read-line: :stream must be 'stdout or 'stderr")
        ) ;unless
        (g_subprocess-read-line proc stream)
      ) ;let
    ) ;define

    (define (%normalize-cmd-arg arg)
      (cond ((string? arg) arg)
            ((symbol? arg) (symbol->string arg))
            ((path? arg) (path->string arg))
            (else (type-error (format #f
                                "process-spawn: command element must be a string, symbol, or path, got: ~a"
                                (object->string arg)
                              ) ;format
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define (%normalize-process-command command)
      (cond ((string? command) command)
            ((proper-list? command)
             (if (null? command)
               (value-error "process-spawn: command list cannot be empty")
               (map %normalize-cmd-arg command)
             ) ;if
            ) ;
            (else (type-error (format #f
                                "process-spawn: command must be a string or a proper list, got: ~a"
                                (object->string command)
                              ) ;format
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define (process-spawn command . opts)
      (let ((cwd (%keyword-value :cwd opts #f))
            (env (%keyword-value :env opts #f))
            (stdin (%keyword-value :stdin opts #f))
            (stdout (%keyword-value :stdout opts #f))
            (stderr (%keyword-value :stderr opts #f))
            (norm-cmd (%normalize-process-command command))
           ) ;
        (set! cwd (%normalize-cwd "process-spawn" cwd))
        (when env
          (unless (%valid-env? env)
            (type-error "process-spawn: :env must be a list of (key . value) string pairs")
          ) ;unless
        ) ;when
        (let ((norm-stdin (%normalize-stream-spec "process-spawn" :stdin stdin '(pipe null
                                                                                  inherit) #f)
              ) ;norm-stdin
              (norm-stdout (%normalize-stream-spec "process-spawn"
                             :stdout stdout
                             '(pipe capture discard inherit)
                             #f
                           ) ;%normalize-stream-spec
              ) ;norm-stdout
              (norm-stderr (%normalize-stream-spec "process-spawn"
                             :stderr stderr
                             '(pipe capture discard inherit stdout)
                             #f
                           ) ;%normalize-stream-spec
              ) ;norm-stderr
             ) ;
          (g_subprocess-spawn norm-cmd cwd env norm-stdin norm-stdout norm-stderr)
        ) ;let
      ) ;let
    ) ;define

    (define (%current-time-ms)
      (* 1000.0 (/ (current-jiffy) (jiffies-per-second)))
    ) ;define

    (define (%invoke-on-idle on-idle proc idle-secs)
      (let ((ar (car (arity on-idle))))
        (if (= ar 1) (on-idle idle-secs) (on-idle proc idle-secs))
      ) ;let
    ) ;define

    (define (run-streaming command . opts)
      (let ((cwd (%keyword-value :cwd opts #f))
            (env (%keyword-value :env opts #f))
            (on-stdout (%keyword-value :on-stdout opts #f))
            (on-stderr (%keyword-value :on-stderr opts #f))
            (timeout (%keyword-value :timeout opts #f))
            (idle-timeout (%keyword-value :idle-timeout opts #f))
            (on-idle (%keyword-value :on-idle opts #f))
           ) ;
        (when (and on-stdout (not (procedure? on-stdout)))
          (type-error "run-streaming: :on-stdout must be a procedure")
        ) ;when
        (when (and on-stderr (not (procedure? on-stderr)))
          (type-error "run-streaming: :on-stderr must be a procedure")
        ) ;when
        (when (and on-idle (not (procedure? on-idle)))
          (type-error "run-streaming: :on-idle must be a procedure")
        ) ;when
        (when (and timeout (not (real? timeout)))
          (type-error "run-streaming: :timeout must be a real number")
        ) ;when
        (when (and timeout (<= timeout 0))
          (value-error "run-streaming: :timeout must be a positive number")
        ) ;when
        (when (and idle-timeout (not (real? idle-timeout)))
          (type-error "run-streaming: :idle-timeout must be a real number")
        ) ;when
        (when (and idle-timeout (<= idle-timeout 0))
          (value-error "run-streaming: :idle-timeout must be a positive number")
        ) ;when
        (when (and timeout idle-timeout (> idle-timeout timeout))
          (value-error "run-streaming: :idle-timeout cannot be greater than :timeout")
        ) ;when
        (let* ((capture-out? (or on-stdout idle-timeout on-idle))
               (capture-err? (or on-stderr idle-timeout on-idle))
               (proc (process-spawn command
                       :cwd    cwd
                       :env    env
                       :stdout (if capture-out? 'pipe 'inherit)
                       :stderr (if capture-err? 'pipe 'inherit)
                     ) ;process-spawn
               ) ;proc
              ) ;
          (let* ((timeout-ms
                   (if timeout (inexact->exact (round (* timeout 1000))) #f)
                 ) ;timeout-ms
                 (deadline-ms (if timeout-ms (+ (%current-time-ms) timeout-ms) #f))
                ) ;
            (define (%rem-ms)
              (if (not deadline-ms)
                -1
                (let ((rem (inexact->exact (round (- deadline-ms (%current-time-ms))))))
                  (if (<= rem 0) 0 rem)
                ) ;let
              ) ;if
            ) ;define

            (define (%idle-rem-ms idle-secs)
              (if (not idle-timeout)
                -1
                (let ((rem-ms (inexact->exact (round (* (- idle-timeout idle-secs) 1000)))))
                  (if (<= rem-ms 0) 0 rem-ms)
                ) ;let
              ) ;if
            ) ;define

            (cond
             ((and (not capture-out?) (not capture-err?))
              (let ((code (process-wait proc (or timeout-ms -1))))
                (process-close proc)
                code
              ) ;let
             ) ;
             (else
               (let loop
                 ()
                 (let* ((rem-ms (%rem-ms))
                        (idle-secs (and (or idle-timeout on-idle) (g_subprocess-idle-seconds proc)))
                        (idle-rem-ms (%idle-rem-ms idle-secs))
                       ) ;
                   (cond
                    ((or (and deadline-ms (<= rem-ms 0)) (and idle-timeout (<= idle-rem-ms 0)))
                     (process-kill-tree proc)
                     (process-close proc)
                     (process-wait proc 500)
                     #f
                    ) ;
                    (else
                      (let* ((poll-timeout
                               (max 1
                                 (min 50 (if (>= rem-ms 0) rem-ms 50) (if (>= idle-rem-ms 0) idle-rem-ms 50))
                               ) ;max
                             ) ;poll-timeout
                             (ev (g_subprocess-poll-io proc poll-timeout))
                            ) ;
                        (cond
                         ((eq? ev 'stdout)
                          (let ((line (process-read-line proc :stream 'stdout)))
                            (when (and on-stdout (not (eof-object? line)))
                              (on-stdout line)
                            ) ;when
                          ) ;let
                          (loop)
                         ) ;
                         ((eq? ev 'stderr)
                          (let ((line (process-read-line proc :stream 'stderr)))
                            (when (and on-stderr (not (eof-object? line)))
                              (on-stderr line)
                            ) ;when
                          ) ;let
                          (loop)
                         ) ;
                         ((eq? ev 'both)
                          (let ((line-out (process-read-line proc :stream 'stdout)))
                            (when (and on-stdout (not (eof-object? line-out)))
                              (on-stdout line-out)
                            ) ;when
                          ) ;let
                          (let ((line-err (process-read-line proc :stream 'stderr)))
                            (when (and on-stderr (not (eof-object? line-err)))
                              (on-stderr line-err)
                            ) ;when
                          ) ;let
                          (loop)
                         ) ;
                         ((eq? ev 'eof)
                          (when on-stdout
                            (let drain
                              ()
                              (let ((line (process-read-line proc :stream 'stdout)))
                                (when (not (eof-object? line))
                                  (on-stdout line)
                                  (drain)
                                ) ;when
                              ) ;let
                            ) ;let
                          ) ;when
                          (when on-stderr
                            (let drain
                              ()
                              (let ((line (process-read-line proc :stream 'stderr)))
                                (when (not (eof-object? line))
                                  (on-stderr line)
                                  (drain)
                                ) ;when
                              ) ;let
                            ) ;let
                          ) ;when
                          (process-close proc)
                          (process-wait proc (%rem-ms))
                         ) ;
                         (else
                           (if (and (not (process-alive? proc))
                                 (g_subprocess-eof? proc 'stdout)
                                 (g_subprocess-eof? proc 'stderr)
                               ) ;and
                             (begin
                               (when on-stdout
                                 (let drain
                                   ()
                                   (let ((line (process-read-line proc :stream 'stdout)))
                                     (when (not (eof-object? line))
                                       (on-stdout line)
                                       (drain)
                                     ) ;when
                                   ) ;let
                                 ) ;let
                               ) ;when
                               (when on-stderr
                                 (let drain
                                   ()
                                   (let ((line (process-read-line proc :stream 'stderr)))
                                     (when (not (eof-object? line))
                                       (on-stderr line)
                                       (drain)
                                     ) ;when
                                   ) ;let
                                 ) ;let
                               ) ;when
                               (process-close proc)
                               (process-wait proc (%rem-ms))
                             ) ;begin
                             (begin
                               (when on-idle
                                 (%invoke-on-idle on-idle proc idle-secs)
                               ) ;when
                               (loop)
                             ) ;begin
                           ) ;if
                         ) ;else
                        ) ;cond
                      ) ;let*
                    ) ;else
                   ) ;cond
                 ) ;let*
               ) ;let
             ) ;else
            ) ;cond
          ) ;let*
        ) ;let*
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
