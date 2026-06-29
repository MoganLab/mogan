(import (liii check))

(tm-define (test_stem_python_equiv)
  (define python-ts "$TEXMACS_PATH/plugins/python/packages/code/python.ts")
  (define python-stem "$TEXMACS_PATH/plugins/python/packages/code/python.stem")

  (define ts-tree (tree-import python-ts "texmacs"))
  (define stem-tree (tree-import python-stem "stem"))

  (display* "ts tree arity: " (tree-arity ts-tree) "\n")
  (display* "stem tree arity: " (tree-arity stem-tree) "\n")

  (check (tree->stree ts-tree) => (tree->stree stem-tree))

  (check-report)
) ;tm-define
