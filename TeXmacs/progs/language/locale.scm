
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : locale.scm
;; DESCRIPTION : Mapping of locale and language
;; COPYRIGHT   : (C) 2023 Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

(texmacs-module (language locale))

(define (locale-to-language-name locale)
  (cond ((== locale "bg_BG") "Bulgarian")
        ((== locale "cs_CZ") "Czech")
        ((== locale "da_DK") "Danish")
        ((== locale "de_DE") "German")
        ((== locale "en_US") "English (United States)")
        ((== locale "en_GB") "English (United Kingdom)")
        ((== locale "eo_EO") "Esperanto")
        ((== locale "es_ES") "Spanish")
        ((== locale "fi_FI") "Finnish")
        ((== locale "fr_FR") "French")
        ((== locale "gr_GR") "Greek")
        ((== locale "hr_HR") "Croatian")
        ((== locale "hu_HU") "Hungarian")
        ((== locale "it_IT") "Italian")
        ((== locale "ja_JP") "Japanese")
        ((== locale "ko_KR") "Korean")
        ((== locale "nl_NL") "Dutch")
        ((== locale "pl_PL") "Polish")
        ((== locale "pt_PT") "Portuguese")
        ((== locale "ro_RO") "Romanian")
        ((== locale "ru_RU") "Russian")
        ((== locale "sk_SK") "Slovak")
        ((== locale "sl_SI") "Slovenian")
        ((== locale "sv_SV") "Swedish")
        ((== locale "uk_UA") "Ukrainian")
        ((== locale "zh_CN") "Chinese")
        ((== locale "zh_TW") "Chinese (Taiwan)")
        (else "Unknown")
  ) ;cond
) ;define

(define (language-name-to-locale name)
  (cond ((== name "Bulgarian") "bg_BG")
        ((== name "Czech") "cs_CZ")
        ((== name "Danish") "da_DK")
        ((== name "German") "de_DE")
        ((== name "English (United States)") "en_US")
        ((== name "English (United Kingdom)") "en_GB")
        ((== name "Esperanto") "eo_EO")
        ((== name "Spanish") "es_ES")
        ((== name "Finnish") "fi_FI")
        ((== name "French") "fr_FR")
        ((== name "Greek") "gr_GR")
        ((== name "Croatian") "hr_HR")
        ((== name "Hungarian") "hu_HU")
        ((== name "Italian") "it_IT")
        ((== name "Japanese") "ja_JP")
        ((== name "Korean") "ko_KR")
        ((== name "Dutch") "nl_NL")
        ((== name "Polish") "pl_PL")
        ((== name "Portuguese") "pt_PT")
        ((== name "Romanian") "ro_RO")
        ((== name "Russian") "ru_RU")
        ((== name "Slovak") "sk_SK")
        ((== name "Slovenian") "sl_SI")
        ((== name "Swedish") "sv_SV")
        ((== name "Ukrainian") "uk_UA")
        ((== name "Chinese") "zh_CN")
        ((== name "Chinese (Taiwan)") "zh_TW")
        (else "Unknown")
  ) ;cond
) ;define

(tm-define (language-to-language-name lan)
  (locale-to-language-name (language-to-locale lan))
) ;tm-define

(tm-define (language-name-to-language name)
  (locale-to-language (language-name-to-locale name))
) ;tm-define
