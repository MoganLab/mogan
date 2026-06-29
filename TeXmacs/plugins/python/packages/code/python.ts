<TeXmacs|2.1.4>

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

  \;
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>