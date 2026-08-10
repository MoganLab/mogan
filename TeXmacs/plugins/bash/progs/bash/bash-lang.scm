;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : bash-lang.scm
;; DESCRIPTION : Bash Language
;; COPYRIGHT   : (C) 2026 Hongli Zha
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (bash bash-lang) (:use (prog default-lang)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Parser Features
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Bash reserved words / builtins / constants
(tm-define (parser-feature lan key)
  (:require (and (== lan "bash") (== key "keyword")))
  `(,(string->symbol key)
    (extra_chars "_")
    ;; 标识符允许下划线
    ;; 常量
    (constant "true" "false")
    ;; Bash builtins
    (declare_function ":"
      "."
      "source"
      "alias"
      "unalias"
      "bg"
      "fg"
      "jobs"
      "disown"
      "wait"
      "kill"
      "suspend"
      "cd"
      "pwd"
      "dirs"
      "pushd"
      "popd"
      "echo"
      "printf"
      "read"
      "mapfile"
      "test"
      "["
      "[["
      "type"
      "hash"
      "help"
      "builtin"
      "command"
      "enable"
      "eval"
      "exec"
      "exit"
      "logout"
      "return"
      "export"
      "readonly"
      "unset"
      "set"
      "shift"
      "getopts"
      "declare"
      "typeset"
      "local"
      "let"
      "trap"
      "shopt"
      "ulimit"
      "umask"
      "history"
      "fc"
      "compgen"
      "complete"
      "times")
    (external_command "ssh"
      "scp"
      "rsync"
      ;; 常用外部命令
      "curl"
      "wget"
      "xmake"
      "cmake"
      "make"
      "ninja"
      "gcc"
      "g++"
      "clang"
      "clang++"
      "python"
      "python3"
      "pip"
      "pip3"
      "node"
      "npm"
      "pnpm"
      "docker"
      "docker-compose"
      "tar"
      "zip"
      "unzip"
      "grep"
      "sed"
      "awk"
      "jq"
      "sudo"
      "claude"
      "ll"
      "gh"
      "code"
      "gco"
      ;; Git / GitHub
      "git"
      "gh"
      "gco"
      "gcb"
      "gcm"
      "gca"
      "gcp"
      "gst"
      "gpl"
      "gps"
      ;; Editor / AI
      "code"
      "code-insiders"
      "vim"
      "nvim"
      "claude"
      "cursor"
      "zed"
      ;; Common shell aliases
      "ll"
      "la"
      "l"
      ;; GNU coreutils
      "arch"
      "basename"
      "b2sum"
      "base32"
      "base64"
      "basenc"
      "cat"
      "chcon"
      "chgrp"
      "chmod"
      "chown"
      "chroot"
      "cksum"
      "comm"
      "cp"
      "csplit"
      "cut"
      "date"
      "dd"
      "df"
      "dir"
      "dircolors"
      "dirname"
      "du"
      "env"
      "expand"
      "expr"
      "factor"
      "false"
      "fmt"
      "fold"
      "groups"
      "head"
      "hostid"
      "id"
      "install"
      "join"
      "link"
      "ln"
      "logname"
      "ls"
      "md5sum"
      "mkdir"
      "mkfifo"
      "mknod"
      "mktemp"
      "mv"
      "nice"
      "nl"
      "nohup"
      "nproc"
      "numfmt"
      "od"
      "paste"
      "pathchk"
      "pinky"
      "pr"
      "printenv"
      "ptx"
      "readlink"
      "realpath"
      "rm"
      "rmdir"
      "runcon"
      "seq"
      "sha1sum"
      "sha224sum"
      "sha256sum"
      "sha384sum"
      "sha512sum"
      "shred"
      "shuf"
      "sleep"
      "sort"
      "split"
      "stat"
      "stdbuf"
      "stty"
      "sum"
      "sync"
      "tac"
      "tail"
      "tee"
      "timeout"
      "touch"
      "tr"
      "true"
      "truncate"
      "tsort"
      "tty"
      "uname"
      "unexpand"
      "uniq"
      "unlink"
      "uptime"
      "users"
      "vdir"
      "wc"
      "who"
      "whoami"
      "yes")
    ;; 关键字
    (keyword "if"
      "then"
      "elif"
      "else"
      "fi"
      "for"
      "in"
      "until"
      "while"
      "do"
      "done"
      "case"
      "esac"
      "select"
      "function"
      "coproc"
      "time"
      "{"
      "}"
      "!")
    ;; 控制/环境相关
    (keyword_control "break" "continue"))
) ;tm-define

;; Operators / redirections / parameter expansion helpers
(tm-define (parser-feature lan key)
  (:require (and (== lan "bash") (== key "operator")))
  `(,(string->symbol key)
    ;; 基本运算符（算术/比较/逻辑）
    (operator "+"
      "-"
      "*"
      "/"
      "%"
      "**"
      "++"
      "--"
      "=="
      "!="
      "<"
      ">"
      "<="
      ">="
      "!"
      "~"
      "&"
      "&&"
      "|"
      "||"
      "^"
      "="
      "+="
      "-="
      "*="
      "/="
      "%=")
    ;; 特殊运算符
    (operator_special ";"
      ";;"
      "|&"
      ">"
      ">>"
      "<"
      "<<"
      "<<<"
      "<>"
      ">|"
      "2>"
      "2>>"
      "1>"
      "1>>"
      "&>"
      "&>>")
    ;; 开闭符号
    (operator_openclose "(" ")" "[" "]" "{" "}")
    ;; 字段/参数相关符号（特殊参数 + 参数展开运算符）
    (operator_field "$"
      "@"
      "#"
      "?"
      "!"
      "-"
      "*"
      ;; $@ $# $? $! $$ $- $*
      ":-"
      ":="
      ":?"
      ":+"
      ;; ${var:-x} ${var:=x} ...
      "##"
      "#"
      "%%"
      "%"
      ;; ${var##pat} ${var%pat} ...
    )
  )
) ;tm-define

;; Paths / urls
(tm-define (parser-feature lan key)
  (:require (and (== lan "bash") (== key "path")))
  `(,(string->symbol key) (enable))
) ;tm-define

;; Numbers
(tm-define (parser-feature lan key)
  (:require (and (== lan "bash") (== key "number")))
  `(,(string->symbol key)
    ;; Bash 算术扩展常见前缀：0x(十六进制), 0(八进制)
    (bool_features "decimal" "prefix_0x" "prefix_0")
    (separator "_")
    (suffix))
) ;tm-define

;; Strings
(tm-define (parser-feature lan key)
  (:require (and (== lan "bash") (== key "string")))
  `(,(string->symbol key)
    (bool_features "single_quote"
      "double_quote"
      "multi_byte"
      "unicode_escape"
      "hex_escape")
    ;; Bash 常见转义
    (escape_sequences "\\"
      "\""
      "'"
      "a"
      "b"
      "f"
      "n"
      "r"
      "t"
      "v"
      "newline"
      "x"
      "u"
      "U"
      "$"
      "`"
      "!"))
) ;tm-define

;; Comments
(tm-define (parser-feature lan key)
  (:require (and (== lan "bash") (== key "comment")))
  `(,(string->symbol key) (inline "#") (inline_require_space))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (notify-bash-syntax var val)
  (syntax-read-preferences "bash")
) ;define

(define-preferences ("syntax:bash:none" "red" notify-bash-syntax)
 ("syntax:bash:comment" "brown" notify-bash-syntax)
 ("syntax:bash:error" "dark red" notify-bash-syntax)
 ("syntax:bash:constant" "#4040c0" notify-bash-syntax)
 ("syntax:bash:constant_number" "#4040c0" notify-bash-syntax)
 ("syntax:bash:constant_string" "dark grey" notify-bash-syntax)
 ("syntax:bash:constant_char" "#333333" notify-bash-syntax)
 ("syntax:bash:declare_function" "#0000c0" notify-bash-syntax)
 ("syntax:bash:external_command" "#0000c0" notify-bash-syntax)
 ("syntax:bash:declare_type" "#0000c0" notify-bash-syntax)
 ("syntax:bash:operator" "#8b008b" notify-bash-syntax)
 ("syntax:bash:operator_openclose" "#B02020" notify-bash-syntax)
 ("syntax:bash:operator_field" "#B02020" notify-bash-syntax)
 ("syntax:bash:operator_special" "orange" notify-bash-syntax)
 ("syntax:bash:keyword" "#309090" notify-bash-syntax)
 ("syntax:bash:keyword_control" "#309090" notify-bash-syntax)
) ;define-preferences
