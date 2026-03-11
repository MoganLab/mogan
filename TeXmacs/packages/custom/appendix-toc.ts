<TeXmacs|2.1.4>

<style|<tuple|source|std|section-base>>

<\body>
  <active*|<\src-title>
    <src-package|appendix-toc|1.0>

    <\src-purpose>
      Support for independent table of contents in appendix sections. This
      package allows creating a separate table of contents that only includes
      appendix chapters, sections, and subsections.
    </src-purpose>

    <src-copyright|2026|Soyo and Yuki>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <assign|appendix-section|<\macro|title>
    <section|<arg|title>><with|toc-prefix|appendix-toc|<toc-main-2|<the-section><section-sep><arg|title>>>
  </macro>>

  <assign|appendix-subsection|<\macro|title>
    <subsection|<arg|title>><with|toc-prefix|appendix-toc|<toc-normal-2|<the-subsection><subsection-sep><arg|title>>>
  </macro>>

  <assign|appendix-subsubsection|<\macro|title>
    <subsubsection|<arg|title>><with|toc-prefix|appendix-toc|<toc-normal-3|<the-subsubsection><subsubsection-sep><arg|title>>>
  </macro>>

  <assign|with-subtoc|<\macro|prefix|body>
    <with|toc-prefix|<arg|prefix>|<arg|body>>
  </macro>>

  <assign|table-of-contents|<\macro|aux|body>
    <render-table-of-contents|<if|<value|in-appendix>|<appendix-table-of-contents-text>|<table-of-contents-text>>|<arg|body>>
  </macro>>

  <assign|render-table-of-contents|<\macro|name|body>
    <with|chapter-toc|<macro|name|>|section-toc|<macro|name|>|<section*|<arg|name>>>

    <with|par-first|0fn|par-par-sep|0fn|<arg|body>>
  </macro>>

</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>
