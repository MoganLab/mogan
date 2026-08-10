<TeXmacs|2.1.4>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <compound|src-package|sql|1.0>

    <\src-purpose>
      SQL Language support for TeXmacs
    </src-purpose>
  </src-title>>

  <use-module|(data sql)>
  <use-module|(sql sql-edit)>

  <assign|sql|<macro|body|<with|mode|prog|prog-language|sql|font-family|rm|<arg|body>>>>

  <assign|sql-code|<\macro|body>
    <\pseudo-code>
      <sql|<arg|body>>
    </pseudo-code>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
    <associate|sfactor|5>
  </collection>
</initial>