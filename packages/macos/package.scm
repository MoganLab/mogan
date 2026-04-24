(import (liii os) (liii path) (liii sys) (liii string) (liii list) (liii json))

(define (get-script-dir)
  (let* ((args (argv))
         (script-path (if (and (list? args) (> (length args) 1))
                          (cadr args)
                          "")))
    (if (string-contains script-path "/")
        (let ((last-slash (string-index-right script-path (lambda (c) (char=? c #\/)))))
          (if last-slash
              (substring script-path 0 last-slash)
              "."))
        ".")))

(define (call-or-quit . params)
  (define cmd (string-join params " "))
  (display cmd)
  (display "\n")
  (define ec (os-call cmd))
  (when (not (= ec 0))
        (display (string-append "Error: command failed with exit code "
                                (number->string ec)
                                "\n"))
        (exit ec)))

(define (call-quiet . params)
  (define cmd (string-join params " "))
  (define ec (os-call cmd))
  (when (not (= ec 0))
        (display (string-append "Error: command failed with exit code "
                                (number->string ec)
                                "\n"))
        (exit ec)))

(define (shell-output cmd)
  (call-or-quit "bash" "-c" (string-append "'" cmd " > /tmp/gf_cmd_out.txt 2>&1'"))
  (path-read-text "/tmp/gf_cmd_out.txt"))

(define (shell-output-unchecked cmd)
  (os-call (string-append "bash -c '" cmd " > /tmp/gf_cmd_out.txt 2>&1'"))
  (path-read-text "/tmp/gf_cmd_out.txt"))

(define (extract-version-from-file file-path)
  (let* ((content (path-read-text file-path))
         (has-version (string-contains content "XMACS_VERSION")))
    (if has-version
        (let* ((version-start (string-index content (lambda (c) (char=? c #\X))))
               (quote-start-pos (string-index content (lambda (c) (char=? c #\")) version-start)))
          (if quote-start-pos
              (let ((quote-end-pos (string-index content (lambda (c) (char=? c #\")) (+ quote-start-pos 1))))
                (if quote-end-pos
                    (substring content (+ quote-start-pos 1) quote-end-pos)
                    #f))
              #f))
        #f)))

;; Normalize to absolute path before any chdir
(define (realpath dir)
  (let ((cmd (string-append "cd \"" dir "\" && pwd")))
    (os-call (string-append "bash -c '" cmd " > /tmp/gf_realpath.txt 2>/dev/null'"))
    (string-trim-both (path-read-text "/tmp/gf_realpath.txt"))))

(define PACKAGE_HOME (realpath (path-join (get-script-dir) "../..")))
(define VERSION (extract-version-from-file (path-join PACKAGE_HOME "xmake/vars.lua")))
(define NOTO_HOME (path-join PACKAGE_HOME "TeXmacs/fonts/opentype/noto"))

(define (install-noto)
  (if (path-exists? NOTO_HOME)
      (os-call (string-join (list "rm" "-rf" NOTO_HOME) " ")))
  (mkdir NOTO_HOME)
  (let* ((notosans-bold "https://github.com/XmacsLabs/mogan/releases/download/v1.2.9.7/NotoSansCJK-Bold.ttc")
         (notosans-regular "https://github.com/XmacsLabs/mogan/releases/download/v1.2.9.7/NotoSansCJK-Regular.ttc")
         (notoserif-bold "https://github.com/XmacsLabs/mogan/releases/download/v1.2.9.7/NotoSerifCJK-Bold.ttc")
         (notoserif-regular "https://github.com/XmacsLabs/mogan/releases/download/v1.2.9.7/NotoSerifCJK-Regular.ttc"))
    (chdir NOTO_HOME)
    (os-call "pwd")
    (call-or-quit "curl" "-L" "-O" notosans-bold)
    (call-or-quit "curl" "-L" "-O" notosans-regular)
    (call-or-quit "curl" "-L" "-O" notoserif-bold)
    (call-or-quit "curl" "-L" "-O" notoserif-regular)
    (chdir PACKAGE_HOME)
    (os-call "pwd")))

;; ===== Config =====

;; Compute absolute path before chdir changes cwd
(define MACOS_KEY_PATH
  (let ((script (get-script-dir)))
    (if (string-starts? script "/")
        (path-join script ".macos_key")
        (path-join (getcwd) script ".macos_key"))))

(define (parse-macos-key path)
  (if (not (path-exists? path))
      '()
      (let ((content (path-read-text path)))
        (let loop ((lines (string-split content #\newline))
                   (result '()))
          (if (null? lines)
              (reverse result)
              (let ((line (string-trim-both (car lines))))
                (cond ((string-null? line) (loop (cdr lines) result))
                      ((string-starts? line "#") (loop (cdr lines) result))
                      (else
                        (let ((pos (string-index line (lambda (c) (char=? c #\=)))))
                          (if pos
                              (let ((k (string-trim-both (substring line 0 pos)))
                                    (v (string-trim-both (substring line (+ pos 1) (string-length line)))))
                                (loop (cdr lines) (cons (cons k v) result)))
                              (loop (cdr lines) result)))))))))))

(define MACOS_CONFIG (parse-macos-key MACOS_KEY_PATH))

(define (get-config key default)
  (let ((env (getenv key #f)))
    (if env
        env
        (let ((pair (assoc key MACOS_CONFIG)))
          (if pair (cdr pair) default)))))

(define (has-signing-config?)
  (display (string-append "DEBUG: MACOS_KEY_PATH=" MACOS_KEY_PATH "\n"))
  (display (string-append "DEBUG: path-exists?=" (if (path-exists? MACOS_KEY_PATH) "#t" "#f") "\n"))
  (or (path-exists? MACOS_KEY_PATH)
      (getenv "APPLE_CERTIFICATE_P12_BASE64" #f)))

;; ===== Keychain (login keychain) =====

(define LOGIN_KEYCHAIN (path-join (getenv "HOME") "Library/Keychains/login.keychain-db"))
(define CERT_PATH "/tmp/mogan_cert.p12")

(define (decode-base64-to-file b64-str out-path)
  (path-write-text "/tmp/gf_cert_b64.txt" b64-str)
  (call-or-quit "base64" "-D" "-i" "/tmp/gf_cert_b64.txt" "-o" out-path))

(define (import-certificate password)
  ;; Unlock login keychain first
  (os-call "security unlock-keychain -u login.keychain-db 2>/dev/null || true")
  ;; Import certificate
  (let ((ec1 (os-call (string-append "security import " CERT_PATH " -P " password " -k " LOGIN_KEYCHAIN " -T /usr/bin/codesign 2>/dev/null"))))
    (when (not (= ec1 0))
          (call-quiet "security" "import" CERT_PATH "-P" password "-k" LOGIN_KEYCHAIN)))
  ;; Allow codesign to access the keychain without prompting
  (os-call (string-append "security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k '' " LOGIN_KEYCHAIN " 2>/dev/null || true")))

(define (setup-keychain)
  (let ((cert-b64 (get-config "APPLE_CERTIFICATE_P12_BASE64" ""))
        (pass (get-config "APPLE_CERTIFICATE_PASSWORD" "")))
    (when (string-null? cert-b64)
          (display "No certificate configured\n")
          (exit 0))
    (display "Installing certificate to login keychain...\n")
    (decode-base64-to-file cert-b64 CERT_PATH)
    (import-certificate pass)))

(define (extract-quoted-string line)
  (let ((start (string-index line (lambda (c) (char=? c #\")))))
    (if start
        (let ((end (string-index line (lambda (c) (char=? c #\")) (+ start 1))))
          (if (and end (> end (+ start 1)))
              (substring line (+ start 1) end)
              #f))
        #f)))

(define (find-signing-identity)
  (let ((output (shell-output-unchecked "security find-identity -v -p codesigning")))
    (let ((lines (string-split output #\newline)))
      (let loop ((lines lines))
        (if (null? lines)
            #f
            (let ((line (car lines)))
              (if (and (string-contains line "Developer ID Application")
                       (string-contains line "\""))
                  (extract-quoted-string line)
                  (loop (cdr lines)))))))))

;; ===== File helpers =====

(define (find-files-recursive dir predicate)
  (define result '())
  (define (walk d)
    (when (path-dir? d)
      (let ((entries (listdir d)))
        (vector-for-each
          (lambda (entry)
            (let ((full-path (path-join d entry)))
              (cond ((and (path-file? full-path) (predicate full-path))
                     (set! result (cons full-path result)))
                    ((path-dir? full-path)
                     (walk full-path)))))
          entries))))
  (walk dir)
  (reverse result))

(define (find-first-matching dir predicate)
  (define result #f)
  (define (walk d)
    (display (string-append "  [find] entering: " d "\n"))
    (when (and (not result) (path-dir? d))
      (let ((entries (listdir d)))
        (vector-for-each
          (lambda (entry)
            (when (not result)
              (let ((full-path (path-join d entry)))
                (display (string-append "  [find] checking: " full-path "\n"))
                (cond ((predicate full-path)
                       (display (string-append "  [find] matched: " full-path "\n"))
                       (set! result full-path))
                      ((path-dir? full-path)
                       (walk full-path))))))
          entries))))
  (walk dir)
  result)

(define (find-app)
  (find-first-matching "build"
    (lambda (p) (string-ends? p ".app"))))

(define (find-dmg)
  (find-first-matching "build"
    (lambda (p) (string-ends? p ".dmg"))))

;; ===== Codesign =====

(define (codesign-file identity file-path)
  (call-quiet "codesign" "--force" "--options" "runtime" "--timestamp" "--sign" identity file-path))

(define (sign-dylibs identity app-path)
  (let ((fw-dir (path-join app-path "Contents/Frameworks")))
    (when (path-dir? fw-dir)
          (display "Signing dylibs...\n")
          (let ((files (find-files-recursive fw-dir (lambda (p) (string-ends? p ".dylib")))))
            (for-each (lambda (f) (codesign-file identity f)) files)))))

(define (sign-frameworks identity app-path)
  (let ((fw-dir (path-join app-path "Contents/Frameworks")))
    (when (path-dir? fw-dir)
          (display "Signing frameworks...\n")
          (let ((entries (listdir fw-dir)))
            (vector-for-each
              (lambda (entry)
                (let ((fw-path (path-join fw-dir entry)))
                  (when (and (path-dir? fw-path) (string-ends? entry ".framework"))
                        (let ((name (substring entry 0 (- (string-length entry) 10)))
                              (binary-path (path-join fw-path (string-append "Versions/A/" name))))
                          (when (path-file? binary-path)
                                (codesign-file identity binary-path))
                          (codesign-file identity fw-path)))))
              entries)))))

(define (sign-plugins identity app-path)
  (let ((plugins-dir (path-join app-path "Contents/PlugIns")))
    (when (path-dir? plugins-dir)
          (display "Signing plugins...\n")
          (let ((files (find-files-recursive plugins-dir (lambda (p) #t))))
            (for-each (lambda (f) (codesign-file identity f)) files)))))

(define (sign-app-bundle)
  (if (not (has-signing-config?))
      (begin
        (display "Error: No signing config found.\n")
        (display "Please create packages/macos/.macos_key or set APPLE_CERTIFICATE_P12_BASE64 env var.\n")
        (exit 1))
      (begin
        (display "Signing application bundle...\n")
        (setup-keychain)
        (let ((identity (find-signing-identity)))
          (if (not identity)
              (begin
                (display "Error: No signing identity found\n")
                (exit 1))
              (begin
                (display (string-append "Identity: " identity "\n"))
                (let ((app (find-app)))
                  (if (not app)
                      (begin
                        (display "Error: No .app found\n")
                        (exit 1))
                      (begin
                        (display (string-append "App: " app "\n"))
                        (sign-dylibs identity app)
                        (sign-frameworks identity app)
                        (sign-plugins identity app)
                        (display "Signing app bundle...\n")
                        (call-quiet "codesign" "--force" "--options" "runtime" "--deep" "--timestamp" "--sign" identity app)
                        (display "App signing done\n"))))))))))

;; ===== DMG =====

(define (hdiutil-attach dmg-path)
  (let ((output (shell-output (string-append "hdiutil attach \"" dmg-path "\" -nobrowse"))))
    (let ((lines (string-split output #\newline)))
      (let loop ((lines lines))
        (if (null? lines)
            #f
            (let ((line (string-trim-both (car lines))))
              (let ((idx (string-contains line "/Volumes/")))
                (if idx
                    (string-trim-both (substring line idx (string-length line)))
                    (loop (cdr lines))))))))))

(define (hdiutil-detach mount-point)
  (os-call (string-append "bash -c 'hdiutil detach \"" mount-point "\" -force'")))

(define (find-app-in-mount mount-point)
  (let ((entries (listdir mount-point)))
    (let loop ((i 0))
      (if (>= i (vector-length entries))
          #f
          (let ((entry (vector-ref entries i)))
            (if (string-ends? entry ".app")
                entry
                (loop (+ i 1))))))))

(define (create-dmg-from-app app-path dmg-path)
  (call-or-quit "create-dmg"
    "--volname" "Mogan STEM"
    "--window-pos" "200" "120"
    "--window-size" "800" "400"
    "--icon-size" "100"
    "--app-drop-link" "600" "185"
    dmg-path
    app-path))

(define (sign-and-notarize-dmg)
  (when (has-signing-config?)
    (display "Signing DMG and notarizing...\n")
    (let ((cert-b64 (get-config "APPLE_CERTIFICATE_P12_BASE64" "")))
      (when (string-null? cert-b64)
            (display "No certificate configured, skipping DMG signing\n")
            (exit 0)))

          (let ((identity (find-signing-identity)))
            (if (not identity)
                (begin
                  (display "Error: No signing identity found\n")
                  (exit 1))
                (let ((dmg (find-dmg)))
                  (if (not dmg)
                      (begin
                        (display "Error: No DMG found\n")
                        (exit 1))
                      (begin
                        (display (string-append "DMG: " dmg "\n"))

                        ;; Re-sign app in DMG
                        (display "Re-signing app in DMG...\n")
                        (let ((temp-dir (shell-output "mktemp -d")))
                          (set! temp-dir (string-trim-both temp-dir))
                          (let ((mount-point (hdiutil-attach dmg)))
                            (if (not mount-point)
                                (begin
                                  (display "Error: Failed to attach DMG\n")
                                  (exit 1))
                                (let ((app-name (find-app-in-mount mount-point)))
                                  (if (not app-name)
                                      (begin
                                        (display "Error: No app found in DMG\n")
                                        (exit 1))
                                      (begin
                                        (call-quiet "cp" "-R" (path-join mount-point app-name) temp-dir)
                                        (hdiutil-detach mount-point)

                                        (let ((app (path-join temp-dir app-name)))
                                          ;; Sign frameworks and binaries
                                          (let ((fw-dir (path-join app "Contents/Frameworks")))
                                            (when (path-dir? fw-dir)
                                                  (let ((files (find-files-recursive fw-dir (lambda (p) (or (string-ends? p ".dylib") (string-starts? (path-name p) "Qt"))))))
                                                    (for-each (lambda (f) (codesign-file identity f)) files))
                                                  (let ((entries (listdir fw-dir)))
                                                    (vector-for-each
                                                      (lambda (entry)
                                                        (let ((fw-path (path-join fw-dir entry)))
                                                          (when (and (path-dir? fw-path) (string-ends? entry ".framework"))
                                                                (let ((name (substring entry 0 (- (string-length entry) 10)))
                                                                      (binary-path (path-join fw-path (string-append "Versions/A/" name))))
                                                                  (when (path-file? binary-path)
                                                                        (codesign-file identity binary-path))
                                                                  (codesign-file identity fw-path)))))
                                                      entries))))

                                          ;; Sign plugins
                                          (let ((plugins-dir (path-join app "Contents/PlugIns")))
                                            (when (path-dir? plugins-dir)
                                                  (let ((files (find-files-recursive plugins-dir (lambda (p) #t))))
                                                    (for-each (lambda (f) (codesign-file identity f)) files))))

                                          ;; Sign Resources/bin
                                          (let ((bin-dir (path-join app "Contents/Resources/bin")))
                                            (when (path-dir? bin-dir)
                                                  (let ((files (find-files-recursive bin-dir (lambda (p) #t))))
                                                    (for-each (lambda (f) (codesign-file identity f)) files))))

                                          ;; Sign app bundle
                                          (call-quiet "codesign" "--force" "--options" "runtime" "--deep" "--timestamp" "--sign" identity app)

                                          ;; Recreate DMG
                                          (os-call (string-append "bash -c 'rm \"" dmg "\"'"))
                                          (display "Re-creating DMG...\n")
                                          (create-dmg-from-app app dmg)

                                          ;; Cleanup temp
                                          (os-call (string-append "bash -c 'rm -rf \"" temp-dir "\"'"))

                                          ;; Sign DMG
                                          (display "Signing DMG...\n")
                                          (call-quiet "codesign" "--force" "--timestamp" "--sign" identity "--verbose" dmg))))))))

                        ;; Notarization
                        (let ((api-key-id (get-config "APPLE_API_KEY_ID" ""))
                              (api-key-p8 (get-config "APPLE_API_KEY_P8" ""))
                              (api-issuer (get-config "APPLE_API_ISSUER_ID" ""))
                              (team-id (get-config "APPLE_TEAM_ID" "")))
                          (if (and (not (string-null? api-key-id))
                                   (not (string-null? api-key-p8))
                                   (not (string-null? api-issuer)))
                              (begin
                                (display "Setting up API key...\n")
                                (os-call "mkdir -p ~/.appstoreconnect/private_keys/")
                                (let ((api-key-file (string-append "~/.appstoreconnect/private_keys/AuthKey_" api-key-id ".p8")))
                                  (path-write-text api-key-file api-key-p8)
                                  (os-call "chmod 600 ~/.appstoreconnect/private_keys/AuthKey_*.p8")

                                  (display "Submitting for notarization...\n")
                                  (call-or-quit "bash" "-c"
                                    (string-append
                                      "xcrun notarytool submit \"" dmg "\""
                                      " --key " api-key-file
                                      " --key-id " api-key-id
                                      " --issuer " api-issuer
                                      " --team-id " team-id
                                      " --wait --timeout 60m --output-format json > /tmp/notary_result.json"))

                                  (display "Notarization result:\n")
                                  (let ((result (string->json (path-read-text "/tmp/notary_result.json"))))
                                    (let ((status (json-ref-string result "status" "")))
                                      (display (string-append "Status: " status "\n"))
                                      (if (string=? status "Accepted")
                                          (begin
                                            (display "Notarization accepted\n")
                                            ;; Staple
                                            (display "Stapling...\n")
                                            (os-call "sleep 30")
                                            (let loop ((attempt 1))
                                              (if (> attempt 3)
                                                  (display "Staple failed after 3 attempts\n")
                                                  (begin
                                                    (display (string-append "Staple attempt " (number->string attempt) "\n"))
                                                    (let ((ec (os-call (string-append "bash -c 'xcrun stapler staple \"" dmg "\"'"))))
                                                      (if (= ec 0)
                                                          (display "Staple successful\n")
                                                          (begin
                                                            (os-call "sleep 20")
                                                            (loop (+ attempt 1)))))))))
                                          (begin
                                            (display (string-append "Notarization failed: " status "\n"))
                                            (exit 1)))))))
                              (display "No API key, skipping notarization\n"))))))))

    ;; Cleanup temp cert
    (display "Cleaning up temp cert...\n")
    (os-call "rm -f /tmp/mogan_cert.p12 /tmp/gf_cert_b64.txt")
    (display "Done\n")))

;; ===== Main Workflow =====

(display "Start packing in macOS...\n")

(chdir PACKAGE_HOME)
(display PACKAGE_HOME)
(display "\n")
(os-call "pwd")

(display "Install Noto fonts...\n")
(install-noto)
(display "Noto installation finished.\n")

(display "Start install create-dmg...\n")
(call-or-quit "brew" "install" "create-dmg")
(display "create-dmg installation finished.\n")

(display "Start xmake config...\n")
(call-or-quit "xmake" "config" "-m" "release" "-vD" "--yes")
(display "xmake config finished.\n")

(display "Start xmake build...\n")
(call-or-quit "xmake" "build" "-vD" "stem")
(display "xmake build finished.\n")

(display "Start xmake install...\n")
(call-or-quit "xmake" "install" "-vD" "stem")
(display "xmake install finished.\n")

(sign-app-bundle)

(display "Start clean up mounted DMGs...\n")
(call-or-quit "bash" "-c" "'hdiutil" "detach" "/Volumes/*" "-force" "2>/dev/null" "||" "true'")
(call-or-quit "bash" "-c" "'diskutil" "unmount" "/Volumes/*" "-force" "2>/dev/null" "||" "true'")
(call-or-quit "bash" "-c" "'rm" "-rf" "/tmp/create-dmg.*" "2>/dev/null" "||" "true'")
(call-or-quit "bash" "-c" "'find" "/Volumes" "-maxdepth" "1" "-type" "d" "-name" "*" "-exec" "umount" "{}" "\\;" "2>/dev/null" "||" "true'")
(call-or-quit "sleep" "3")
(display "Clean up mounted DMGs finished.\n")

(display "Start create dmg...\n")
(call-or-quit "xmake" "install" "-vD" "stem_packager")
(display "dmg file has been placed in path \"mogan/build\"\n")

(sign-and-notarize-dmg)
