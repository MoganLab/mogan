<TeXmacs|2.1.2>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|fish|1.0>

    <\src-purpose>
      fish Language
    </src-purpose>
  </src-title>>

  <use-module|(data fish)>
  <use-module|(code fish-edit)>

  <assign|fish|<macro|body|<with|mode|prog|prog-language|fish|font-family|rm|<arg|body>>>>

  <assign|fish-code|<\macro|body>
    <\pseudo-code>
      <fish|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>

