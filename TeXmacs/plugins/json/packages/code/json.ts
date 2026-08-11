<TeXmacs|2.1.4>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|json|1.0>

    <\src-purpose>
      json Language
    </src-purpose>

    <src-copyright|2020-2024|Darcy Shen>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <use-module|(json json-format)>

  <assign|json|<macro|body|<with|mode|prog|prog-language|json|font-family|rm|<arg|body>>>>

  <assign|json-code|<\macro|body>
    <\pseudo-code>
      <json|<arg|body>>
    </pseudo-code>
  </macro>>

  <assign|json-chunk|<\macro|name|prev|next|body>
    <\generic-chunk|<arg|name>|<arg|prev>|<arg|next>>
      <\json>
        <arg|body>
      </json>
    </generic-chunk>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>