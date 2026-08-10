<TeXmacs|2.1.4>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|r|1.0>

    <\src-purpose>
      r Language
    </src-purpose>
  </src-title>>

  <use-module|(data r)> <use-module|(r r-edit)>

  <assign|r|<macro|body|<with|mode|prog|prog-language|r|font-family|rm|<arg|body>>>>

  <assign|r-code|<\macro|body>
    <\pseudo-code>
      <r|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>