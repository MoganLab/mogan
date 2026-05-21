<TeXmacs|2.1.2>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|tikz|1.0>

    <\src-purpose>
      TikZ Language
    </src-purpose>
  </src-title>>

  <use-module|(data tikz)>
  <use-module|(code tikz-edit)>

  <assign|tikz|<macro|body|<with|mode|prog|prog-language|tikz|font-family|rm|<arg|body>>>>

  <assign|tikz-code|<\macro|body>
    <\pseudo-code>
      <tikz|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>
