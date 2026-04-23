(import (liii os) (liii path) (liii sys) (liii string) (liii list))

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

(define PACKAGE_HOME (path-join (get-script-dir) "../.."))
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

