;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
;; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

(define-library (liii gfproject)
  (import (scheme base)
    (scheme write)
    (liii base)
    (liii json)
    (liii list)
    (liii os)
    (liii path)
  ) ;import
  (export gfproject-get-gf-lib gfproject-find-lib-path gfproject-find-local-path
    gfproject-extract-tools gfproject-deep-merge gfproject-load-config-bundle
    gfproject-load-config gfproject-find-tool-root gfproject-resolve-tool
    gfproject-resolve-tool-bundle gfproject-prepare-and-run-tool
    gfproject-run-tool
  ) ;export

  (begin

    (define (gfproject-get-gf-lib)
      (g_goldfish-library-dir)
    ) ;define

    (define (gfproject--opt-gf-lib opt-gf-lib)
      (if (and (pair? opt-gf-lib) (string? (car opt-gf-lib)))
        (car opt-gf-lib)
        (gfproject-get-gf-lib)
      ) ;if
    ) ;define

    (define (bundle-ref bundle key)
      (cdr (assoc key bundle))
    ) ;define

    (define (gfproject-find-local-path)
      (let ((root (g_project-root)))
        (and root (path->string (path-join root "gfproject.json")))
      ) ;let
    ) ;define

    (define (gfproject-find-lib-path . opt-gf-lib)
      (let* ((gf-lib (gfproject--opt-gf-lib opt-gf-lib))
             (p1 (path-join gf-lib "gfproject.json"))
             (p2 (path-join (path-parent gf-lib) "gfproject.json"))
            ) ;
        (cond ((path-file? p1) (path->string p1))
              ((path-file? p2) (path->string p2))
              (else #f)
        ) ;cond
      ) ;let*
    ) ;define

    (define (gfproject-read-file path)
      (if (or (not path) (not (path-file? path)))
        '(())
        (catch #t
          (lambda ()
            (let* ((text (path-read-text path)) (data (string->json text)))
              (if (json-object? data) data '(()))
            ) ;let*
          ) ;lambda
          (lambda (type info) '(()))
        ) ;catch
      ) ;if
    ) ;define

    (define (gfproject-extract-tools config)
      (if (and (json-object? config) (json-contains-key? config "tools"))
        (let ((tools (json-ref config "tools")))
          (if (json-object? tools) tools '(()))
        ) ;let
        '(())
      ) ;if
    ) ;define

    (define (gfproject-deep-merge base overlay)
      (cond ((equal? base '(())) overlay)
            ((equal? overlay '(())) base)
            ((and (json-object? base) (json-object? overlay))
             (let loop
               ((keys (json-keys overlay)) (acc base))
               (if (null? keys)
                 acc
                 (let* ((k (car keys)) (v-overlay (json-ref overlay k)))
                   (if (json-contains-key? acc k)
                     (loop (cdr keys)
                       (json-set acc k (gfproject-deep-merge (json-ref acc k) v-overlay))
                     ) ;loop
                     (loop (cdr keys) (json-push acc k v-overlay))
                   ) ;if
                 ) ;let*
               ) ;if
             ) ;let
            ) ;
            (else overlay)
      ) ;cond
    ) ;define

    (define (gfproject-load-config-bundle . opt-gf-lib)
      (let* ((lib-path (apply gfproject-find-lib-path opt-gf-lib))
             (local-path (gfproject-find-local-path))
             (lib-config (gfproject-read-file lib-path))
             (local-config (gfproject-read-file local-path))
             (lib-tools (gfproject-extract-tools lib-config))
             (local-tools (gfproject-extract-tools local-config))
             (merged-tools (gfproject-deep-merge lib-tools local-tools))
             (merged-config
               (if (equal? lib-config '(()))
                 (if (equal? merged-tools '(())) '(()) (list (cons "tools" merged-tools)))
                 (if (json-contains-key? lib-config "tools")
                   (json-set lib-config "tools" merged-tools)
                   (json-push lib-config "tools" merged-tools)
                 ) ;if
               ) ;if
             ) ;merged-config
            ) ;
        (list (cons "lib_config" lib-config)
          (cons "local_config" local-config)
          (cons "merged_config" merged-config)
          (cons "lib_tools" lib-tools)
          (cons "local_tools" local-tools)
          (cons "merged_tools" merged-tools)
        ) ;list
      ) ;let*
    ) ;define

    (define (gfproject-load-config . opt-gf-lib)
      (bundle-ref (apply gfproject-load-config-bundle opt-gf-lib) "merged_config")
    ) ;define

    (define (gfproject-find-tool-root command . opt-gf-lib)
      (let* ((gf-lib (gfproject--opt-gf-lib opt-gf-lib))
             (candidates (list (path-join (getcwd) "tools" command)
                           (path-join gf-lib "tools" command)
                           (path-join (path-parent gf-lib) "tools" command)
                         ) ;list
             ) ;candidates
             (found (find path-dir? candidates))
            ) ;
        (and found (path->string found))
      ) ;let*
    ) ;define

    (define (gfproject-resolve-tool command . opt-gf-lib)
      (let* ((bundle (apply gfproject-load-config-bundle opt-gf-lib))
             (merged-tools (bundle-ref bundle "merged_tools"))
            ) ;
        (and (json-contains-key? merged-tools command) (json-ref merged-tools command))
      ) ;let*
    ) ;define

    (define (gfproject-resolve-tool-bundle command . opt-gf-lib)
      (let* ((bundle (apply gfproject-load-config-bundle opt-gf-lib))
             (local-tools (bundle-ref bundle "local_tools"))
             (lib-tools (bundle-ref bundle "lib_tools"))
             (merged-tools (bundle-ref bundle "merged_tools"))
             (has-lib? (json-contains-key? lib-tools command))
            ) ;
        (and (json-contains-key? merged-tools command)
          (list (cons "has-local-override" (json-contains-key? local-tools command))
            (cons "has-lib-tool" has-lib?)
            (cons "merged-tool" (json-ref merged-tools command))
            (cons "lib-tool" (if has-lib? (json-ref lib-tools command) '(())))
          ) ;list
        ) ;and
      ) ;let*
    ) ;define

    (define (gfproject-prepare-and-run-tool command tool-config gf-lib allow-fallback)
      (define (fail . parts)
        (if allow-fallback
          #f
          (begin
            (for-each (lambda (p) (display p (current-error-port))) parts)
            1
          ) ;begin
        ) ;if
      ) ;define
      (if (not (json-object? tool-config))
        (fail "Error: Tool '" command "' config must be a JSON object.\n")
        (let ((org (json-ref-string tool-config "organization" #f))
              (module (json-ref-string tool-config "module" #f))
             ) ;
          (if (or (not org) (not module))
            (fail "Error: Tool '"
              command
              "' is not fully implemented (missing organization or module).\n"
            ) ;fail
            (let ((tool-root (gfproject-find-tool-root command gf-lib)))
              (if (not tool-root)
                (fail "Error: tools/" command "/" org " directory not found.\n")
                (begin
                  (set! *load-path* (cons tool-root *load-path*))
                  (let ((import-err
                          (catch #t
                            (lambda ()
                              (eval
                                `(import (,(string->symbol org)
                                          ,(string->symbol module)))
                                (rootlet)
                              ) ;eval
                              #f
                            ) ;lambda
                            (lambda (tag info) (if (pair? info) (car info) "import failed"))
                          ) ;catch
                        ) ;import-err
                       ) ;
                    (if import-err
                      (fail "Error importing (" org " " module "):\n" import-err "\n")
                      (let ((main-proc
                              (catch #t (lambda () (eval 'main (rootlet))) (lambda (tag info) #f))
                            ) ;main-proc
                           ) ;
                        (if (not (procedure? main-proc))
                          (fail "Error: Failed to find main function in (" org " " module ").\n")
                          (let ((res
                                  (catch #t
                                    (lambda () (main-proc))
                                    (lambda (tag info) (display (format #f "~A\n" info) (current-error-port)) 1)
                                  ) ;catch
                                ) ;res
                               ) ;
                            (if (integer? res) res 0)
                          ) ;let
                        ) ;if
                      ) ;let
                    ) ;if
                  ) ;let
                ) ;begin
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    (define (gfproject-run-tool command . opt-gf-lib)
      (let ((bundle (apply gfproject-resolve-tool-bundle command opt-gf-lib)))
        (if (not bundle)
          #f
          (let* ((gf-lib (gfproject--opt-gf-lib opt-gf-lib))
                 (has-local? (bundle-ref bundle "has-local-override"))
                 (has-lib? (bundle-ref bundle "has-lib-tool"))
                 (merged-tool (bundle-ref bundle "merged-tool"))
                 (lib-tool (bundle-ref bundle "lib-tool"))
                 (builtin-fallback? (member command '("help" "version" "eval"
                                                      "load" "repl" "run"))
                 ) ;builtin-fallback?
                ) ;
            (if (and has-local? has-lib?)
              (let ((ret (gfproject-prepare-and-run-tool command merged-tool gf-lib #t)))
                (if ret
                  ret
                  (gfproject-prepare-and-run-tool command lib-tool gf-lib builtin-fallback?)
                ) ;if
              ) ;let
              (gfproject-prepare-and-run-tool command merged-tool gf-lib builtin-fallback?)
            ) ;if
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
