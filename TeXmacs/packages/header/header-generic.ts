<TeXmacs|2.1.4>

<style|source>

<\body>
  <active*|<\src-title>
    <src-package-dtd|header-generic|1.0|header|1.0>

    <\src-purpose>
      Headers for the generic style.
    </src-purpose>

    <src-copyright|1998--2004|Joris van der Hoeven>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <assign|page-odd-footer|<tabular*|<tformat|<twith|table-width|1par>|<twith|table-hmode|exact>|<cwith|1|1|1|1|cell-width|0.3par>|<cwith|1|1|1|1|cell-hmode|exact>|<cwith|1|1|3|3|cell-width|0.3par>|<cwith|1|1|3|3|cell-hmode|exact>|<cwith|1|1|2|2|cell-width|0.4par>|<cwith|1|1|2|2|cell-hmode|exact>|<cwith|1|1|1|1|cell-halign|l>|<cwith|1|1|3|3|cell-halign|r>|<cwith|1|-1|1|-1|cell-hyphen|t>|<table|<row|<cell|>|<cell|<page-number>>|<cell|>>>>>>

  <assign|page-even-footer|<tabular*|<tformat|<twith|table-width|1par>|<twith|table-hmode|exact>|<cwith|1|1|1|1|cell-width|0.3par>|<cwith|1|1|1|1|cell-hmode|exact>|<cwith|1|1|3|3|cell-width|0.3par>|<cwith|1|1|3|3|cell-hmode|exact>|<cwith|1|1|2|2|cell-width|0.4par>|<cwith|1|1|2|2|cell-hmode|exact>|<cwith|1|1|1|1|cell-halign|l>|<cwith|1|1|3|3|cell-halign|r>|<cwith|1|-1|1|-1|cell-hyphen|t>|<table|<row|<cell|>|<cell|<page-number>>|<cell|>>>>>>

  \;

  <assign|start-page|<macro|s|>>

  <assign|odd-page-text|<macro|s|>>

  <assign|even-page-text|<macro|s|>>

  \;

  <assign|header-title|<macro|name|>>

  <assign|header-author|<macro|name|>>

  <assign|header-primary|<macro|name|nr|what|>>

  <assign|header-secondary|<macro|name|nr|what|>>

  \;
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>