;; IMPORTRANT:
;; Use `node tools/loro-server/server.js` to start the server 
;; before running this test.
(texmacs-module (texmacs tests 2026_collab))

(tm-define (test_2026_collab)
  (display "Running Collab integration test...") (newline)
  (if (loro-enabled?)
    (begin (debug-set "loro" #t)
    (new-buffer)
    
    (display "Creating collab session (Client A)...") (newline)
    (loro-collab-create "ws://127.0.0.1:8765")
    
    (let loopA ((retries 100))
      (loro-collab-poll)
      (if (loro-collab-active?)
          (begin
            (display "Client A active. Doc ID: ")
            (let ((doc-id (loro-collab-doc-id)))
              (display doc-id) (newline)
              
              (insert "Client A Data")
              ;; Wait a bit for the broadcast to happen
              (let wait-send ((w 20))
                (loro-collab-poll)
                (if (> w 0)
                    (exec-delayed-at (lambda () (wait-send (- w 1))) (+ (texmacs-time) 50))
                    (begin
                      (display "Disconnecting Client A...") (newline)
                      (loro-collab-disconnect)
                      
                      (display "Creating new buffer for Client B...") (newline)
                      (new-buffer)
                      
                      (display "Joining collab session (Client B)...") (newline)
                      (loro-collab-join "ws://127.0.0.1:8765" doc-id)
                      
                      (let loopB ((retriesB 100))
                        (loro-collab-poll)
                        (if (loro-collab-active?)
                            (begin
                              (display "Client B active.") (newline)
                              ;; Wait a bit for the remote operations to be applied
                              (let wait-apply ((w2 20))
                                (loro-collab-poll)
                                (if (> w2 0)
                                    (exec-delayed-at (lambda () (wait-apply (- w2 1))) (+ (texmacs-time) 50))
                                    (begin
                                      (display "Buffer B content:") (newline)
                                      (display (tree->stree (buffer-tree))) (newline)
                                      
                                      (quit-TeXmacs)))))
                            (if (> retriesB 0)
                                (exec-delayed-at (lambda () (loopB (- retriesB 1))) (+ (texmacs-time) 100))
                                (begin
                                  (display "Timeout Client B\n")
                                  (quit-TeXmacs))))))))))
          (if (> retries 0)
              (exec-delayed-at (lambda () (loopA (- retries 1))) (+ (texmacs-time) 100))
              (begin
                (display "Timeout Client A\n")
                (quit-TeXmacs)))))))
    (noop))
