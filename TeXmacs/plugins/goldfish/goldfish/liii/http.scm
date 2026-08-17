;;
;; Copyright (C) 2025 The Goldfish Scheme Authors
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

(define-library (liii http)
  (import (liii error) (liii http-common) (scheme file))
  (export http-head http-get http-post http-ok?)
  (begin

    (define* (http-head url)
      (let ((r (g_http-head (http-require-string "http-head" "url" url))))
        r
      ) ;let
    ) ;define*

    (define* (http-get url
               (params '())
               (headers '())
               (proxy '())
               (output-file #f)
               (stream #f)
               (callback #f)
             ) ;http-get
      (let* ((url (http-require-string "http-get" "url" url))
             (params (http-normalize-string-alist "http-get" "params" params))
             (headers (http-normalize-string-alist "http-get" "headers" headers))
             (proxy (http-normalize-string-alist "http-get" "proxy" proxy))
             (output-file (http-optional-string "http-get" "output-file" output-file))
             (stream (http-require-boolean "http-get" "stream" stream))
             (callback (http-optional-procedure "http-get" "callback" callback))
            ) ;
        (cond ((not stream) (g_http-get url params headers proxy #f))
              ((and (not output-file) (not callback))
               (value-error "http-get: stream mode requires output-file or callback")
              ) ;
              (else (let ((stream-callback (lambda (chunk)
                                             (if callback (let ((ret (callback chunk))) (if (boolean? ret) ret #t)) #t)
                                           ) ;lambda
                          ) ;stream-callback
                         ) ;
                      (if output-file
                        (let ((port (open-binary-output-file output-file)))
                          (dynamic-wind (lambda () #f)
                            (lambda ()
                              (g_http-get url
                                params
                                headers
                                proxy
                                (lambda (chunk) (write-string chunk port) (stream-callback chunk))
                              ) ;g_http-get
                            ) ;lambda
                            (lambda () (close-port port))
                          ) ;dynamic-wind
                        ) ;let
                        (g_http-get url params headers proxy stream-callback)
                      ) ;if
                    ) ;let
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define*

    (define* (http-post url
               (params '())
               (data "")
               (headers '())
               (proxy '())
               (files '())
               (output-file #f)
               (stream #f)
               (callback #f)
             ) ;http-post
      (let* ((url (http-require-string "http-post" "url" url))
             (params (http-normalize-string-alist "http-post" "params" params))
             (headers (http-normalize-string-alist "http-post" "headers" headers))
             (proxy (http-normalize-string-alist "http-post" "proxy" proxy))
             (files (http-normalize-files "http-post" files))
             (output-file (http-optional-string "http-post" "output-file" output-file))
             (stream (http-require-boolean "http-post" "stream" stream))
             (callback (http-optional-procedure "http-post" "callback" callback))
            ) ;
        (let* ((body-or-data (if (null? files)
                               (http-require-string "http-post" "data" data)
                               (http-normalize-post-form-data "http-post" data)
                             ) ;if
               ) ;body-or-data
               (headers (if (and (null? files) (> (string-length body-or-data) 0) (null? headers))
                          '(("Content-Type" . "text/plain"))
                          headers
                        ) ;if
               ) ;headers
              ) ;
          (cond ((not stream) (g_http-post url params body-or-data headers proxy files #f))
                ((and (not output-file) (not callback))
                 (value-error "http-post: stream mode requires output-file or callback")
                ) ;
                (else (let ((stream-callback (lambda (chunk)
                                               (if callback (let ((ret (callback chunk))) (if (boolean? ret) ret #t)) #t)
                                             ) ;lambda
                            ) ;stream-callback
                           ) ;
                        (if output-file
                          (let ((port (open-binary-output-file output-file)))
                            (dynamic-wind (lambda () #f)
                              (lambda ()
                                (g_http-post url
                                  params
                                  body-or-data
                                  headers
                                  proxy
                                  files
                                  (lambda (chunk) (write-string chunk port) (stream-callback chunk))
                                ) ;g_http-post
                              ) ;lambda
                              (lambda () (close-port port))
                            ) ;dynamic-wind
                          ) ;let
                          (g_http-post url params body-or-data headers proxy files stream-callback)
                        ) ;if
                      ) ;let
                ) ;else
          ) ;cond
        ) ;let*
      ) ;let*
    ) ;define*

  ) ;begin
) ;define-library
