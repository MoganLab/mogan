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

;; Copyright (C) 2026 Peter McGoron
;;
;; Permission is hereby granted, free of charge, to any person obtaining
;; a copy of this software and associated documentation files (the
;; "Software"), to deal in the Software without restriction, including
;; without limitation the rights to use, copy, modify, merge, publish,
;; distribute, sublicense, and/or sell copies of the Software, and to
;; permit persons to whom the Software is furnished to do so, subject to
;; the following conditions:
;;
;; The above copyright notice and this permission notice shall be
;; included in all copies or substantial portions of the Software.
;;
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
;; LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
;; OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
;; WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;;

(define-library (liii date-time internal database-discover)
  (import (scheme base) (liii path))
  (export discover-zoneinfo-names)
  (begin

    (define (collect-dir-names base curr subdirs names)
      (let* ((dir-path (if (string=? curr "") base (string-append base "/" curr)))
             (entries (vector->list (path-list (path dir-path))))
            ) ;
        (let loop
          ((entries entries) (names names) (subdirs subdirs))
          (if (null? entries)
            (values names subdirs)
            (let* ((ent (car entries))
                   (name (path->string ent))
                   (rel (if (string=? curr "") name (string-append curr "/" name)))
                   (full (string-append base "/" rel))
                  ) ;
              (cond ((member name '("." ".." "posix" "right" "Etc"))
                     (loop (cdr entries) names subdirs)
                    ) ;
                    ((path-dir? (path full)) (loop (cdr entries) names (cons rel subdirs)))
                    ((and (not (string=? (path-suffix ent) ".tab"))
                       (not (string=? (path-suffix ent) ".zi"))
                       (not (string=? (path-suffix ent) ".list"))
                     ) ;and
                     (loop (cdr entries) (cons rel names) subdirs)
                    ) ;
                    (else (loop (cdr entries) names subdirs))
              ) ;cond
            ) ;let*
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (discover-zoneinfo-names)
      (if (not (g_zoneinfo_available?))
        (g_embedded_zoneinfo_names)
        (let ((base "/usr/share/zoneinfo"))
          (let loop
            ((subdirs '("")) (acc '()))
            (if (null? subdirs)
              acc
              (let-values (((names subdirs) (collect-dir-names base (car subdirs) (cdr subdirs) acc)))
                (loop subdirs names)
              ) ;let-values
            ) ;if
          ) ;let
        ) ;let
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
