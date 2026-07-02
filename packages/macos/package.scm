(import (liii os) (liii path) (liii sys) (liii string) (liii list) (liii json))

(define (get-script-dir)
  (let* ((args (argv))
         (script-path (if (and (list? args) (> (length args) 1)) (cadr args) ""))
        ) ;
    (if (string-contains script-path "/")
      (let ((last-slash (string-index-right script-path (lambda (c) (char=? c #\/)))))
        (if last-slash (substring script-path 0 last-slash) ".")
      ) ;let
      "."
    ) ;if
  ) ;let*
) ;define

(define (call-or-quit . params)
  (define cmd (string-join params " "))
  (display cmd)
  (display "\n")
  (define ec (os-call cmd))
  (when (not (= ec 0))
    (display (string-append "Error: command failed with exit code " (number->string ec) "\n")
    ) ;display
    (exit ec)
  ) ;when
) ;define

(define (call-quiet . params)
  (define cmd (string-join params " "))
  (display (string-append "  [run] " cmd "\n"))
  (define ec (os-call cmd))
  (when (not (= ec 0))
    (display (string-append "  [err] command failed with exit code "
               (number->string ec)
               ": "
               cmd
               "\n"
             ) ;string-append
    ) ;display
    (exit ec)
  ) ;when
) ;define

(define (shell-output cmd)
  (call-or-quit "bash"
    "-c"
    (string-append "'" cmd " > /tmp/gf_cmd_out.txt 2>&1'")
  ) ;call-or-quit
  (path-read-text "/tmp/gf_cmd_out.txt")
) ;define

(define (shell-output-unchecked cmd)
  (os-call (string-append "bash -c '" cmd " > /tmp/gf_cmd_out.txt 2>&1'"))
  (path-read-text "/tmp/gf_cmd_out.txt")
) ;define

(define (extract-version-from-file file-path)
  (let* ((content (path-read-text file-path))
         (has-version (string-contains content "XMACS_VERSION"))
        ) ;
    (if has-version
      (let* ((version-start (string-index content (lambda (c) (char=? c #\X))))
             (quote-start-pos (string-index content (lambda (c) (char=? c #\")) version-start)
             ) ;quote-start-pos
            ) ;
        (if quote-start-pos
          (let ((quote-end-pos (string-index content (lambda (c) (char=? c #\")) (+ quote-start-pos 1))
                ) ;quote-end-pos
               ) ;
            (if quote-end-pos (substring content (+ quote-start-pos 1) quote-end-pos) #f)
          ) ;let
          #f
        ) ;if
      ) ;let*
      #f
    ) ;if
  ) ;let*
) ;define

(define (realpath dir)
  (let ((dir-str (path->string dir)))
    (let ((cmd (string-append "cd \"" dir-str "\" && pwd")))
      (os-call (string-append "bash -c '" cmd " > /tmp/gf_realpath.txt 2>/dev/null'"))
      (string-trim-both (path-read-text "/tmp/gf_realpath.txt"))
    ) ;let
  ) ;let
) ;define

(define PACKAGE_HOME
  (realpath (path->string (path-join (get-script-dir) "../..")))
) ;define

(define VERSION
  (extract-version-from-file (path->string (path-join PACKAGE_HOME "xmake/vars.lua"))
  ) ;extract-version-from-file
) ;define

(define NOTO_HOME
  (path->string (path-join PACKAGE_HOME "TeXmacs/fonts/opentype/noto"))
) ;define

(define (install-noto)
  (if (path-exists? NOTO_HOME)
    (os-call (string-join (list "rm" "-rf" NOTO_HOME) " "))
  ) ;if
  (mkdir NOTO_HOME)
  (let* ((notosans-bold "https://github.com/XmacsLabs/mogan/releases/download/v1.2.9.7/NotoSansCJK-Bold.ttc"
         ) ;notosans-bold
         (notosans-regular "https://github.com/XmacsLabs/mogan/releases/download/v1.2.9.7/NotoSansCJK-Regular.ttc"
         ) ;notosans-regular
         (notoserif-bold "https://github.com/XmacsLabs/mogan/releases/download/v1.2.9.7/NotoSerifCJK-Bold.ttc"
         ) ;notoserif-bold
         (notoserif-regular "https://github.com/XmacsLabs/mogan/releases/download/v1.2.9.7/NotoSerifCJK-Regular.ttc"
         ) ;notoserif-regular
        ) ;
    (chdir NOTO_HOME)
    (os-call "pwd")
    (call-or-quit "curl" "-L" "-O" notosans-bold)
    (call-or-quit "curl" "-L" "-O" notosans-regular)
    (call-or-quit "curl" "-L" "-O" notoserif-bold)
    (call-or-quit "curl" "-L" "-O" notoserif-regular)
    (chdir PACKAGE_HOME)
    (os-call "pwd")
  ) ;let*
) ;define

;; ===== Config =====

;; Compute absolute path before chdir changes cwd

(define MACOS_KEY_PATH
  (let ((script (get-script-dir)))
    (if (string-starts? script "/")
      (path->string (path-join script ".macos_key"))
      (path->string (path-join (getcwd) script ".macos_key"))
    ) ;if
  ) ;let
) ;define

(define (parse-macos-key path)
  (if (not (path-exists? path))
    '()
    (let ((content (path-read-text path)))
      (let loop
        ((lines (string-split content #\newline)) (result '()))
        (if (null? lines)
          (reverse result)
          (let ((line (string-trim-both (car lines))))
            (cond ((string-null? line) (loop (cdr lines) result))
                  ((string-starts? line "#") (loop (cdr lines) result))
                  (else (let ((pos (string-index line (lambda (c) (char=? c #\=)))))
                          (if pos
                            (let ((k (string-trim-both (substring line 0 pos)))
                                  (v (string-trim-both (substring line (+ pos 1) (string-length line))))
                                 ) ;
                              (loop (cdr lines) (cons (cons k v) result))
                            ) ;let
                            (loop (cdr lines) result)
                          ) ;if
                        ) ;let
                  ) ;else
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;let
  ) ;if
) ;define

(define MACOS_CONFIG (parse-macos-key MACOS_KEY_PATH))

(define (get-config key default)
  (let ((env (getenv key #f)))
    (if env
      env
      (let ((pair (assoc key MACOS_CONFIG)))
        (if pair (cdr pair) default)
      ) ;let
    ) ;if
  ) ;let
) ;define

(define (has-signing-config?)
  (or (path-exists? MACOS_KEY_PATH) (getenv "APPLE_CERTIFICATE_P12_BASE64" #f))
) ;define

;; ===== Keychain =====

(define KEYCHAIN_PATH
  (path->string (path-join (getenv "HOME") "Library/Keychains/mogan-signing.keychain-db")
  ) ;path->string
) ;define

(define CERT_PATH "/tmp/mogan_cert.p12")

(define (generate-keychain-pass)
  (call-or-quit "bash"
    "-c"
    "'openssl rand -base64 32 > /tmp/gf_keychain_pass.txt'"
  ) ;call-or-quit
  (string-trim-both (path-read-text "/tmp/gf_keychain_pass.txt"))
) ;define

(define (decode-base64-to-file b64-str out-path)
  (path-write-text "/tmp/gf_cert_b64.txt" b64-str)
  (call-or-quit "base64" "-D" "-i" "/tmp/gf_cert_b64.txt" "-o" out-path)
) ;define

(define (delete-keychain)
  (os-call (string-append "bash -c 'security delete-keychain "
             KEYCHAIN_PATH
             " 2>/dev/null'"
           ) ;string-append
  ) ;os-call
) ;define

(define (create-keychain password)
  (call-quiet "security" "create-keychain" "-p" password KEYCHAIN_PATH)
  (call-quiet "security" "set-keychain-settings" "-lut" "21600" KEYCHAIN_PATH)
  (call-quiet "security" "unlock-keychain" "-p" password KEYCHAIN_PATH)
) ;define

(define (import-apple-ca-certificates)
  (display "Importing Apple CA certificates...\n")
  (let ((devid-ca "/tmp/apple_devid_ca.cer")
        (devid-ca-g2 "/tmp/apple_devid_ca_g2.cer")
       ) ;
    ;; Download Developer ID Certification Authority if not present
    (when (not (path-exists? devid-ca))
      (call-or-quit "curl"
        "-L"
        "-o"
        devid-ca
        "https://www.apple.com/certificateauthority/DeveloperIDCA.cer"
      ) ;call-or-quit
    ) ;when
    ;; Download Developer ID Certification Authority G2 if not present
    (when (not (path-exists? devid-ca-g2))
      (call-or-quit "curl"
        "-L"
        "-o"
        devid-ca-g2
        "https://www.apple.com/certificateauthority/DeveloperIDG2CA.cer"
      ) ;call-or-quit
    ) ;when
    ;; Import to keychain
    (call-quiet "security" "add-certificates" "-k" KEYCHAIN_PATH devid-ca)
    (call-quiet "security" "add-certificates" "-k" KEYCHAIN_PATH devid-ca-g2)
  ) ;let
) ;define

(define (import-certificate password keychain-pass)
  (let ((ec1 (os-call (string-append "bash -c 'security import "
                        CERT_PATH
                        " -P "
                        password
                        " -k "
                        KEYCHAIN_PATH
                        " -T /usr/bin/codesign 2>/dev/null'"
                      ) ;string-append
             ) ;os-call
        ) ;ec1
       ) ;
    (when (not (= ec1 0))
      (call-quiet "security" "import" CERT_PATH "-P" password "-k" KEYCHAIN_PATH)
    ) ;when
  ) ;let
  (import-apple-ca-certificates)
  (call-quiet "security"
    "set-key-partition-list"
    "-S"
    "apple-tool:,apple:,codesign:"
    "-s"
    "-k"
    keychain-pass
    KEYCHAIN_PATH
  ) ;call-quiet
  ;; Build keychain search list dynamically
  (let ((login-keychain (path->string (path-join (getenv "HOME") "Library/Keychains/login.keychain-db"))
        ) ;login-keychain
        (system-keychain "/Library/Keychains/System.keychain")
       ) ;
    (if (path-exists? login-keychain)
      (call-quiet "security"
        "list-keychains"
        "-d"
        "user"
        "-s"
        KEYCHAIN_PATH
        login-keychain
        system-keychain
      ) ;call-quiet
      (call-quiet "security"
        "list-keychains"
        "-d"
        "user"
        "-s"
        KEYCHAIN_PATH
        system-keychain
      ) ;call-quiet
    ) ;if
  ) ;let
  (call-quiet "security" "default-keychain" "-s" KEYCHAIN_PATH)
) ;define

(define (setup-keychain)
  (let ((cert-b64 (get-config "APPLE_CERTIFICATE_P12_BASE64" ""))
        (pass (get-config "APPLE_CERTIFICATE_PASSWORD" ""))
       ) ;
    (when (string-null? cert-b64)
      (display "No certificate configured\n")
      (exit 0)
    ) ;when
    (display "Installing certificate...\n")
    (decode-base64-to-file cert-b64 CERT_PATH)
    (delete-keychain)
    (let ((keychain-pass (generate-keychain-pass)))
      (create-keychain keychain-pass)
      (import-certificate pass keychain-pass)
    ) ;let
  ) ;let
) ;define

(define (extract-quoted-string line)
  (let ((start (string-index line (lambda (c) (char=? c #\")))))
    (if start
      (let ((end (string-index line (lambda (c) (char=? c #\")) (+ start 1))))
        (if (and end (> end (+ start 1))) (substring line (+ start 1) end) #f)
      ) ;let
      #f
    ) ;if
  ) ;let
) ;define

(define (find-signing-identity)
  (let ((output (shell-output-unchecked "security find-identity -v -p codesigning")))
    (let ((lines (string-split output #\newline)))
      (let loop
        ((lines lines))
        (if (null? lines)
          #f
          (let ((line (car lines)))
            (if (and (string-contains line "Developer ID Application")
                  (string-contains line "\"")
                ) ;and
              (extract-quoted-string line)
              (loop (cdr lines))
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;let
  ) ;let
) ;define

;; ===== File helpers =====

(define (find-files-recursive dir predicate)
  (define result '())
  (define (skip-dir? entry)
    (or (string=? entry ".") (string=? entry "..") (string-starts? entry "."))
  ) ;define
  (define (walk d)
    (when (path-dir? d)
      (let ((entries (listdir d)))
        (vector-for-each (lambda (entry)
                           (when (not (skip-dir? entry))
                             (let ((full-path (path->string (path-join d entry))))
                               (cond ((and (path-file? full-path) (predicate full-path))
                                      (set! result (cons full-path result))
                                     ) ;
                                     ((path-dir? full-path) (walk full-path))
                               ) ;cond
                             ) ;let
                           ) ;when
                         ) ;lambda
          entries
        ) ;vector-for-each
      ) ;let
    ) ;when
  ) ;define
  (walk dir)
  (reverse result)
) ;define

(define (find-first-matching dir predicate)
  (define result #f)
  (define (skip-dir? entry)
    (or (string=? entry ".") (string=? entry "..") (string-starts? entry "."))
  ) ;define
  (define (walk d)
    (when (and (not result) (path-dir? d))
      (display (string-append "  [find] entering: " d "\n"))
      (let ((entries (listdir d)))
        (vector-for-each (lambda (entry)
                           (when (and (not result) (not (skip-dir? entry)))
                             (let ((full-path (path->string (path-join d entry))))
                               (cond ((predicate full-path)
                                      (display (string-append "  [find] matched: " full-path "\n"))
                                      (set! result full-path)
                                     ) ;
                                     ((path-dir? full-path) (walk full-path))
                               ) ;cond
                             ) ;let
                           ) ;when
                         ) ;lambda
          entries
        ) ;vector-for-each
      ) ;let
    ) ;when
  ) ;define
  (walk dir)
  result
) ;define

(define (find-app)
  (find-first-matching "build" (lambda (p) (string-ends? p ".app")))
) ;define

(define (find-dmg)
  (find-first-matching "build" (lambda (p) (string-ends? p ".dmg")))
) ;define

;; ===== Codesign =====

(define (codesign-file identity file-path)
  (call-quiet "codesign"
    "--force"
    "--options"
    "runtime"
    "--timestamp"
    "--sign"
    (string-append "\"" identity "\"")
    (string-append "\"" file-path "\"")
  ) ;call-quiet
) ;define

(define (sign-dylibs identity app-path)
  (let ((fw-dir (path->string (path-join app-path "Contents/Frameworks"))))
    (when (path-dir? fw-dir)
      (display "Signing dylibs...\n")
      (let ((files (find-files-recursive fw-dir (lambda (p) (string-ends? p ".dylib")))))
        (for-each (lambda (f) (codesign-file identity f)) files)
      ) ;let
    ) ;when
  ) ;let
) ;define

(define (sign-frameworks identity app-path)
  (let ((fw-dir (path->string (path-join app-path "Contents/Frameworks"))))
    (when (path-dir? fw-dir)
      (display "Signing frameworks...\n")
      (let ((entries (listdir fw-dir)))
        (vector-for-each (lambda (entry)
                           (let ((fw-path (path->string (path-join fw-dir entry))))
                             (when (and (path-dir? fw-path) (string-ends? entry ".framework"))
                               (let ((name (substring entry 0 (- (string-length entry) 10))))
                                 (let ((binary-path (path->string (path-join fw-path (string-append "Versions/A/" name)))
                                       ) ;binary-path
                                      ) ;
                                   (when (path-file? binary-path)
                                     (codesign-file identity binary-path)
                                   ) ;when
                                   (codesign-file identity fw-path)
                                 ) ;let
                               ) ;let
                             ) ;when
                           ) ;let
                         ) ;lambda
          entries
        ) ;vector-for-each
      ) ;let
    ) ;when
  ) ;let
) ;define

(define (sign-plugins identity app-path)
  (let ((plugins-dir (path->string (path-join app-path "Contents/PlugIns"))))
    (when (path-dir? plugins-dir)
      (display "Signing plugins...\n")
      (let ((files (find-files-recursive plugins-dir (lambda (p) #t))))
        (for-each (lambda (f) (codesign-file identity f)) files)
      ) ;let
    ) ;when
  ) ;let
) ;define

(define (executable-file? p)
  (or (string-ends? p ".dylib")
    (string-ends? p ".so")
    (string-ends? p ".bundle")
    (not (or (string-ends? p ".svg")
           (string-ends? p ".png")
           (string-ends? p ".jpg")
           (string-ends? p ".jpeg")
           (string-ends? p ".gif")
           (string-ends? p ".txt")
           (string-ends? p ".md")
           (string-ends? p ".html")
           (string-ends? p ".css")
           (string-ends? p ".js")
           (string-ends? p ".json")
           (string-ends? p ".xml")
           (string-ends? p ".plist")
           (string-ends? p ".ttf")
           (string-ends? p ".otf")
           (string-ends? p ".woff")
           (string-ends? p ".woff2")
           (string-ends? p ".eot")
           (string-ends? p ".ico")
           (string-ends? p ".icns")
           (string-ends? p ".pdf")
           (string-ends? p ".doc")
           (string-ends? p ".docx")
           (string-ends? p ".xls")
           (string-ends? p ".xlsx")
           (string-ends? p ".ppt")
           (string-ends? p ".pptx")
           (string-ends? p ".zip")
           (string-ends? p ".tar")
           (string-ends? p ".gz")
           (string-ends? p ".bz2")
           (string-ends? p ".7z")
           (string-ends? p ".dmg")
           (string-ends? p ".pkg")
           (string-ends? p ".mp3")
           (string-ends? p ".mp4")
           (string-ends? p ".avi")
           (string-ends? p ".mov")
           (string-ends? p ".wav")
           (string-ends? p ".flac")
           (string-ends? p ".ogg")
           (string-ends? p ".webm")
           (string-ends? p ".wasm")
           (string-ends? p ".scm")
           (string-ends? p ".tmu")
           (string-ends? p ".ts")
           (string-ends? p ".tfm")
           (string-ends? p ".xpm")
           (string-ends? p ".tm")
           (string-ends? p ".cache")
         ) ;or
    ) ;not
  ) ;or
) ;define

(define (sign-resources identity app-path)
  (let ((resources-dir (path->string (path-join app-path "Contents/Resources"))))
    (when (path-dir? resources-dir)
      (display "Signing resources...\n")
      (let ((files (find-files-recursive resources-dir
                     (lambda (p) (and (executable-file? p) (not (substring-index p "/fonts/"))))
                   ) ;find-files-recursive
            ) ;files
           ) ;
        (for-each (lambda (f) (codesign-file identity f)) files)
      ) ;let
    ) ;when
  ) ;let
) ;define

(define (sign-app-bundle)
  (if (not (has-signing-config?))
    (begin
      (display "Error: No signing config found.\n")
      (display "Please create packages/macos/.macos_key or set APPLE_CERTIFICATE_P12_BASE64 env var.\n"
      ) ;display
      (exit 1)
    ) ;begin
    (begin
      (display "Signing application bundle...\n")
      (setup-keychain)
      (let ((identity (find-signing-identity)))
        (if (not identity)
          (begin
            (display "Error: No signing identity found\n")
            (exit 1)
          ) ;begin
          (begin
            (display (string-append "Identity: " identity "\n"))
            (let ((app (find-app)))
              (if (not app)
                (begin
                  (display "Error: No .app found\n")
                  (exit 1)
                ) ;begin
                (begin
                  (display (string-append "App: " app "\n"))
                  (sign-dylibs identity app)
                  (sign-frameworks identity app)
                  (sign-plugins identity app)
                  (sign-resources identity app)
                  (display "Signing app bundle...\n")
                  (call-quiet "codesign"
                    "--force"
                    "--options"
                    "runtime"
                    "--deep"
                    "--timestamp"
                    "--sign"
                    (string-append "\"" identity "\"")
                    (string-append "\"" app "\"")
                  ) ;call-quiet
                  (display "App signing done\n")
                ) ;begin
              ) ;if
            ) ;let
          ) ;begin
        ) ;if
      ) ;let
    ) ;begin
  ) ;if
) ;define

;; ===== DMG =====

(define (substring-index str substr)
  (let ((str-len (string-length str)) (sub-len (string-length substr)))
    (let loop
      ((i 0))
      (cond ((> i (- str-len sub-len)) #f)
            ((string=? (substring str i (+ i sub-len)) substr) i)
            (else (loop (+ i 1)))
      ) ;cond
    ) ;let
  ) ;let
) ;define

(define (hdiutil-attach dmg-path)
  (let ((output (shell-output (string-append "hdiutil attach \"" dmg-path "\" -nobrowse"))
        ) ;output
       ) ;
    (let ((lines (string-split output #\newline)))
      (let loop
        ((lines lines))
        (if (null? lines)
          #f
          (let ((line (string-trim-both (car lines))))
            (let ((idx (substring-index line "/Volumes/")))
              (if idx
                (string-trim-both (substring line idx (string-length line)))
                (loop (cdr lines))
              ) ;if
            ) ;let
          ) ;let
        ) ;if
      ) ;let
    ) ;let
  ) ;let
) ;define

(define (hdiutil-detach mount-point)
  (os-call (string-append "bash -c 'hdiutil detach \"" mount-point "\" -force'"))
) ;define

(define (find-app-in-mount mount-point)
  (let ((entries (listdir mount-point)))
    (let loop
      ((i 0))
      (if (>= i (vector-length entries))
        #f
        (let ((entry (vector-ref entries i)))
          (if (string-ends? entry ".app") entry (loop (+ i 1)))
        ) ;let
      ) ;if
    ) ;let
  ) ;let
) ;define

(define (create-dmg-from-app app-path dmg-path)
  (call-or-quit "create-dmg"
    "--volname"
    "\"Mogan STEM\""
    "--window-pos"
    "200"
    "120"
    "--window-size"
    "800"
    "400"
    "--icon-size"
    "100"
    "--app-drop-link"
    "600"
    "185"
    (string-append "\"" dmg-path "\"")
    (string-append "\"" app-path "\"")
  ) ;call-or-quit
) ;define

(define (sign-and-notarize-dmg)
  (if (not (has-signing-config?))
    (begin
      (display "Error: No signing config found for DMG signing and notarization.\n")
      (display "Please create packages/macos/.macos_key or set APPLE_CERTIFICATE_P12_BASE64 env var.\n"
      ) ;display
      (exit 1)
    ) ;begin
    (begin
      (display "Signing DMG and notarizing...\n")
      (let ((cert-b64 (get-config "APPLE_CERTIFICATE_P12_BASE64" "")))
        (when (string-null? cert-b64)
          (display "Error: APPLE_CERTIFICATE_P12_BASE64 is empty\n")
          (delete-keychain)
          (exit 1)
        ) ;when

        (let ((identity (find-signing-identity)))
          (if (not identity)
            (begin
              (display "Error: No signing identity found\n")
              (exit 1)
            ) ;begin
            (let ((dmg (find-dmg)))
              (if (not dmg)
                (begin
                  (display "Error: No DMG found\n")
                  (exit 1)
                ) ;begin
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
                          (exit 1)
                        ) ;begin
                        (let ((app-name (find-app-in-mount mount-point)))
                          (if (not app-name)
                            (begin
                              (display "Error: No app found in DMG\n")
                              (exit 1)
                            ) ;begin
                            (begin
                              (call-quiet "cp"
                                "-R"
                                (string-append "\"" (path->string (path-join mount-point app-name)) "\"")
                                temp-dir
                              ) ;call-quiet
                              (hdiutil-detach mount-point)

                              (let ((app (path->string (path-join temp-dir app-name))))
                                ;; Sign frameworks and binaries
                                (let ((fw-dir (path->string (path-join app "Contents/Frameworks"))))
                                  (when (path-dir? fw-dir)
                                    (let ((files (find-files-recursive fw-dir
                                                   (lambda (p) (or (string-ends? p ".dylib") (string-starts? (path-name p) "Qt")))
                                                 ) ;find-files-recursive
                                          ) ;files
                                         ) ;
                                      (for-each (lambda (f) (codesign-file identity f)) files)
                                    ) ;let
                                    (let ((entries (listdir fw-dir)))
                                      (vector-for-each (lambda (entry)
                                                         (let ((fw-path (path->string (path-join fw-dir entry))))
                                                           (when (and (path-dir? fw-path) (string-ends? entry ".framework"))
                                                             (let ((name (substring entry 0 (- (string-length entry) 10))))
                                                               (let ((binary-path (path->string (path-join fw-path (string-append "Versions/A/" name)))
                                                                     ) ;binary-path
                                                                    ) ;
                                                                 (when (path-file? binary-path)
                                                                   (codesign-file identity binary-path)
                                                                 ) ;when
                                                                 (codesign-file identity fw-path)
                                                               ) ;let
                                                             ) ;let
                                                           ) ;when
                                                         ) ;let
                                                       ) ;lambda
                                        entries
                                      ) ;vector-for-each
                                    ) ;let
                                  ) ;when
                                ) ;let

                                ;; Sign plugins
                                (let ((plugins-dir (path->string (path-join app "Contents/PlugIns"))))
                                  (when (path-dir? plugins-dir)
                                    (let ((files (find-files-recursive plugins-dir (lambda (p) #t))))
                                      (for-each (lambda (f) (codesign-file identity f)) files)
                                    ) ;let
                                  ) ;when
                                ) ;let

                                ;; Sign resources (including plugin binaries like goldfish)
                                (sign-resources identity app)

                                ;; Sign app bundle
                                (call-quiet "codesign"
                                  "--force"
                                  "--options"
                                  "runtime"
                                  "--deep"
                                  "--timestamp"
                                  "--sign"
                                  (string-append "\"" identity "\"")
                                  (string-append "\"" app "\"")
                                ) ;call-quiet

                                ;; Recreate DMG
                                (os-call (string-append "bash -c 'rm \"" dmg "\"'"))
                                (display "Re-creating DMG...\n")
                                (create-dmg-from-app app dmg)

                                ;; Cleanup temp
                                (os-call (string-append "bash -c 'rm -rf \"" temp-dir "\"'"))

                                ;; Sign DMG
                                (display "Signing DMG...\n")
                                (call-quiet "codesign"
                                  "--force"
                                  "--timestamp"
                                  "--sign"
                                  (string-append "\"" identity "\"")
                                  "--verbose"
                                  (string-append "\"" dmg "\"")
                                ) ;call-quiet
                              ) ;let
                            ) ;begin
                          ) ;if
                        ) ;let
                      ) ;if
                    ) ;let
                  ) ;let

                  ;; Notarization
                  (let ((api-key-id (get-config "APPLE_API_KEY_ID" ""))
                        (api-key-p8 (get-config "APPLE_API_KEY_P8" ""))
                        (api-issuer (get-config "APPLE_API_ISSUER_ID" ""))
                        (team-id (get-config "APPLE_TEAM_ID" ""))
                       ) ;
                    (if (and (not (string-null? api-key-id))
                          (not (string-null? api-key-p8))
                          (not (string-null? api-issuer))
                        ) ;and
                      (begin
                        (display "Setting up API key...\n")
                        (let ((api-key-dir (path->string (path-join (getenv "HOME") ".appstoreconnect/private_keys"))
                              ) ;api-key-dir
                             ) ;
                          (os-call (string-append "mkdir -p " api-key-dir))
                          (let ((api-key-file (path->string (path-join api-key-dir (string-append "AuthKey_" api-key-id ".p8"))
                                              ) ;path->string
                                ) ;api-key-file
                               ) ;
                            (path-write-text "/tmp/gf_apikey_b64.txt" api-key-p8)
                            (call-or-quit "bash"
                              "-c"
                              (string-append "'base64 -D -i /tmp/gf_apikey_b64.txt -o \"" api-key-file "\"'")
                            ) ;call-or-quit
                            (os-call (string-append "chmod 600 \"" api-key-file "\""))

                            (display "Submitting for notarization...\n")
                            (call-or-quit "bash"
                              "-c"
                              (string-append "'xcrun notarytool submit \""
                                dmg
                                "\""
                                " --key "
                                api-key-file
                                " --key-id "
                                api-key-id
                                " --issuer "
                                api-issuer
                                " --wait --timeout 60m --output-format json > /tmp/notary_result.json 2>&1"
                                " || { echo \"--- notarytool stderr ---\"; cat /tmp/notary_result.json 1>&2; exit 1; }'"
                              ) ;string-append
                            ) ;call-or-quit
                          ) ;let

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
                                  (let loop
                                    ((attempt 1))
                                    (if (> attempt 3)
                                      (display "Staple failed after 3 attempts\n")
                                      (begin
                                        (display (string-append "Staple attempt " (number->string attempt) "\n"))
                                        (let ((ec (os-call (string-append "bash -c 'xcrun stapler staple \"" dmg "\"'"))))
                                          (if (= ec 0)
                                            (display "Staple successful\n")
                                            (begin
                                              (os-call "sleep 20")
                                              (loop (+ attempt 1))
                                            ) ;begin
                                          ) ;if
                                        ) ;let
                                      ) ;begin
                                    ) ;if
                                  ) ;let
                                ) ;begin
                                (begin
                                  (display (string-append "Notarization failed: " status "\n"))
                                  (exit 1)
                                ) ;begin
                              ) ;if
                            ) ;let
                          ) ;let
                        ) ;let
                      ) ;begin
                      (begin
                        (display "Error: Notarization requires APPLE_API_KEY_ID, APPLE_API_KEY_P8, and APPLE_API_ISSUER_ID\n"
                        ) ;display
                        (display "Please add them to packages/macos/.macos_key or set as environment variables\n"
                        ) ;display
                        (exit 1)
                      ) ;begin
                    ) ;if
                  ) ;let
                ) ;begin
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let

      ;; Cleanup keychain
      (display "Cleaning up keychain...\n")
      (delete-keychain)
      (display "Done\n")
    ) ;begin
  ) ;if
) ;define

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
(call-or-quit "bash"
  "-c"
  "'hdiutil"
  "detach"
  "/Volumes/*"
  "-force"
  "2>/dev/null"
  "||"
  "true'"
) ;call-or-quit
(call-or-quit "bash"
  "-c"
  "'diskutil"
  "unmount"
  "/Volumes/*"
  "-force"
  "2>/dev/null"
  "||"
  "true'"
) ;call-or-quit
(call-or-quit "bash"
  "-c"
  "'rm"
  "-rf"
  "/tmp/create-dmg.*"
  "2>/dev/null"
  "||"
  "true'"
) ;call-or-quit
(call-or-quit "bash"
  "-c"
  "'find"
  "/Volumes"
  "-maxdepth"
  "1"
  "-type"
  "d"
  "-name"
  "*"
  "-exec"
  "umount"
  "{}"
  "\\;"
  "2>/dev/null"
  "||"
  "true'"
) ;call-or-quit
(call-or-quit "sleep" "3")
(display "Clean up mounted DMGs finished.\n")

(display "Start create dmg...\n")
(call-or-quit "xmake" "install" "-vD" "stem_packager")
(display "dmg file has been placed in path \"mogan/build\"\n")

(sign-and-notarize-dmg)
