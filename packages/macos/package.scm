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

;; ===== Apple Developer Signing & Notarization =====

(define (run-bash-script content)
  (path-write-text "/tmp/mogan_script.sh" content)
  (os-call "chmod +x /tmp/mogan_script.sh")
  (call-or-quit "bash" "/tmp/mogan_script.sh"))

(define MACOS_KEY_PATH (path-join (get-script-dir) ".macos_key"))

(define (has-signing-config?)
  (or (path-exists? MACOS_KEY_PATH)
      (getenv "APPLE_CERTIFICATE_P12_BASE64" #f)))

(define (source-config-cmd)
  (if (path-exists? MACOS_KEY_PATH)
      (string-append "set -a && source \"" MACOS_KEY_PATH "\" && set +a")
      ":"))

(define (sign-app-bundle)
  (when (has-signing-config?)
    (display "Signing application bundle...\n")
    (run-bash-script
      (string-append
        "#!/bin/bash\n"
        "set -e\n"
        (source-config-cmd) "\n"
        "\n"
        "CERT=\"${APPLE_CERTIFICATE_P12_BASE64:-}\"\n"
        "PASS=\"${APPLE_CERTIFICATE_PASSWORD:-}\"\n"
        "\n"
        "if [ -z \"$CERT\" ]; then\n"
        "  echo 'No certificate configured, skipping app signing'\n"
        "  exit 0\n"
        "fi\n"
        "\n"
        "echo 'Installing certificate...'\n"
        "printf '%s' \"$CERT\" | base64 -d > /tmp/mogan_cert.p12\n"
        "\n"
        "KEYCHAIN=\"/tmp/mogan-signing.keychain-db\"\n"
        "KEYCHAIN_PASS=\"$(openssl rand -base64 32)\"\n"
        "\n"
        "security delete-keychain \"$KEYCHAIN\" 2>/dev/null || true\n"
        "security create-keychain -p \"$KEYCHAIN_PASS\" \"$KEYCHAIN\"\n"
        "security set-keychain-settings -lut 21600 \"$KEYCHAIN\"\n"
        "security unlock-keychain -p \"$KEYCHAIN_PASS\" \"$KEYCHAIN\"\n"
        "security import /tmp/mogan_cert.p12 -P \"$PASS\" -k \"$KEYCHAIN\" -T /usr/bin/codesign 2>/dev/null || security import /tmp/mogan_cert.p12 -P \"$PASS\" -k \"$KEYCHAIN\"\n"
        "security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k \"$KEYCHAIN_PASS\" \"$KEYCHAIN\"\n"
        "security list-keychains -d user -s \"$KEYCHAIN\"\n"
        "security default-keychain -s \"$KEYCHAIN\"\n"
        "\n"
        "IDENTITY=$(security find-identity -v -p codesigning | grep -E 'Developer ID Application|Apple Distribution' | head -1 | awk -F'\"' '{print $2}')\n"
        "if [ -z \"$IDENTITY\" ]; then\n"
        "  echo 'Error: No signing identity found'\n"
        "  exit 1\n"
        "fi\n"
        "echo \"Identity: $IDENTITY\"\n"
        "\n"
        "APP=$(find build -name '*.app' -type d | head -n 1)\n"
        "if [ -z \"$APP\" ]; then\n"
        "  echo 'Error: No .app found'\n"
        "  exit 1\n"
        "fi\n"
        "echo \"App: $APP\"\n"
        "\n"
        "if [ -d \"$APP/Contents/Frameworks\" ]; then\n"
        "  echo 'Signing dylibs...'\n"
        "  find \"$APP/Contents/Frameworks\" -name '*.dylib' -exec codesign --force --options runtime --timestamp --sign \"$IDENTITY\" {} \\; 2>/dev/null || true\n"
        "  echo 'Signing frameworks...'\n"
        "  for fw in \"$APP/Contents/Frameworks\"/*.framework; do\n"
        "    [ -d \"$fw\" ] || continue\n"
        "    name=$(basename \"$fw\" .framework)\n"
        "    [ -f \"$fw/Versions/A/$name\" ] && codesign --force --options runtime --timestamp --sign \"$IDENTITY\" \"$fw/Versions/A/$name\" 2>/dev/null || true\n"
        "    codesign --force --options runtime --timestamp --sign \"$IDENTITY\" \"$fw\" 2>/dev/null || true\n"
        "  done\n"
        "fi\n"
        "\n"
        "if [ -d \"$APP/Contents/PlugIns\" ]; then\n"
        "  echo 'Signing plugins...'\n"
        "  find \"$APP/Contents/PlugIns\" -type f -exec codesign --force --options runtime --timestamp --sign \"$IDENTITY\" {} \\; 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "echo 'Signing app bundle...'\n"
        "codesign --force --options runtime --deep --timestamp --sign \"$IDENTITY\" \"$APP\"\n"
        "echo 'App signing done'\n"))))

(define (sign-and-notarize-dmg)
  (when (has-signing-config?)
    (display "Signing DMG and notarizing...\n")
    (run-bash-script
      (string-append
        "#!/bin/bash\n"
        "set -e\n"
        (source-config-cmd) "\n"
        "\n"
        "CERT=\"${APPLE_CERTIFICATE_P12_BASE64:-}\"\n"
        "PASS=\"${APPLE_CERTIFICATE_PASSWORD:-}\"\n"
        "API_KEY_ID=\"${APPLE_API_KEY_ID:-}\"\n"
        "API_KEY_P8=\"${APPLE_API_KEY_P8:-}\"\n"
        "API_ISSUER=\"${APPLE_API_ISSUER_ID:-}\"\n"
        "TEAM_ID=\"${APPLE_TEAM_ID:-}\"\n"
        "\n"
        "if [ -z \"$CERT\" ]; then\n"
        "  echo 'No certificate configured, skipping DMG signing'\n"
        "  security delete-keychain /tmp/mogan-signing.keychain-db 2>/dev/null || true\n"
        "  exit 0\n"
        "fi\n"
        "\n"
        "KEYCHAIN=\"/tmp/mogan-signing.keychain-db\"\n"
        "if ! security list-keychains | grep -q \"mogan-signing\"; then\n"
        "  echo 'Keychain not found, importing certificate...'\n"
        "  printf '%s' \"$CERT\" | base64 -d > /tmp/mogan_cert.p12\n"
        "  KEYCHAIN_PASS=\"$(openssl rand -base64 32)\"\n"
        "  security create-keychain -p \"$KEYCHAIN_PASS\" \"$KEYCHAIN\"\n"
        "  security set-keychain-settings -lut 21600 \"$KEYCHAIN\"\n"
        "  security unlock-keychain -p \"$KEYCHAIN_PASS\" \"$KEYCHAIN\"\n"
        "  security import /tmp/mogan_cert.p12 -P \"$PASS\" -k \"$KEYCHAIN\" -T /usr/bin/codesign 2>/dev/null || security import /tmp/mogan_cert.p12 -P \"$PASS\" -k \"$KEYCHAIN\"\n"
        "  security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k \"$KEYCHAIN_PASS\" \"$KEYCHAIN\"\n"
        "  security list-keychains -d user -s \"$KEYCHAIN\"\n"
        "  security default-keychain -s \"$KEYCHAIN\"\n"
        "fi\n"
        "\n"
        "IDENTITY=$(security find-identity -v -p codesigning | grep -E 'Developer ID Application|Apple Distribution' | head -1 | awk -F'\"' '{print $2}')\n"
        "if [ -z \"$IDENTITY\" ]; then\n"
        "  echo 'Error: No signing identity found'\n"
        "  exit 1\n"
        "fi\n"
        "\n"
        "DMG=$(find build -name '*.dmg' | head -n 1)\n"
        "if [ -z \"$DMG\" ]; then\n"
        "  echo 'Error: No DMG found'\n"
        "  exit 1\n"
        "fi\n"
        "echo \"DMG: $DMG\"\n"
        "\n"
        "echo 'Re-signing app in DMG...'\n"
        "TEMP=$(mktemp -d)\n"
        "MOUNT=$(hdiutil attach \"$DMG\" -nobrowse | grep -o '/Volumes/.*')\n"
        "APP_NAME=$(ls \"$MOUNT\" | grep '\\.app$')\n"
        "cp -R \"$MOUNT/$APP_NAME\" \"$TEMP/\"\n"
        "hdiutil detach \"$MOUNT\" -force\n"
        "\n"
        "APP=\"$TEMP/$APP_NAME\"\n"
        "\n"
        "if [ -d \"$APP/Contents/Frameworks\" ]; then\n"
        "  find \"$APP/Contents/Frameworks\" -type f \\( -name '*.dylib' -o -name 'Qt*' \\) -exec codesign --force --options runtime --timestamp --sign \"$IDENTITY\" {} \\; 2>/dev/null || true\n"
        "  for fw in \"$APP/Contents/Frameworks\"/*.framework; do\n"
        "    [ -d \"$fw\" ] || continue\n"
        "    name=$(basename \"$fw\" .framework)\n"
        "    [ -f \"$fw/Versions/A/$name\" ] && codesign --force --options runtime --timestamp --sign \"$IDENTITY\" \"$fw/Versions/A/$name\" 2>/dev/null || true\n"
        "    codesign --force --options runtime --timestamp --sign \"$IDENTITY\" \"$fw\" 2>/dev/null || true\n"
        "  done\n"
        "fi\n"
        "\n"
        "if [ -d \"$APP/Contents/PlugIns\" ]; then\n"
        "  find \"$APP/Contents/PlugIns\" -type f -exec codesign --force --options runtime --timestamp --sign \"$IDENTITY\" {} \\; 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "if [ -d \"$APP/Contents/Resources/bin\" ]; then\n"
        "  find \"$APP/Contents/Resources/bin\" -type f -exec codesign --force --options runtime --timestamp --sign \"$IDENTITY\" {} \\; 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "codesign --force --options runtime --deep --timestamp --sign \"$IDENTITY\" \"$APP\"\n"
        "\n"
        "rm \"$DMG\"\n"
        "\n"
        "echo 'Re-creating DMG...'\n"
        "create-dmg \\\n"
        "  --volname 'Mogan STEM' \\\n"
        "  --window-pos 200 120 \\\n"
        "  --window-size 800 400 \\\n"
        "  --icon-size 100 \\\n"
        "  --app-drop-link 600 185 \\\n"
        "  \"$DMG\" \\\n"
        "  \"$APP\"\n"
        "\n"
        "rm -rf \"$TEMP\"\n"
        "\n"
        "echo 'Signing DMG...'\n"
        "codesign --force --timestamp --sign \"$IDENTITY\" --verbose \"$DMG\"\n"
        "\n"
        "if [ -n \"$API_KEY_ID\" ] && [ -n \"$API_KEY_P8\" ] && [ -n \"$API_ISSUER\" ]; then\n"
        "  echo 'Setting up API key...'\n"
        "  mkdir -p ~/.appstoreconnect/private_keys/\n"
        "  printf '%s\\n' \"$API_KEY_P8\" > ~/.appstoreconnect/private_keys/AuthKey_${API_KEY_ID}.p8\n"
        "  chmod 600 ~/.appstoreconnect/private_keys/AuthKey_${API_KEY_ID}.p8\n"
        "\n"
        "  echo 'Submitting for notarization...'\n"
        "  xcrun notarytool submit \"$DMG\" \\\n"
        "    --key ~/.appstoreconnect/private_keys/AuthKey_${API_KEY_ID}.p8 \\\n"
        "    --key-id \"$API_KEY_ID\" \\\n"
        "    --issuer \"$API_ISSUER\" \\\n"
        "    --team-id \"$TEAM_ID\" \\\n"
        "    --wait \\\n"
        "    --timeout 60m \\\n"
        "    --output-format json > /tmp/notary_result.json\n"
        "\n"
        "  echo 'Notarization result:'\n"
        "  cat /tmp/notary_result.json\n"
        "\n"
        "  STATUS=$(cat /tmp/notary_result.json | grep -o '\"status\":\"[^\"]*\"' | head -1 | cut -d'\"' -f4)\n"
        "  if [ \"$STATUS\" != 'Accepted' ]; then\n"
        "    echo \"Notarization failed: $STATUS\"\n"
        "    exit 1\n"
        "  fi\n"
        "  echo 'Notarization accepted'\n"
        "\n"
        "  echo 'Stapling...'\n"
        "  sleep 30\n"
        "  for i in 1 2 3; do\n"
        "    echo \"Staple attempt $i\"\n"
        "    if xcrun stapler staple \"$DMG\"; then\n"
        "      echo 'Staple successful'\n"
        "      break\n"
        "    fi\n"
        "    sleep 20\n"
        "  done\n"
        "else\n"
        "  echo 'No API key, skipping notarization'\n"
        "fi\n"
        "\n"
        "echo 'Cleaning up keychain...'\n"
        "security delete-keychain \"$KEYCHAIN\" 2>/dev/null || true\n"
        "\n"
        "echo 'Done'\n"))))

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

