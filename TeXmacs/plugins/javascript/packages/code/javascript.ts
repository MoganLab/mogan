<TeXmacs|2.1.2>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|javascript|1.0>

    <\src-purpose>
      JavaScript Language
    </src-purpose>

    <src-copyright|2020-2024|Darcy Shen>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <use-module|(javascript javascript-format)>

  <assign|javascript|<macro|body|<with|mode|prog|prog-language|javascript|font-family|rm|<arg|body>>>>

  <assign|javascript-code|<\macro|body>
    <\pseudo-code>
      <javascript|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>