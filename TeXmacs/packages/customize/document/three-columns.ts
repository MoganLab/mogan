<TeXmacs|2.1.4>

<style|<tuple|source|std-pattern>>

<\body>
  <active*|<\src-title>
    <src-package-dtd|three-columns|1.0|three-columns|1.0>

    <\src-purpose>
      Standard customization for three column styles
    </src-purpose>

    <src-copyright|2026|(Jack) Yansong Li>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <assign|par-columns|3>

  <assign|par-columns-sep|0.6667fn>

  <\active*>
    <\src-comment>
      Titles.
    </src-comment>
  </active*>

  <assign|doc-make-title|<\macro|body>
    <\with|par-columns|1>
      <\surround||<right-flush>>
        <doc-title-block|<arg|body>>

        \;
      </surround>
    </with>
  </macro>>

  <assign|doc-make-rich-title|<\macro|notes|body>
    <\with|par-columns|1>
      <\surround||<with|par-columns|3|<arg|notes>>>
        <\doc-make-title>
          <arg|body>
        </doc-make-title>
      </surround>
    </with>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|stem-doc-id|1BBE8D65-51A6-4730-888B-09E3E033E540>
  </collection>
</initial>