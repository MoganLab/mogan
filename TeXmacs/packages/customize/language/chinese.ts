<TeXmacs|2.1.4>

<style|source>

<\body>
  <active*|<\src-title>
    <src-package|chinese|1.0>

    <\src-purpose>
      A style package for chinese language support
    </src-purpose>

    <\src-copyright|2013>
      Joris van der Hoeven
    </src-copyright>

    <\src-copyright|2020-2021>
      Darcy Shen
    </src-copyright>

    <\src-copyright|2023>
      Jingkaimori
    </src-copyright>

    <\src-copyright|2025>
      Yuki Lu
    </src-copyright>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <assign|language|chinese>

  <assign|font|sys-chinese>

  <use-module|(lang chinese-kbd)>

  <if|<greater|<value|par-first>|0fn>|<assign|par-first|2fn>>

  <assign|par-spacing|kaiming>

  <\active*>
    <\src-comment>
      Document in Chinese should indent every paragraph, though some
      mathematics documents in Latin align the first paragraph in section to
      the title of section.
    </src-comment>
  </active*>

  <assign|sectional-normal|<macro|name|<wide-normal|<arg|name><no-page-break>>>>

  <assign|sectional-centered|<macro|name|<wide-centered|<arg|name><no-page-break>>>>

  <\active*>
    <\src-comment>
      Customize doc-data macro
    </src-comment>
  </active*>

  <assign|by-text|>

  <\active*>
    <\src-comment>
      For Algorithm and Pseudo code blocks, do not translate the underlying
      keywords when documentation language is Chinese.

      \;

      Affected:

      Insert -\<gtr\> Program<compound|math|><compound|math|> -\<gtr\>
      Algorithm

      Insert -\<gtr\> Program -\<gtr\> Pseudo code
    </src-comment>
  </active*>

  <assign|render-keyword|<macro|name|<with|font-series|bold|<arg|name>>>>

  <assign|dfn|<macro|body|<with|font-series|bold|math-font-series|bold|<arg|body>>>>

  <assign|cite*|<macro|body|<arg|body>>>

  <assign|render-theorem|<\macro|which|body>
    <render-enunciation|<theorem-name|<arg|which><theorem-sep>>|<arg|body>>
  </macro>>

  \;

  <assign|ordinal-text|\<#7B2C\>>

  <assign|the-chapter-hanzi|<macro|<number|<value|chapter-nr>|hanzi>>>

  <assign|chapter-numbered-title|<macro|title|<style-with|src-compact|none|<chapter-title|<ordinal-text>
  <the-chapter> <chapter-text>><new-line><chapter-title-render|<huge|<arg|title>>>>>>

  <assign|chapter-toc|<macro|name|<style-with|src-compact|none|<if|<sectional-short-style>|<toc-main-1|<toc-title|chapter|<arg|name>>>|<toc-main-2|<if|<chapter-numbered>|<ordinal-text>
  <the-chapter> <chapter-text><chapter-sep><arg|name>|<arg|name>>>>>>>

  \;

  <assign|part-numbered-title|<macro|name|<part-title-sub|<htab|0fn><ordinal-text>
  <the-part> <part-text><htab|0fn><vspace|0.1pag><new-line><htab|0fn><arg|name><htab|0fn>>>>
</body>

<initial|<\collection>
</collection>>