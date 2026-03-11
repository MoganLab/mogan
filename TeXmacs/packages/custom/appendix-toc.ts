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

  ;; 附录章节：同时写入主目录(toc)和子目录(appendix-toc)
  <assign|appendix|<\macro|title>
    <style-with|src-compact|none|
      ;; 先写入主目录
      <with|toc-prefix|toc|<appendix-toc|<arg|title>>>
      ;; 再写入子目录
      <with|toc-prefix|appendix-toc|<style-with|src-compact|none|
        <assign|appendix-numbered|<compound|appendix-display-numbers>>
        <assign|appendix-prefix|<macro|<the-appendix>.>>
        <next-appendix>
        <appendix-clean>
        <appendix-header|<arg|title>>
        <appendix-toc|<arg|title>>
        <if|<value|appendix-numbered>|<appendix-numbered-title|<arg|title>>|<appendix-unnumbered-title|<arg|title>>>
      >>
    >
  </macro>>

  <assign|appendix-section|<\macro|title>
    <with|toc-prefix|appendix-toc|<section|<arg|title>>>
  </macro>>

  <assign|appendix-subsection|<\macro|title>
    <with|toc-prefix|appendix-toc|<subsection|<arg|title>>>
  </macro>>

  <assign|appendix-subsubsection|<\macro|title>
    <with|toc-prefix|appendix-toc|<subsubsection|<arg|title>>>
  </macro>>

  <assign|appendix-table-of-contents|<macro|<\table-of-contents|appendix-toc>
    \;
  </table-of-contents>>>

  <assign|with-subtoc|<\macro|prefix|body>
    <with|toc-prefix|<arg|prefix>|<arg|body>>
  </macro>>
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>