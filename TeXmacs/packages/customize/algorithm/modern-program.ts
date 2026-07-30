<TeXmacs|2.1.4>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <src-package|modern-program|1.0>

    <\src-purpose>
      An algorithm style similar to LaTeX, modern style
    </src-purpose>

    <src-copyright|2025|Yuki Lu>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <\active*>
    <\src-comment>
      Extra style parameters
    </src-comment>
  </active*>

  <assign|intro-color|>

  <assign|body-color|>

  <\active*>
    <\src-comment>
      Modern programs
    </src-comment>
  </active*>

  \;

  <\active*>
    <\src-comment>
      Customizations of algorithmic environments
    </src-comment>
  </active*>

  <assign|render-code|<\macro|body>
    <\surround||<if|<value|indent-after-block>||<no-indent*>>>
      <\padded*>
        <\indent>
          <\with|par-first|0fn|par-par-sep|0fn|item-hsep|<macro|1tab>|numbered-offset|<value|code-numbered-offset>>
            <arg|body>
          </with>
        </indent>
      </padded*>
    </surround>
  </macro>>

  <assign|render-remark|<\macro|which|body>
    <render-enunciation|<remark-name|<arg|which> >|<arg|body>>
  </macro>>

  <assign|render-algorithm|<\macro|name|body>
    <\padded*>
      <\with|par-first|0fn|par-par-sep|0fn|item-hsep|<macro|1tab>|numbered-offset|<value|algorithm-numbered-offset>>
        <render-remark|<algorithm-name|<arg|name>>|>

        <\surround||<yes-indent*>>
          <\algorithm-indent>
            <arg|body>
          </algorithm-indent>
        </surround>
      </with>
    </padded*>
  </macro>>

  <assign|render-specified-algorithm|<\macro|name|intro|body>
    <\padded*>
      <\with|par-first|0fn|par-par-sep|0fn|item-hsep|<macro|1tab>|numbered-offset|<value|algorithm-numbered-offset>>
        <render-remark|<algorithm-name|<arg|name>>|<arg|intro>>

        <\surround||<yes-indent*>>
          <\algorithm-indent>
            <arg|body>
          </algorithm-indent>
        </surround>
      </with>
    </padded*>
  </macro>>

  <assign|pseudo-code|<\macro|body>
    <\render-code>
      <arg|body>
    </render-code>
  </macro>>

  \;
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>