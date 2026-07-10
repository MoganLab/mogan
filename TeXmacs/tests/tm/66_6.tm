<TeXmacs|2.1.2>

<style|<tuple|generic|no-page-numbers|chinese|python>>

<\body>
  <\session|scheme|default>
    <\unfolded-io|Scheme] >
      (stree-\<gtr\>tree `(frac 1 2))
    <|unfolded-io>
      <text|<frac|1|2>>
    </unfolded-io>

    <\input|Scheme] >
      \;
    </input>
  </session>

  <\session|python|default>
    <\output>
      Python 3.12.2 [/usr/bin/python3]

      Python plugin for TeXmacs.

      Please see the documentation in Help -\<gtr\> Plugins -\<gtr\> Python
    </output>

    <\input>
      \<gtr\>\<gtr\>\<gtr\>\ 
    <|input>
      from tmpy.protocol import flush_any
    </input>

    <\unfolded-io>
      \<gtr\>\<gtr\>\<gtr\>\ 
    <|unfolded-io>
      flush_any ("scheme:(frac 1 2)")
    <|unfolded-io>
      <frac|1|2>
    </unfolded-io>

    <\unfolded-io>
      \<gtr\>\<gtr\>\<gtr\>\
    <|unfolded-io>
      flush_any ("scheme:(frac \<#5206\>\<#5B50\> \<#5206\>\<#6BCD\>)")
    <|unfolded-io>
      <frac|\<#5206\>\<#5B50\>|\<#5206\>\<#6BCD\>>
    </unfolded-io>

    <\input>
      \<gtr\>\<gtr\>\<gtr\>\ 
    <|input>
      \;
    </input>
  </session>

  \;
</body>

<\initial>
  <\collection>
    <associate|page-medium|paper>
    <associate|page-screen-margin|false>
  </collection>
</initial>