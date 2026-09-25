;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : sql-lang.scm
;; DESCRIPTION : SQL Language support (syntax highlighting / parser features)
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (sql sql-lang) (:use (prog default-lang)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keywords / identifiers
;;
;; SQL keywords categorized by function:
;; - Data Query Language (DQL): SELECT, FROM, WHERE, etc.
;; - Data Manipulation Language (DML): INSERT, UPDATE, DELETE, etc.
;; - Data Definition Language (DDL): CREATE, DROP, ALTER, etc.
;; - Data Control Language (DCL): GRANT, REVOKE, etc.
;; - Transaction Control Language (TCL): COMMIT, ROLLBACK, etc.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "sql") (== key "keyword")))
  `(,(string->symbol key)
    (extra_chars "_")
    ;; SQL constants
    (constant "TRUE" "FALSE" "NULL" "UNKNOWN" "true" "false" "null" "unknown")
    ;; SQL function and procedure declarations
    (declare_function "FUNCTION" "PROCEDURE" "TRIGGER" "function" "procedure"
      "trigger")
    ;; SQL type and object declarations
    (declare_type "CREATE" "DROP" "ALTER" "TABLE" "VIEW" "INDEX" "SEQUENCE"
      "TRIGGER" "TYPE" "DOMAIN" "CONSTRAINT" "TRUNCATE" "create" "drop" "alter"
      "table" "view" "index" "sequence" "trigger" "type" "domain" "constraint"
      "truncate")
    ;; SQL module declarations (schemas, databases)
    (declare_module "SCHEMA" "DATABASE" "schema" "database")
    ;; SQL identifier declarations
    (declare_identifier "PRIMARY" "FOREIGN" "REFERENCES" "UNIQUE" "CHECK"
      "DEFAULT" "NOT" "NULL" "AUTO_INCREMENT" "IDENTITY" "SERIAL" "BIGSERIAL"
      "primary" "foreign" "references" "unique" "check" "default" "not" "null"
      "auto_increment" "identity" "serial" "bigserial")
    ;; General SQL keywords
    (keyword "SELECT" "FROM" "WHERE" "GROUP" "BY" "HAVING" "ORDER" "ASC" "DESC"
      "DISTINCT" "AS" "ON" "USING" "WITH" "RECURSIVE" "JOIN" "INNER" "LEFT"
      "RIGHT" "FULL" "OUTER" "CROSS" "NATURAL" "INSERT" "INTO" "VALUES" "UPDATE"
      "SET" "DELETE" "MERGE" "UPSERT" "GRANT" "REVOKE" "DENY" "UNION"
      "INTERSECT" "EXCEPT" "MINUS" "LIMIT" "OFFSET" "FETCH" "FIRST" "NEXT"
      "ONLY" "ROWS" "ROW" "TOP" "RETURNING" "CAST" "COALESCE" "NULLIF" "IFNULL"
      "NVL" "KEY" "OVER" "PARTITION" "WINDOW" "RANGE" "PRECEDING" "FOLLOWING"
      "UNBOUNDED" "CURRENT" "ADD" "COLUMN" "RENAME" "TO" "MODIFY" "CASCADE"
      "RESTRICT" "TEMPORARY" "TEMP" "IF" "EXPLAIN" "ANALYZE" "SHOW" "DESCRIBE"
      "USE" "select" "from" "where" "group" "by" "having" "order" "asc" "desc"
      "distinct" "as" "on" "using" "with" "recursive" "join" "inner" "left"
      "right" "full" "outer" "cross" "natural" "insert" "into" "values" "update"
      "set" "delete" "merge" "upsert" "grant" "revoke" "deny" "union"
      "intersect" "except" "minus" "limit" "offset" "fetch" "first" "next"
      "only" "rows" "row" "top" "returning" "cast" "coalesce" "nullif" "ifnull"
      "nvl" "key" "over" "partition" "window" "range" "preceding" "following"
      "unbounded" "current" "add" "column" "rename" "to" "modify" "cascade"
      "restrict" "temporary" "temp" "if" "explain" "analyze" "show" "describe"
      "use")
    ;; SQL conditional keywords
    (keyword_conditional "CASE" "WHEN" "THEN" "ELSE" "ELSEIF" "END" "case"
      "when" "then" "else" "elseif" "end")
    ;; SQL control keywords (transactions, etc.)
    (keyword_control "BEGIN" "COMMIT" "ROLLBACK" "SAVEPOINT" "TRANSACTION"
      "START" "RELEASE" "begin" "commit" "rollback" "savepoint" "transaction"
      "start" "release")
    ;; SQL functions and aggregates (treated as keywords for highlighting)
    (keyword "COUNT" "SUM" "AVG" "MIN" "MAX" "STRING_AGG" "ARRAY_AGG"
      "GROUP_CONCAT" "ROW_NUMBER" "RANK" "DENSE_RANK" "PERCENT_RANK" "CUME_DIST"
      "NTILE" "LAG" "LEAD" "FIRST_VALUE" "LAST_VALUE" "NTH_VALUE" "ROUND"
      "TRUNC" "CEIL" "CEILING" "FLOOR" "ABS" "MOD" "POWER" "SQRT" "EXP" "LOG"
      "LN" "SIN" "COS" "TAN" "ASIN" "ACOS" "ATAN" "ATAN2" "RANDOM" "RAND"
      "GREATEST" "LEAST" "NOW" "CURRENT_DATE" "CURRENT_TIME" "CURRENT_TIMESTAMP"
      "LOCALTIME" "LOCALTIMESTAMP" "DATE" "TIME" "TIMESTAMP" "INTERVAL"
      "EXTRACT" "DATE_PART" "DATE_TRUNC" "DATEADD" "DATEDIFF" "DATE_ADD"
      "DATE_SUB" "AGE" "TO_CHAR" "TO_DATE" "TO_NUMBER" "TO_TIMESTAMP" "CONCAT"
      "CONCAT_WS" "SUBSTR" "SUBSTRING" "TRIM" "LTRIM" "RTRIM" "UPPER" "LOWER"
      "INITCAP" "LENGTH" "CHAR_LENGTH" "CHARACTER_LENGTH" "POSITION" "INSTR"
      "REPLACE" "TRANSLATE" "REGEXP_MATCH" "REGEXP_REPLACE" "LPAD" "RPAD"
      "REPEAT" "REVERSE" "NVL2" "CONVERT" "count" "sum" "avg" "min" "max"
      "string_agg" "array_agg" "group_concat" "row_number" "rank" "dense_rank"
      "percent_rank" "cume_dist" "ntile" "lag" "lead" "first_value" "last_value"
      "nth_value" "round" "trunc" "ceil" "ceiling" "floor" "abs" "mod" "power"
      "sqrt" "exp" "log" "ln" "sin" "cos" "tan" "asin" "acos" "atan" "atan2"
      "random" "rand" "greatest" "least" "now" "current_date" "current_time"
      "current_timestamp" "localtime" "localtimestamp" "date" "time" "timestamp"
      "interval" "extract" "date_part" "date_trunc" "dateadd" "datediff"
      "date_add" "date_sub" "age" "to_char" "to_date" "to_number" "to_timestamp"
      "concat" "concat_ws" "substr" "substring" "trim" "ltrim" "rtrim" "upper"
      "lower" "initcap" "length" "char_length" "character_length" "position"
      "instr" "replace" "translate" "regexp_match" "regexp_replace" "lpad"
      "rpad" "repeat" "reverse" "nvl2" "convert")
    ;; SQL data types (treated as keywords for highlighting)
    (keyword "INT" "INTEGER" "SMALLINT" "BIGINT" "TINYINT" "MEDIUMINT" "DECIMAL"
      "NUMERIC" "REAL" "FLOAT" "DOUBLE" "PRECISION" "BOOLEAN" "BOOL" "CHAR"
      "CHARACTER" "VARCHAR" "TEXT" "CLOB" "BLOB" "BYTEA" "VARBINARY" "BINARY"
      "DATE" "TIME" "TIMESTAMP" "TIMESTAMPTZ" "DATETIME" "YEAR" "MONTH" "DAY"
      "HOUR" "MINUTE" "SECOND" "INTERVAL" "ENUM" "SET" "JSON" "JSONB" "XML"
      "UUID" "ARRAY" "RECORD" "ROW" "OBJECT" "MONEY" "BIT" "int" "integer"
      "smallint" "bigint" "tinyint" "mediumint" "decimal" "numeric" "real"
      "float" "double" "precision" "boolean" "bool" "char" "character" "varchar"
      "text" "clob" "blob" "bytea" "varbinary" "binary" "date" "time"
      "timestamp" "timestamptz" "datetime" "year" "month" "day" "hour" "minute"
      "second" "interval" "enum" "set" "json" "jsonb" "xml" "uuid" "array"
      "record" "row" "object" "money" "bit"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Operators
;;
;; SQL operators include:
;; - Arithmetic: + - * / %
;; - Comparison: = <> != < <= > >=
;; - Logical: AND OR NOT
;; - String: || (concatenation in some dialects)
;; - Bitwise: & | ^ ~ << >>
;; - Special: LIKE, ILIKE, BETWEEN, IN, IS, IS NOT
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "sql") (== key "operator")))
  `(,(string->symbol key)
    ;; SQL operators
    (operator "+" "-" "*" "/" "%" "||" "=" "<>" "!=" "<" "<=" ">" ">=" "&" "|"
      "^" "~" "<<" ">>" "::" "->" "->>" "LIKE" "ILIKE" "BETWEEN" "IN" "IS"
      "EXISTS" "ANY" "ALL" "SOME" "AND" "OR" "NOT" "like" "ilike" "between" "in"
      "is" "exists" "any" "all" "some" "and" "or" "not")
    ;; Brackets / braces / parentheses
    (operator_openclose "{" "}" "(" ")" "[" "]" ";" "," "."))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Numbers
;;
;; SQL numeric literals:
;; - Decimal: 123, 123.456, .456, 123.
;; - Scientific: 1.23e4, 1.23E-4
;; - Hex: 0x1F, 0XFF (in some dialects)
;; - Binary: 0b1010, 0B1010 (in some dialects)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "sql") (== key "number")))
  `(,(string->symbol key)
    (bool_features ;; Hex prefix: 0x / 0X (MySQL, PostgreSQL)
      "prefix_0x"
      ;; Binary prefix: 0b / 0B (MySQL)
      "prefix_0b"
      ;; Scientific notation: e/E
      "sci_notation")
    (suffix)
    ;; No standard numeric suffixes in SQL
   )
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Strings
;;
;; SQL strings:
;; - Single-quoted: 'text'
;; - Double-quoted: "text" (for identifiers in some dialects)
;; - Escape sequences: \' \" \\ \n \r \t \b \f
;; - Unicode escapes: \uXXXX, \UXXXXXXXX
;; - Hex escapes: \xHH
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "sql") (== key "string")))
  `(,(string->symbol key)
    (bool_features ;; Escape sequences after backslash
      "escape_char_after_backslash"
      ;; Unicode escapes
      "unicode_escape"
      ;; Hex escapes
      "hex_escape")
    (double_escape "'")
    (escape_sequences "\\" "'" "\"" "a" "b" "f" "n" "r" "t" "v" "x" "u" "U"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Comments
;;
;; SQL comments:
;; - Single line: -- (two hyphens)
;; - Multi-line: /* ... */ (C-style)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parser-feature lan key)
  (:require (and (== lan "sql") (== key "comment")))
  `(,(string->symbol key) (inline "--" "#") (block_comment "/*" "*/"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences for syntax highlighting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (notify-sql-syntax var val)
  (syntax-read-preferences "sql")
) ;define

(define-preferences ("syntax:sql:none" "red" notify-sql-syntax)
 ("syntax:sql:comment" "brown" notify-sql-syntax)
 ("syntax:sql:error" "dark red" notify-sql-syntax)
 ("syntax:sql:constant" "#4040c0" notify-sql-syntax)
 ("syntax:sql:constant_number" "#3030b0" notify-sql-syntax)
 ("syntax:sql:constant_string" "dark grey" notify-sql-syntax)
 ("syntax:sql:constant_char" "#333333" notify-sql-syntax)
 ("syntax:sql:declare_function" "#0000c0" notify-sql-syntax)
 ("syntax:sql:declare_type" "#0000c0" notify-sql-syntax)
 ("syntax:sql:declare_module" "#0000c0" notify-sql-syntax)
 ("syntax:sql:declare_identifier" "#0000c0" notify-sql-syntax)
 ("syntax:sql:operator" "#8b008b" notify-sql-syntax)
 ("syntax:sql:operator_openclose" "#B02020" notify-sql-syntax)
 ("syntax:sql:operator_field" "#888888" notify-sql-syntax)
 ("syntax:sql:operator_special" "orange" notify-sql-syntax)
 ("syntax:sql:keyword" "#309090" notify-sql-syntax)
 ("syntax:sql:keyword_conditional" "#309090" notify-sql-syntax)
 ("syntax:sql:keyword_control" "#008080ff" notify-sql-syntax)
) ;define-preferences
