<TeXmacs|2.1.4>

<style|<tuple|source|english>>

<\body>
  <active*|<\src-title>
    <src-package|dark|1.0>

    <\src-purpose>
      Dark theme
    </src-purpose>

    <src-copyright|2023|Joris van der Hoeven>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <use-package|gui-button>

  <\active*>
    <\src-comment>
      GUI color scheme
    </src-comment>
  </active*>

  <assign|color|white>

  <assign|cursor-color|white>

  <assign|context-color|#5C8086>

  <assign|locus-color|white>

  <assign|visited-color|white>

  <assign|bg-color|#404040>

  <assign|canvas-color|#202020>

  <assign|nbsp-color|<value|bg-color>>

  <assign|gui-bg-color|#404040>

  <assign|gui-sunny-color|#606060>

  <assign|gui-shadow-color|#202020>

  <assign|gui-blur-color|#c0c0ff>

  <assign|gui-select-color|#205080>

  <assign|button-bg-color|#606060>

  <assign|gui-input-color|#606060>

  <assign|gui-input-list-color|>

  <assign|gui-input-sunny-color|#000000>

  <assign|gui-input-shadow-color|#202020>

  <assign|gui-title-bg-color|#2c2c2c>

  <assign|gui-contour*|<macro|body|<with|marked-color|#8080ff80|<marked|<arg|body>>>>>

  <assign|toggle-off-hover|<gui-contour*|<toggle-off>>>

  <assign|toggle-on-hover|<gui-contour*|<toggle-on>>>

  <assign|pre-edit|<macro|body|<with|ornament-color|#707070|ornament-sunny-color|#808080|ornament-shadow-color|#606060|ornament-border|1px|ornament-hpadding|2px|ornament-vpadding|2px|<smash|<ornament|<with|color|white|<arg|body>>>>>>>

  <assign|focus-color|#3A86FF>

  <assign|selection-color|#3A86FF33>

  <assign|scheme-prompt-color|dark green>

  <assign|scheme-input-color|white>

  <assign|generic-error-color|#c00000>

  <assign|generic-prompt-color|brown>

  <assign|generic-input-color|#c0c0ff>

  <assign|defined-color|#4a71a1>

  <assign|preview-bg-color|#5a7cc2>

  <assign|gr-grid-aspect|<tuple|<tuple|axes|#999999>|<tuple|1|#666666>|<tuple|10|#003153>>>

  <\active*>
    <\src-comment>
      Preamble
    </src-comment>
  </active*>

  <assign|src-tag-color|#00a5db>

  <assign|src-var|<macro|body|<with|mode|src|color|#90ee90|font-shape|italic|<arg|body>>>>

  <assign|src-arg|<macro|body|<with|mode|src|color|#ffa07a|font-shape|italic|<arg|body>>>>

  <assign|src-regular|<macro|body|<with|color|#e0e0e0|<arg|body>>>>

  <assign|src-tt|<macro|body|<with|mode|src|color|#87ceeb|font-family|tt|<arg|body>>>>

  <assign|src-numeric|<macro|body|<with|mode|src|color|#dda0dd|<arg|body>>>>

  <assign|src-length|<macro|body|<with|mode|src|color|#2aadad|<arg|body>>>>

  \;
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>