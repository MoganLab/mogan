<TeXmacs|2.1.4>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|bash|1.0>

    <\src-purpose>
      Bash Language
    </src-purpose>
  </src-title>>

  <use-module|(data bash)> <use-module|(bash bash-edit)>

  <assign|bash|<macro|body|<with|mode|prog|prog-language|bash|font-family|rm|<arg|body>>>>

  <assign|bash-code|<\macro|body>
    <\pseudo-code>
      <bash|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>