;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : fish-lang.scm
;; DESCRIPTION : fish language support
;; COPYRIGHT   : (C) 2025  veista
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code fish-lang)
  (:use (prog default-lang))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Parser Features
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "keyword")))
  `(,(string->symbol key)
    ;; Built-in names may contain underscores and official dotted namespaces.
    (extra_chars "_" ".")

    (constant
      "false" "true" "null")

    (declare_function
      "define"
      "code.name"

      "creep.active" "creep.cycle" "creep.safety.factor" "creep.solve"
      "creep.step" "creep.time.total" "creep.timestep"
      "creep.timestep.given" "creep.timestep.max"

      "dynamic.active" "dynamic.cycle" "dynamic.safety.factor"
      "dynamic.solve" "dynamic.step" "dynamic.time.total"
      "dynamic.timestep" "dynamic.timestep.given"
      "dynamic.timestep.max"

      "file.all" "file.close" "file.delete" "file.end" "file.exist"
      "file.name" "file.open" "file.open.check" "file.pos"
      "file.read" "file.rename" "file.size" "file.write"

      "fluid.active" "fluid.cycle" "fluid.safety.factor" "fluid.solve"
      "fluid.step" "fluid.time.total" "fluid.timestep"
      "fluid.timestep.given" "fluid.timestep.max"

      "global.cycle" "global.deterministic" "global.dim" "global.fos"
      "global.gravity" "global.step" "global.threads"
      "global.timestep" "global.title"

      "io.dialog.in" "io.dialog.message" "io.dialog.notify"
      "io.in" "io.input" "io.out"

      "mail.account" "mail.attachment.add" "mail.attachment.delete"
      "mail.body" "mail.clear" "mail.domain" "mail.from"
      "mail.host" "mail.password" "mail.port"
      "mail.recipient.add" "mail.recipient.delete"
      "mail.send" "mail.subject"

      "math.aangle.to.euler" "math.abs" "math.acos" "math.and"
      "math.area.intersect.poly.poly" "math.area.poly" "math.asin"
      "math.atan" "math.atan2" "math.bound" "math.ceiling"
      "math.choose" "math.closest.segment.point"
      "math.closest.triangle.point" "math.cos" "math.cosh"
      "math.cross" "math.cyl.bessel.i" "math.ddir.from.normal"
      "math.degrad" "math.dip.from.normal" "math.dist.segment.point"
      "math.dist.segment.segment" "math.dist.triangle.segment"
      "math.dot" "math.erf" "math.erfc" "math.euler.to.aangle"
      "math.exp" "math.expint" "math.floor" "math.gamma"
      "math.in.range" "math.isinf" "math.isnan" "math.ln"
      "math.log" "math.lshift" "math.mag" "math.mag2"
      "math.max" "math.min" "math.normal.from.dip"
      "math.normal.from.dip.ddir" "math.not" "math.or"
      "math.outer.product" "math.pi" "math.random.gauss"
      "math.random.uniform" "math.round" "math.rshift"
      "math.sgn" "math.sin" "math.sinh" "math.sqrt"
      "math.tan" "math.tanh" "math.triangle.inside"
      "math.triangle.interp" "math.unit"

      "mech.active" "mech.cycle" "mech.safety.factor"
      "mech.solve" "mech.step" "mech.time.total"
      "mech.timestep" "mech.timestep.given" "mech.timestep.max"

      "socket.close" "socket.create" "socket.delete"
      "socket.open" "socket.read" "socket.read.array"
      "socket.write" "socket.write.array"

      "system.beep" "system.clone" "system.command"
      "system.directory" "system.directory.absolute"
      "system.directory.create" "system.directory.current"
      "system.directory.delete" "system.directory.list"
      "system.directory.relative" "system.environment"
      "system.error" "system.os" "system.sleep"

      "thermal.active" "thermal.cycle" "thermal.safety.factor"
      "thermal.solve" "thermal.step" "thermal.time.total"
      "thermal.timestep" "thermal.timestep.given"
      "thermal.timestep.max"

      "time.clock" "time.cpu" "time.kernel" "time.real"

      "array.copy" "array.delete" "array.dim" "array.size"

      "list.append" "list.at" "list.concatenate" "list.count"
      "list.create" "list.extend" "list.find.index" "list.insert"
      "list.insert.list" "list.max" "list.min" "list.prepend"
      "list.range" "list.resize" "list.reverse" "list.separate"
      "list.sequence" "list.size" "list.sort" "list.sum"

      "map.add" "map.add.list" "map.has" "map.keys"
      "map.merge" "map.remove" "map.size" "map.value"
      "map.value.all"

      "matrix.cols" "matrix.det" "matrix.from.axis.angle"
      "matrix.from.euler" "matrix.identity" "matrix.inverse"
      "matrix.lubksb" "matrix.ludcmp" "matrix.rows"
      "matrix.to.axis.angle" "matrix.to.euler"
      "matrix.transpose"

      "memory" "memory.create" "memory.delete"
      "memory.fortran.float" "memory.fortran.index"
      "memory.fortran.integer" "memory.offset" "memory.size"

      "string.build" "string.compare" "string.csv.from"
      "string.csv.to" "string.file.ext" "string.file.name"
      "string.file.path" "string.find" "string.find.regex"
      "string.join" "string.len" "string.lower"
      "string.match.regex" "string.replace"
      "string.replace.regex" "string.simplify" "string.split"
      "string.split.regex" "string.sub" "string.token"
      "string.token.type" "string.type" "string.upper"

      "structure.check" "structure.from.map" "structure.name"

      "tensor.i2" "tensor.j2" "tensor.prin"
      "tensor.prin.dir" "tensor.prin.from" "tensor.total"
      "tensor.trace"

      "type" "type.index" "type.name"
      "type.pointer" "type.pointer.id" "type.pointer.name"

      "version.code.major" "version.code.minor"
      "version.fish.major" "version.fish.minor")

    (declare_type
      "array" "boolean" "float" "index" "int" "list"
      "map" "matrix" "string" "struct" "structure"
      "tensor" "vector")

    (declare_identifier
      "global" "local")

    (declare_module
      "creep" "dynamic" "file" "fluid" "io" "mail"
      "math" "mech" "socket" "system" "thermal" "time")

    ;; Lightweight approximation for command/endcommand blocks.
    (keyword
      "call" "fish" "history" "model" "new"
      "program" "restore" "save" "solve")

    (keyword_conditional
      "case" "caseof" "else" "endcase"
      "end_if" "endif" "if" "then")

    (keyword_control
      "command" "continue" "end_loop" "end_section"
      "endcommand" "endloop" "endsection" "exit"
      "exit_loop" "exit_section" "foreach" "for"
      "lock" "loop" "return" "section" "while"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "operator")))
  `(,(string->symbol key)
    (operator
      "^" "/" "//" "*" "%" "-" "+"
      "==" ">" "<" "#" ">=" "<=" "&" "|")
    (operator_special
      "~" "and" "or" "not" "," "::"
      "=::" "+=::" "-=::" "*=::" "/=::")
    (operator_field
      "->" "=" "+=" "-=" "*=" "/=")
    (operator_openclose
      "(" ")" "[" "]"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "number")))
  `(,(string->symbol key)
    (bool_features
      "sci_notation"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "string")))
  `(,(string->symbol key)
    (escape_sequences
      "\\" "\"" "'" "b" "t" "r" "n"))
) ;tm-define

(tm-define (parser-feature lan key)
  (:require (and (== lan "fish") (== key "comment")))
  `(,(string->symbol key)
    (inline ";"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (notify-fish-syntax var val)
  (syntax-read-preferences "fish")
) ;define

(define-preferences
  ("syntax:fish:none" "red" notify-fish-syntax)
  ("syntax:fish:comment" "brown" notify-fish-syntax)
  ("syntax:fish:error" "dark red" notify-fish-syntax)
  ("syntax:fish:constant" "#4040c0" notify-fish-syntax)
  ("syntax:fish:constant_number" "#4040c0" notify-fish-syntax)
  ("syntax:fish:constant_string" "dark grey" notify-fish-syntax)
  ("syntax:fish:constant_char" "#333333" notify-fish-syntax)
  ("syntax:fish:declare_function" "#0000c0" notify-fish-syntax)
  ("syntax:fish:declare_type" "#0000c0" notify-fish-syntax)
  ("syntax:fish:declare_module" "#0000c0" notify-fish-syntax)
  ("syntax:fish:declare_identifier" "#0000c0" notify-fish-syntax)
  ("syntax:fish:operator" "#8b008b" notify-fish-syntax)
  ("syntax:fish:operator_openclose" "#B02020" notify-fish-syntax)
  ("syntax:fish:operator_field" "#B02020" notify-fish-syntax)
  ("syntax:fish:operator_special" "orange" notify-fish-syntax)
  ("syntax:fish:keyword" "#309090" notify-fish-syntax)
  ("syntax:fish:keyword_conditional" "#309090" notify-fish-syntax)
  ("syntax:fish:keyword_control" "#309090" notify-fish-syntax)
) ;define-preferences
