;
; Copyright (C) 2025 The Goldfish Scheme Authors
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
(define-library (liii hashlib)
  (export md5 sha1 sha256
          md5-by-file sha1-by-file sha256-by-file)
  (begin

    (define (md5 str) (g_md5 str))
    (define (sha1 str) (g_sha1 str))
    (define (sha256 str) (g_sha256 str))

    (define (md5-by-file path) (g_md5-by-file path))
    (define (sha1-by-file path) (g_sha1-by-file path))
    (define (sha256-by-file path) (g_sha256-by-file path))

    ) ; end of begin
  ) ; end of define-library
