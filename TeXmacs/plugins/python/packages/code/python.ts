<TeXmacs|2.1.2>

<style|source>

<\body>
  <active*|<\src-title>
    <src-package|python|1.0>

    <\src-purpose>
      Markup for Python sessions.
    </src-purpose>

    <\src-copyright|2021>
      Joris van der Hoeven

      \ \ \ \ 2024 by Darcy Shen
    </src-copyright>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <use-module|(data python)>

  <use-module|(code python-edit)>

  <assign|python|<macro|body|<with|mode|prog|prog-language|python|font-family|rm|<arg|body>>>>

  <assign|python-code|<\macro|body>
    <\pseudo-code>
      <python|<arg|body>>
    </pseudo-code>
  </macro>>

  <\active*>
    <\src-comment>
      Use verbatim output
    </src-comment>
  </active*>

  <assign|python-output|<\macro|body>
    <\with|mode|text|language|verbatim|font-family|tt>
      <\generic-output>
        <arg|body>
      </generic-output>
    </with>
  </macro>>

  <assign|python-errput|<\macro|body>
    <\with|mode|text|language|verbatim|font-family|tt>
      <\generic-errput>
        <arg|body>
      </generic-errput>
    </with>
  </macro>>

  <assign|syntax-python-none|red>

  <assign|syntax-python-comment|brown>

  <assign|syntax-python-error|dark red>

  <assign|syntax-python-constant|#4040c0>

  <assign|syntax-python-constant-number|#4040c0>

  <assign|syntax-python-constant-string|dark grey>

  <assign|syntax-python-constant-char|#333333>

  <assign|syntax-python-declare-function|#0000c0>

  <assign|syntax-python-declare-type|#0000c0>

  <assign|syntax-python-declare-module|#0000c0>

  <assign|syntax-python-operator|#8b008b>

  <assign|syntax-python-operator-openclose|#B02020>

  <assign|syntax-python-operator-field|#888888>

  <assign|syntax-python-operator-special|orange>

  <assign|syntax-python-keyword|#309090>

  <assign|syntax-python-keyword-conditional|#309090>

  <assign|syntax-python-keyword-control|#309090>

  \;
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>