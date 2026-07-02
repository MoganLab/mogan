(use-modules (data java)
  (data julia)
  (data scala)
  (data python)
  (data code)
  (data stem)
) ;use-modules

(import (liii check))

(tm-define (test_70_7)
  ;; Test format-from-suffix function
  (check (format-from-suffix "cpp") => "cpp")
  (check (format-from-suffix "hh") => "cpp")
  (check (format-from-suffix "hpp") => "cpp")
  (check (format-from-suffix "cc") => "cpp")
  (check (format-from-suffix "scala") => "scala")
  (check (format-from-suffix "java") => "java")
  (check (format-from-suffix "py") => "python")
  (check (format-from-suffix "jl") => "julia")
  (check (format-from-suffix "tm") => "texmacs")
  (check (format-from-suffix "ts") => "texmacs")
  (check (format-from-suffix "stm") => "stm")
  (check (format-from-suffix "stem") => "stem")
  (check (format-from-suffix "no-such-format") => "generic")

  ;; Test format? function
  (check (format? "python") => #t)
  (check (format? "scala") => #t)
  (check (format? "no-such-format") => #f)

  ;; Test format-get-name function
  (check (format-get-name "python") => "Python source code")
  (check (format-get-name "scala") => "Scala source code")
  (check (format-get-name "no-such-format") => #f)

  ;; Report results
  (check-report)
) ;tm-define
