<TeXmacs|2.1.2>

<style|<tuple|source|std|section-base>>

<\body>
  <active*|<\src-title>
    <src-package|appendix-toc|1.0>

    <\src-purpose>
      Support for independent table of contents in appendix sections.
      This package allows creating a separate table of contents that only
      includes appendix chapters, sections, and subsections.
    </src-purpose>

    <src-copyright|2026|Soyo>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <\active*>
    <\src-comment>
      Appendix table of contents support.
      
      Usage:
      1. Add <use-package|appendix-toc> to your document
      2. Use <appendix|title> to start appendix sections
      3. Use <appendix-table-of-contents> to display appendix-only TOC
      
      The appendix sections will be written to "appendix-toc" prefix
      instead of the default "toc" prefix, creating an independent TOC.
    </src-comment>
  </active*>

  ;; Redefine appendix to use "appendix-toc" prefix for TOC entries
  <assign|appendix|<macro|title|
    <with|toc-prefix|appendix-toc|
      <style-with|src-compact|none|
        <assign|appendix-numbered|<value|section-display-numbers>>
        <assign|appendix-prefix|<macro|<compound|<if|<sectional-short-style>|the-section|the-chapter>>>>
        <next-appendix>
        <appendix-clean>
        <appendix-header>|<arg|title>>
        ;; Write to appendix-toc instead of default toc
        <with|toc-prefix|appendix-toc|<appendix-toc>|<arg|title>>>
        <style-with|src-compact|none|
          <if|<value|appendix-numbered>|
            <appendix-numbered-title>|<arg|title>>
            <appendix-unnumbered-title>|<arg|title>>
          >
        >
      >
    >
  >>

  ;; Appendix section - also uses appendix-toc prefix
  <assign|appendix-section|<macro|title|
    <with|toc-prefix|appendix-toc|
      <section|<arg|title>>
    >
  >>

  ;; Appendix subsection - also uses appendix-toc prefix
  <assign|appendix-subsection|<macro|title|
    <with|toc-prefix|appendix-toc|
      <subsection|<arg|title>>
    >
  >>

  ;; Appendix subsubsection - also uses appendix-toc prefix
  <assign|appendix-subsubsection|<macro|title|
    <with|toc-prefix|appendix-toc|
      <subsubsection|<arg|title>>
    >
  >>

  ;; Convenient macro to display appendix table of contents
  <assign|appendix-table-of-contents|<macro|
    <table-of-contents|appendix-toc|<document|>>
  >>

  ;; Alternative: use with-toc to wrap any content with custom TOC prefix
  <assign|with-subtoc|<macro|prefix|body|
    <with|toc-prefix|<arg|prefix>|<arg|body>>
  >>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>
