;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : test-claw-ai-http.scm
;; DESCRIPTION : Test Claw AI HTTP client
;; COPYRIGHT   : (C) 2026 Liii Network
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (claw-ai-http))

;; Test 1: API Health Check
(display "Test 1: API Health Check")
(newline)
(let ((result (claw-ai-api-health)))
  (display "Result: ")
  (display result)
  (newline))

;; Test 2: Session ID Generation
(display "Test 2: Session ID Generation")
(newline)
(let ((session-id (claw-ai-generate-session-id)))
  (display "Session ID: ")
  (display session-id)
  (newline))

;; Test 3: Body to JSON Conversion
(display "Test 3: Body to JSON Conversion")
(newline)
(let ((body (make-hash-table)))
  (hash-table-set! body "message" "Hello")
  (hash-table-set! body "count" 42)
  (let ((json (claw-ai-body-to-json body)))
    (display "JSON: ")
    (display json)
    (newline)))

;; Test 4: Params to Query
(display "Test 4: Params to Query")
(newline)
(let ((params '(("key1" . "value1") ("key2" . "value2"))))
  (let ((query (claw-ai-params-to-query params)))
    (display "Query: ")
    (display query)
    (newline)))

(display "Done!")
(newline)
