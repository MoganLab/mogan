;
; Copyright (C) 2024 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (scheme process-context)
  (export get-environment-variable get-environment-variables command-line)
  (begin

    (define (get-environment-variable key)
      (g_get-environment-variable key))

    (define (get-environment-variables)
      (g_getenvs))

    (define (command-line)
      (g_command-line))

    ) ; end of begin
  ) ; end of define-library
