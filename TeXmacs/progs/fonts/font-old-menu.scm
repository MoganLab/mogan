
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : font-old-menu.scm
;; DESCRIPTION : old font menus
;; COPYRIGHT   : (C) 1999--2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (fonts font-old-menu) (:use (generic format-edit)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The Font submenu in text mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind text-font-menu
  (-> "Name"
    ;; ("Modern" (make-with "font" "modern"))
    ("Roman" (make-with "font" "roman"))
    (if (url-exists-in-tex? "ccr10.mf") ("Concrete" (make-with "font" "concrete")))
    (if (url-exists-in-tex? "pnr10.mf") ("Pandora" (make-with "font" "pandora")))
    (if (font-exists-in-tt? "STIX-Regular") ("Stix" (make-with "font" "stix")))
    ---
    (-> "Adobe"
     ("Avant Garde" (make-with "font" "avant-garde"))
     ("Bookman" (make-with "font" "bookman"))
     (if (url-exists-in-tex? "rpzcmi.tfm")
      ("Chancery" (make-with "font" "chancery"))
     ) ;if
     ("Courier" (make-with "font" "courier"))
     (if (url-exists-in-tex? "rpzdr.tfm") ("Dingbat" (make-with "font" "dingbat")))
     ("Helvetica" (make-with "font" "helvetica"))
     ("N.C. Schoolbook" (make-with "font" "new-century-schoolbook"))
     ("Palatino" (make-with "font" "palatino"))
     ("Times" (make-with "font" "times"))
    ) ;->
    (if (or (font-exists-in-tt? "DejaVuSerif")
          (font-exists-in-tt? "luxirr")
          (font-exists-in-tt? "Apple Symbols")
          (font-exists-in-tt? "texgyretermes-regular")
        ) ;or
      (-> "True type"
        (if (font-exists-in-tt? "texgyrebonum-regular")
         ("Bonum" (make-with "font" "bonum"))
        ) ;if
        (if (font-exists-in-tt? "DejaVuSerif") ("Dejavu" (make-with "font" "dejavu")))
        (if (font-exists-in-tt? "LucidaGrande")
         ("Lucida Grande" (make-with "font" "apple-lucida"))
        ) ;if
        (if (font-exists-in-tt? "luxirr") ("Luxi" (make-with "font" "luxi")))
        (if (font-exists-in-tt? "texgyrepagella-regular")
         ("Pagella" (make-with "font" "pagella"))
        ) ;if
        (if (font-exists-in-tt? "texgyreschola-regular")
         ("Schola" (make-with "font" "schola"))
        ) ;if
        (if (font-exists-in-tt? "Apple Symbols")
         ("Symbols" (make-with "font" "apple-symbols"))
        ) ;if
        (if (font-exists-in-tt? "texgyretermes-regular")
         ("Termes" (make-with "font" "termes"))
        ) ;if
      ) ;->
    ) ;if
    (if (font-exists-in-tt? "times")
      (-> "Microsoft"
        (if (font-exists-in-tt? "andalemo")
         ("Andalemo" (make-with "font" "ms-andalemo"))
        ) ;if
        (if (font-exists-in-tt? "arial") ("Arial" (make-with "font" "ms-arial")))
        (if (font-exists-in-tt? "comic") ("Comic" (make-with "font" "ms-comic")))
        (if (font-exists-in-tt? "cour") ("Courier" (make-with "font" "ms-courier")))
        (if (font-exists-in-tt? "georgia") ("Georgia" (make-with "font" "ms-georgia")))
        (if (font-exists-in-tt? "impact") ("Impact" (make-with "font" "ms-impact")))
        (if (font-exists-in-tt? "lucon") ("Lucida" (make-with "font" "ms-lucida")))
        (if (font-exists-in-tt? "tahoma") ("Tahoma" (make-with "font" "ms-tahoma")))
        (if (font-exists-in-tt? "times") ("Times" (make-with "font" "ms-times")))
        (if (font-exists-in-tt? "trebuc")
         ("Trebuchet" (make-with "font" "ms-trebuchet"))
        ) ;if
        (if (font-exists-in-tt? "verdana") ("Verdana" (make-with "font" "ms-verdana")))
      ) ;->
    ) ;if
    (-> "Latin"
      (if (url-exists-in-tex? "callig15.mf")
       ("Calligraphic" (make-with "font" "calligraphic"))
      ) ;if
      (if (url-exists-in-tex? "capbas.mf") ("Capbas" (make-with "font" "capbas")))
      (if (url-exists-in-tex? "cdr10.mf") ("Duerer" (make-with "font" "duerer")))
      (if (url-exists-in-tex? "frcr10.mf")
       ("French cursive" (make-with "font" "frc"))
      ) ;if
      (if (url-exists-in-tex? "hscs10.mf") ("Hershey" (make-with "font" "hershey")))
      (if (url-exists-in-tex? "la14.mf") ("La" (make-with "font" "la")))
      (if (url-exists-in-tex? "cmfi10.mf") ("Messy" (make-with "font" "messy")))
      (if (url-exists-in-tex? "ocr10.mf") ("Optical" (make-with "font" "optical")))
      (if (url-exists-in-tex? "cpcr10.mf") ("Pacioli" (make-with "font" "pacioli")))
      (if (url-exists-in-tex? "punk20.mf") ("Punk" (make-with "font" "punk")))
      (if (url-exists-in-tex? "twcal14.mf")
       ("Tw Calligraphic" (make-with "font" "twcal"))
      ) ;if
      (if (url-exists-in-tex? "va14.mf") ("Va" (make-with "font" "va")))
    ) ;->
    (-> "Gothic"
      (if (url-exists-in-tex? "blackletter.mf")
       ("Blackletter" (make-with "font" "blackletter"))
      ) ;if
      (if (url-exists-in-tex? "eufm10.mf") ("Euler" (make-with "font" "Euler")))
      (if (url-exists-in-tex? "ygoth.mf") ("Gothic" (make-with "font" "gothic")))
      (if (url-exists-in-tex? "hge.mf")
       ("Old English" (make-with "font" "old-english"))
      ) ;if
      (if (url-exists-in-tex? "schwell.mf") ("Schwell" (make-with "font" "schwell")))
      (if (url-exists-in-tex? "suet14.mf") ("Suet" (make-with "font" "suet")))
      (if (url-exists-in-tex? "yswab.mf") ("Swab" (make-with "font" "swab")))
    ) ;->
    (if (or (supports-chinese?) (supports-japanese?) (supports-korean?))
      (-> "CJK"
        (if (font-exists-in-tt? "Batang") ("Batang" (make-with "font" "batang")))
        (if (font-exists-in-tt? "FandolFang")
         ("FandolFang" (make-with "font" "FandolFang"))
        ) ;if
        (if (font-exists-in-tt? "FandolHei")
         ("FandolHei" (make-with "font" "FandolHei"))
        ) ;if
        (if (font-exists-in-tt? "FandolKai")
         ("FandolKai" (make-with "font" "FandolKai"))
        ) ;if
        (if (font-exists-in-tt? "FandolSong")
         ("FandolSong" (make-with "font" "FandolSong"))
        ) ;if
        (if (font-exists-in-tt? "fireflysung")
         ("Fireflysung" (make-with "font" "fireflysung"))
        ) ;if
        (if (font-exists-in-tt? "AppleGothic")
         ("Gothic" (make-with "font" "apple-gothic"))
        ) ;if
        (if (font-exists-in-tt? "Gulim") ("Gulim" (make-with "font" "gulim")))
        (if (font-exists-in-tt? "华文细黑") ("HeiTi" (make-with "font" "heiti")))
        (if (font-exists-in-tt? "ヒラギノ明朝 ProN W6")
         ("Hiragino Kaku" (make-with "font" "kaku"))
        ) ;if
        (if (font-exists-in-tt? "ipam") ("Ipa" (make-with "font" "ipa")))
        (if (font-exists-in-tt? "ttf-japanese-gothic")
         ("Japanese" (make-with "font" "ttf-japanese"))
        ) ;if
        (if (font-exists-in-tt? "kochi-mincho") ("Kochi" (make-with "font" "kochi")))
        (if (font-exists-in-tt? "儷黑 Pro") ("LiHei" (make-with "font" "lihei")))
        (if (font-exists-in-tt? "wqy-microhei")
         ("MicroHei" (make-with "font" "wqy-microhei"))
        ) ;if
        (if (font-exists-in-tt? "mingliu") ("MingLiU" (make-with "font" "mingliu")))
        (if (font-exists-in-tt? "PMingLiU") ("MingLiU" (make-with "font" "pmingliu")))
        (if (font-exists-in-tt? "MS Gothic")
         ("MS Gothic" (make-with "font" "ms-gothic"))
        ) ;if
        (if (font-exists-in-tt? "MS Mincho")
         ("MS Mincho" (make-with "font" "ms-mincho"))
        ) ;if
        (if (font-exists-in-tt? "sazanami-gothic")
         ("Sazanami" (make-with "font" "sazanami"))
        ) ;if
        (if (font-exists-in-tt? "simfang") ("SimFang" (make-with "font" "simfang")))
        (if (font-exists-in-tt? "simhei") ("SimHei" (make-with "font" "simhei")))
        (if (font-exists-in-tt? "simkai") ("SimKai" (make-with "font" "simkai")))
        (if (font-exists-in-tt? "simli") ("SimLi" (make-with "font" "simli")))
        (if (font-exists-in-tt? "simsun") ("SimSun" (make-with "font" "simsun")))
        (if (and (font-exists-in-tt? "SimSun") (not (font-exists-in-tt? "simsun")))
         ("SimSun" (make-with "font" "apple-simsun"))
        ) ;if
        (if (font-exists-in-tt? "simyou") ("SimYou" (make-with "font" "simyou")))
        (if (font-exists-in-tt? "ukai") ("UKai" (make-with "font" "ukai")))
        (if (font-exists-in-tt? "UnBatang") ("UnBatang" (make-with "font" "unbatang")))
        (if (font-exists-in-tt? "uming") ("UMing" (make-with "font" "uming")))
        (if (font-exists-in-tt? "wqy-zenhei")
         ("ZenHei" (make-with "font" "wqy-zenhei"))
        ) ;if
      ) ;->
    ) ;if
    (-> "Foreign"
      (if (url-exists-in-tex? "nash14.mf") ("Arab" (make-with "font" "arab")))
      (if (url-exists-in-tex? "artmr10.mf")
       ("Armenian" (make-with "font" "armenian"))
      ) ;if
      ("Cyrillic" (make-with "font" "cyrillic"))
      (if (url-exists-in-tex? "dvng10.mf")
       ("Devangari" (make-with "font" "devangari"))
      ) ;if
      (if (url-exists-in-tex? "mxed.mf")
        (-> "Georgian"
         ("Mxedruli" (make-with "font" "mxedruli"))
         ("Xucuri" (make-with "font" "xucuri"))
        ) ;->
      ) ;if
      (if (url-exists-in-tex? "grmn.mf") ("Greek" (make-with "font" "greek")))
      (if (url-exists-in-tex? "redis.mf") ("Hebrew" (make-with "font" "hebrew")))
      (if (url-exists-in-tex? "imr10.mf")
       ("Icelandic" (make-with "font" "icelandic"))
      ) ;if
      (if (url-exists-in-tex? "eiadr10.mf") ("Irish" (make-with "font" "irish")))
      (if (url-exists-in-tex? "osmanian.mf")
       ("Osmanian" (make-with "font" "osmanian"))
      ) ;if
      (if (url-exists-in-tex? "wtkr10.mf") ("Turkish" (make-with "font" "turkish")))
      (if (url-exists-in-tex? "wntml10.mf") ("Tamil" (make-with "font" "tamil")))
      (if (url-exists-in-tex? "thairz10.mf") ("Thai" (make-with "font" "thai")))
      (if (url-exists-in-tex? "vmr10.mf")
       ("Vietnamese" (make-with "font" "vietnamese"))
      ) ;if
    ) ;->
    (if (url-exists-in-tex? "givbc10.mf")
      (-> "Archaic"
        (if (url-exists-in-tex? "bard.mf") ("Bard" (make-with "font" "bard")))
        (if (url-exists-in-tex? "cypr10.mf") ("Cypriot" (make-with "font" "cypriot")))
        (if (url-exists-in-tex? "etr10.mf") ("Etruscan" (make-with "font" "etruscan")))
        (if (url-exists-in-tex? "givbc10.mf")
          (-> "Greek"
           ("4tH Century BC" (make-with "font" "greek4cbc"))
           ("6th Century BC" (make-with "font" "greek6cbc"))
          ) ;->
        ) ;if
        (if (url-exists-in-tex? "linb10.mf")
         ("Linear Beta" (make-with "font" "linearb"))
        ) ;if
        (if (url-exists-in-tex? "ogham.mf") ("Ogham" (make-with "font" "ogham")))
        (if (url-exists-in-tex? "phnc10.mf")
         ("Phoenician" (make-with "font" "phoenician"))
        ) ;if
        (if (url-exists-in-tex? "fut10.mf")
          (-> "Runic"
           ("Default" (make-with "font" "runic"))
           ("Futhark" (make-with "font" "runic*"))
           ("Futhork" (make-with "font" "runic**"))
          ) ;->
        ) ;if
        ;; ("south arabian" (make-with "font" "southarabian"))
        ;; ("syriac" (make-with "font" "syriac"))
        (if (url-exists-in-tex? "izhitsa.mf")
         ("Old Slavonic" (make-with "font" "old-slavonic"))
        ) ;if
        (if (url-exists-in-tex? "ugaritic.mf")
         ("Ugaritic" (make-with "font" "ugaritic"))
        ) ;if
      ) ;->
    ) ;if
    (if (url-exists-in-tex? "cherokee.mf")
      (-> "Phantasy"
        (if (url-exists-in-tex? "cherokee.mf")
         ("Cherokee" (make-with "font" "cherokee"))
        ) ;if
        (if (url-exists-in-tex? "shavian.mf") ("Shavian" (make-with "font" "shavian")))
        (if (url-exists-in-tex? "tengwar.mf") ("Tengwar" (make-with "font" "tengwar")))
      ) ;->
    ) ;if
    (if (url-exists-in-tex? "bbding10.mf")
      (-> "Miscellaneous"
        (if (url-exists-in-tex? "dancers.mf") ("Dancers" (make-with "font" "dancers")))
        (if (url-exists-in-tex? "bbding10.mf") ("Dingbat" (make-with "font" "bbding")))
        (if (url-exists-in-tex? "go10.mf") ("Go" (make-with "font" "go")))
        (if (url-exists-in-tex? "iching.mf") ("Iching" (make-with "font" "iching")))
        (if (url-exists-in-tex? "karta15.mf") ("Karta" (make-with "font" "karta")))
        (if (url-exists-in-tex? "klinz.mf") ("Klinz" (make-with "font" "klinz")))
        (if (url-exists-in-tex? "magic.mf") ("Magic" (make-with "font" "magic")))
        (if (url-exists-in-tex? "cmph10.mf") ("Phonetic" (make-with "font" "phonetic")))
        (if (url-exists-in-tex? "tsipa10.mf") ("Tsipa" (make-with "font" "tsipa")))
        (if (url-exists-in-tex? "wsuipa10.mf") ("Wsuipa" (make-with "font" "wsuipa")))
      ) ;->
    ) ;if
    (-> "X-windows"
     ("Times" (make-with "font" "x-times"))
     ("Courier" (make-with "font" "x-courier"))
     ("Helvetica" (make-with "font" "x-helvetica"))
     ("Utopia" (make-with "font" "x-utopia"))
     ("Lucida" (make-with "font" "x-lucida"))
    ) ;->
  ) ;->
  (-> "Variant"
   ("Roman" (make-with "font-family" "rm"))
   ("Typewriter" (make-with "font-family" "tt"))
   ("Sans serif" (make-with "font-family" "ss"))
  ) ;->
  (-> "Series"
   ("Light" (make-with "font-series" "light"))
   ("Medium" (make-with "font-series" "medium"))
   ("Bold" (make-with "font-series" "bold"))
  ) ;->
  (-> "Shape"
   ("Upright" (make-with "font-shape" "right"))
   ("Slanted" (make-with "font-shape" "slanted"))
   ("Italic" (make-with "font-shape" "italic"))
   ("Left slanted" (make-with "font-shape" "left-slanted"))
   ---
   ("Small caps" (make-with "font-shape" "small-caps"))
   ("Proportional" (make-with "font-shape" "proportional"))
   ("Condensed" (make-with "font-shape" "condensed"))
   ("Flat" (make-with "font-shape" "flat"))
   ("Long" (make-with "font-shape" "long"))
  ) ;->
  (-> "Size" (link font-size-menu))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The Font menu in math mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (real-math-font? fn) (or (== fn "roman") (== fn "concrete")))

(tm-define (real-math-family? fn) (or (== fn "mr") (== fn "ms") (== fn "mt")))

(menu-bind math-font-menu
  (-> "Name"
    (if (url-exists-in-tex? "rpsyr.tfm") ("Adobe" (make-with "math-font" "adobe")))
    (if (font-exists-in-tt? "Apple Symbols")
     ("Apple symbols" (make-with "math-font" "math-apple"))
    ) ;if
    (if (font-exists-in-tt? "Asana-Math")
     ("Asana" (make-with "math-font" "math-asana"))
    ) ;if
    (if (url-exists-in-tex? "ccr10.mf")
     ("Concrete" (make-with "math-font" "concrete"))
    ) ;if
    (if (font-exists-in-tt? "DejaVuSerif")
     ("Dejavu" (make-with "math-font" "math-dejavu"))
    ) ;if
    (if (font-exists-in-tt? "LucidaGrande")
     ("Lucida" (make-with "math-font" "math-lucida"))
    ) ;if
    (if (url-exists-in-tex? "eurm10.mf")
     ("New Roman" (make-with "math-font" "ENR"))
    ) ;if
    (if (font-exists-in-tt? "texgyrepagella-math")
     ("Pagella" (make-with "math-font" "math-pagella"))
    ) ;if
    ("Roman" (make-with "math-font" "roman"))
    (if (font-exists-in-tt? "STIX-Regular")
     ("Stix" (make-with "math-font" "math-stix"))
    ) ;if
    (if (font-exists-in-tt? "texgyretermes-math")
     ("Termes" (make-with "math-font" "math-termes"))
    ) ;if
    ---
    (if (url-exists-in-tex? "cdr10.mf") ("Duerer" (make-with "math-font" "Duerer")))
    (if (url-exists-in-tex? "eufm10.mf") ("Euler" (make-with "math-font" "Euler")))
    (-> "Calligraphic"
     ("Default" (make-with "math-font" "cal"))
     (if (url-exists-in-tex? "euxm10.mf") ("Euler" (make-with "math-font" "cal**")))
     (if (url-exists-in-tex? "rsfs10.mf")
      ("Ralph Smith's" (make-with "math-font" "cal*"))
     ) ;if
    ) ;->
    (-> "Blackboard bold"
     ("Default" (make-with "math-font" "Bbb*"))
     (if (url-exists-in-tex? "msbm10.mf") ("A.M.S." (make-with "math-font" "Bbb")))
     (if (url-exists-in-tex? "bbold10.mf")
      ("Blackboard bold" (make-with "math-font" "Bbb**"))
     ) ;if
     (if (url-exists-in-tex? "ocmr10.mf")
      ("Outlined roman" (make-with "math-font" "Bbb***"))
     ) ;if
     (if (url-exists-in-tex? "dsrom10.mf")
      ("Double stroke" (make-with "math-font" "Bbb****"))
     ) ;if
    ) ;->
  ) ;->
  (if (real-math-font? (get-env "math-font"))
    (-> "Variant"
     ("Roman" (make-with "math-font-family" "mr"))
     ("Typewriter" (make-with "math-font-family" "mt"))
     ("Sans serif" (make-with "math-font-family" "ms"))
     ;; ---
     ;; (-> "Text font"
     ;;    ("Roman" (make-with "math-font-family" "trm"))
     ;;    ("Typewriter" (make-with "math-font-family" "ttt"))
     ;;    ("Sans serif" (make-with "math-font-family" "tss"))
     ;;    ("Bold" (make-with "math-font-family" "bf"))
     ;;    ("Right" (make-with "math-font-family" "up"))
     ;;    ("Slanted" (make-with "math-font-family" "sl"))
     ;;    ("Italic" (make-with "math-font-family" "it")))
    ) ;->
    (if (real-math-family? (get-env "math-font-family"))
      (-> "Series"
       ("Light" (make-with "math-font-series" "light"))
       ("Medium" (make-with "math-font-series" "medium"))
       ("Bold" (make-with "math-font-series" "bold"))
      ) ;->
    ) ;if
    (-> "Shape"
     ("Normal" (make-with "math-font-shape" "normal"))
     ("Upright" (make-with "math-font-shape" "right"))
    ) ;->
  ) ;if
  (if (not (real-math-font? (get-env "math-font")))
    (-> "Variant"
     ("Roman" (make-with "math-font-family" "mr"))
     ("Typewriter" (make-with "math-font-family" "mt"))
     ("Sans serif" (make-with "math-font-family" "ms"))
    ) ;->
    (-> "Series"
     ("Medium" (make-with "math-font-series" "medium"))
     ("Bold" (make-with "math-font-series" "bold"))
    ) ;->
    (-> "Shape"
     ("Default" (make-with "math-font-shape" "normal"))
     ("Right" (make-with "math-font-shape" "right"))
     ("Slanted" (make-with "math-font-shape" "slanted"))
     ("Italic" (make-with "math-font-shape" "italic"))
    ) ;->
  ) ;if
  (-> "Size" (link font-size-menu))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The Font submenu in prog mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind prog-font-menu
  (-> "Name"
   ("roman" (make-with "prog-font" "roman"))
   (if (url-exists-in-tex? "ccr10.mf")
    ("concrete" (make-with "prog-font" "concrete"))
   ) ;if
   (if (url-exists-in-tex? "pnr10.mf")
    ("pandora" (make-with "prog-font" "pandora"))
   ) ;if
  ) ;->
  (-> "Variant"
   ("Roman" (make-with "prog-font-family" "rm"))
   ("Typewriter" (make-with "prog-font-family" "tt"))
   ("Sans serif" (make-with "prog-font-family" "ss"))
  ) ;->
  (-> "Series"
   ("Medium" (make-with "prog-font-series" "medium"))
   ("Bold" (make-with "prog-font-series" "bold"))
  ) ;->
  (-> "Shape"
   ("Default" (make-with "prog-font-shape" "normal"))
   ("Right" (make-with "prog-font-shape" "right"))
   ("Slanted" (make-with "prog-font-shape" "slanted"))
   ("Italic" (make-with "prog-font-shape" "italic"))
  ) ;->
  (-> "Size" (link font-size-menu))
) ;menu-bind
