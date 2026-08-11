<TeXmacs|2.1.4>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|moonbit|1.0>

    <\src-purpose>
      moonbit Language
    </src-purpose>

    <src-copyright|2025|Jack Yansong Li>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <use-module|(moonbit moonbit-format)> <use-module|(moonbit moonbit-edit)>

  <assign|moonbit|<macro|body|<with|mode|prog|prog-language|moonbit|font-family|rm|<arg|body>>>>

  <assign|moonbit-code|<\macro|body>
    <\pseudo-code>
      <moonbit|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>