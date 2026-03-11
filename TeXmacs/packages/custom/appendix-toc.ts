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

    <src-copyright|2026|Soyo>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <assign|appendix|<\macro|title>
    <style-with|src-compact|none|<with|toc-prefix|toc|<appendix-toc|<arg|title>>><assign|appendix-numbered|<compound|appendix-display-numbers>><assign|appendix-prefix|<macro|<the-appendix>.>><next-appendix><appendix-clean><appendix-header|<arg|title>><if|<value|appendix-numbered>|<appendix-numbered-title|<arg|title>>|<appendix-unnumbered-title|<arg|title>>>>
  </macro>>

  <assign|appendix-section|<\macro|title>
    <style-with|src-compact|none|<with|toc-prefix|appendix-toc|in-appendix|false|<appendix-section-toc|<arg|title>>><section|<arg|title>>>
  </macro>>

  <assign|appendix-subsection|<\macro|title>
    <style-with|src-compact|none|<with|toc-prefix|appendix-toc|in-appendix|false|<appendix-subsection-toc|<arg|title>>><subsection|<arg|title>>>
  </macro>>

  <assign|appendix-subsubsection|<\macro|title>
    <style-with|src-compact|none|<with|toc-prefix|appendix-toc|in-appendix|false|<appendix-subsubsection-toc|<arg|title>>><subsubsection|<arg|title>>>
  </macro>>

  <assign|appendix-section-toc|<macro|name|<toc-main-2|<style-with|src-compact|none|<the-section><section-sep><arg|name>>>>>>

  <assign|appendix-subsection-toc|<macro|name|<toc-normal-2|<style-with|src-compact|none|<the-subsection><subsection-sep><arg|name>>>>>>

  <assign|appendix-subsubsection-toc|<macro|name|<toc-normal-3|<style-with|src-compact|none|<the-subsubsection><subsubsection-sep><arg|name>>>>>>

  <assign|with-subtoc|<\macro|prefix|body>
    <with|toc-prefix|<arg|prefix>|<arg|body>>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>