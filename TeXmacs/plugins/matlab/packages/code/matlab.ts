<TeXmacs|2.1.2>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|matlab|1.0>

    <\src-purpose>
      matlab Language
    </src-purpose>
  </src-title>>

  <use-module|(data matlab)>
  <use-module|(matlab matlab-edit)>

  <assign|matlab|<macro|body|<with|mode|prog|prog-language|matlab|font-family|rm|<arg|body>>>>

  <assign|matlab-code|<\macro|body>
    <\pseudo-code>
      <matlab|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>

