<TeXmacs|2.1.4>

<style|source>

<\body>
  <active*|<\src-title>
    <src-package|llm|1.0>

    <\src-purpose>
      Markup for LLM sessions.
    </src-purpose>

    <\src-copyright|2021>
      Joris van der Hoeven

      \ \ \ \ 2024 by Darcy Shen

      \ \ \ \ 2025 by Jack Yansong Li
    </src-copyright>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <use-package|session>

  <assign|session|<\macro|language|session|body>
    <\with|prog-language|<arg|language>|prog-session|<arg|session>>
      <render-session|<arg|body>>

      \;
    </with>
  </macro>>

  \;

  <assign|llm-input-bg-color|<macro|<if|<equal|<value|color>|white>|#9ba8c2|#f0f4ff>>>

  <assign|llm-prompt-color|#4d6cff>

  <assign|llm-input-color|<macro|<if|<equal|<value|color>|white>|#242938|dark blue>>>

  <\active*>
    <\src-comment>
      Input field with background color
    </src-comment>
  </active*>

  <macro|indent-both|<\macro|left-indentation|right-indentation|body>
    <with|par-left|<plus|<value|par-left>|<arg|left-indentation>>|par-right|<plus|<value|par-right>|<arg|right-indentation>>|<arg|body>>
  </macro>>

  <assign|llm-indent|<macro|<if|<greater|1par|20cm>|0.1par|0par>>>

  <assign|llm-input|<\macro|prompt|body>
    <\indent-both|<llm-indent>|<llm-indent>>
      <\with|ornament-shape|classic|ornament-color|<llm-input-bg-color>|ornament-border|0ln|ornament-vpadding|0.3fn>
        <\ornament>
          <tabular|<tformat|<twith|table-width|1par>|<cwith|1|1|2|2|cell-hpart|1>|<cwith|1|1|1|1|cell-lsep|0fn>|<cwith|1|1|1|1|cell-rsep|0fn>|<cwith|1|1|2|2|cell-lsep|0fn>|<cwith|1|1|2|2|cell-rsep|0fn>|<cwith|1|1|2|2|cell-hyphen|t>|<twith|table-hyphen|y>|<table|<row|<cell|<id-function|<with|color|<value|llm-prompt-color>|<arg|prompt>>>>|<\cell>
            <with|color|<llm-input-color>|math-display|true|<arg|body>>
          </cell>>>>>
        </ornament>
      </with>
    </indent-both>
  </macro>>

  <\active*>
    <\src-comment>
      Use verbatim output
    </src-comment>
  </active*>

  <assign|llm-output|<\macro|body>
    <\indent-both|<llm-indent>|<llm-indent>>
      <\with|info-flag|none|font-family|CMU>
        <\generic-output>
          <text|<arg|body>>
        </generic-output>
      </with>
    </indent-both>
  </macro>>

  <assign|llm-errput|<\macro|body>
    <\with|mode|text|language|verbatim|font-family|CMU>
      <\generic-errput>
        <arg|body>
      </generic-errput>
    </with>
  </macro>>

  <assign|llm-thinking-dots|<macro|<anim-repeat|<anim-compose|<anim-constant||0.35sec>|<anim-constant|.|0.35sec>|<anim-constant|..|0.35sec>|<anim-constant|...|0.35sec>>>>>

  <assign|script-busy|<macro|msg|<script-status|<if|<equal|<arg|msg>|<uninit>>|<concat|<localize|Thinking>|<llm-thinking-dots>>|<arg|msg>>>>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>
