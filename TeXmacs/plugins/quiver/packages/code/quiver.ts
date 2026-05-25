<TeXmacs|2.1.2>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|quiver|1.0>

    <\src-purpose>
      Quiver Language
    </src-purpose>
  </src-title>>

  <use-module|(data quiver)>
  <use-module|(code quiver-edit)>

  <assign|quiver|<macro|body|<with|mode|prog|prog-language|quiver|font-family|rm|<arg|body>>>>

  <assign|quiver-code|<\macro|body>
    <\pseudo-code>
      <quiver|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>
