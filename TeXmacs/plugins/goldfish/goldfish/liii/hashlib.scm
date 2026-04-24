(define-library (liii hashlib)
  (export md5
    sha1
    sha256
    md5-by-file
    sha1-by-file
    sha256-by-file
  ) ;export
  (begin

    (define (md5 str)
      (g_md5 str)
    ) ;define
    (define (sha1 str)
      (g_sha1 str)
    ) ;define
    (define (sha256 str)
      (g_sha256 str)
    ) ;define

    (define (md5-by-file path)
      (g_md5-by-file path)
    ) ;define
    (define (sha1-by-file path)
      (g_sha1-by-file path)
    ) ;define
    (define (sha256-by-file path)
      (g_sha256-by-file path)
    ) ;define

  ) ;begin
) ;define-library
