<TeXmacs|2.1.4>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <src-package|physics|1.1.0>

    <\src-purpose>
      This package contains macros for physics
    </src-purpose>

    <src-copyright|2026|Darcy Shen and (Jack) Yansong Li>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <use-module|(contrib physics physics-drd)>

  <active*|<\src-comment>
    Braket operators
  </src-comment>>

  <assign|bra|<macro|x|<around*|\<langle\>|<arg|x>|\|>>>

  <assign|bra*|<macro|x|\<langle\><arg|x>\|>>

  <assign|ket|<macro|x|<around*|\||<arg|x>|\<rangle\>>>>

  <assign|ket*|<macro|x|\|<arg|x>\<rangle\>>>

  <assign|braket|<macro|var1|var2|<around*|\<langle\>|<arg|var1><mid|\|><arg|var2>|\<rangle\>>>>

  <assign|braket*|<macro|var1|var2|\<langle\><arg|var1>\|<arg|var2>\<rangle\>>>

  <assign|comm|<macro|var1|var2|<around*|[|<arg|var1>,<arg|var2>|]>>>

  <assign|comm*|<macro|var1|var2|[<arg|var1>,<arg|var2>]>>

  <assign|commutator|<macro|var1|var2|<around*|[|<arg|var1>,<arg|var2>|]>>>

  <assign|commutator*|<macro|var1|var2|[<arg|var1>,<arg|var2>]>>

  <assign|acomm|<macro|var1|var2|<around*|{|<arg|var1>,<arg|var2>|}>>>

  <assign|acomm*|<macro|var1|var2|{<arg|var1>,<arg|var2>}>>

  <assign|anticommutator|<macro|var1|var2|<around*|{|<arg|var1>,<arg|var2>|}>>>

  <assign|anticommutator*|<macro|var1|var2|{<arg|var1>,<arg|var2>}>>

  <assign|mel|<macro|var1|var2|var3|<around*|\<langle\>|<arg|var1><mid|\|><arg|var2><mid|\|><arg|var3>|\<rangle\>>>>

  <assign|mel*|<macro|var1|var2|var3|\<langle\><arg|var1>\|<arg|var2>\|<arg|var3>\<rangle\>>>

  \;

  <active*|<\src-comment>
    vectors
  </src-comment>>

  <assign|vb|<macro|var|<with|font-series|bold|<arg|var>>>>

  <assign|vb*|<macro|var|<with|font-series|bold|<arg|var>>>>

  <assign|vu|<macro|var|<tabular|<tformat|<cwith|1|1|1|1|cell-halign|c>|<cwith|1|1|1|1|cell-tsep|0sep>|<cwith|1|1|1|1|cell-rsep|0spc>|<cwith|1|1|1|1|cell-lsep|0spc>|<cwith|1|1|1|1|cell-valign|b>|<cwith|1|1|1|1|cell-bsep|-9sep>|<cwith|2|2|1|1|cell-tsep|0sep>|<cwith|2|2|1|1|cell-valign|b>|<cwith|2|2|1|1|cell-bsep|-2sep>|<table|<row|<cell|^>>|<row|<cell|<with|font-series|bold|<arg|var>>>>>>>>>

  <assign|va|<macro|var|<tabular|<tformat|<cwith|1|1|1|1|cell-halign|c>|<cwith|1|1|1|1|cell-tsep|0sep>|<cwith|1|1|1|1|cell-rsep|0spc>|<cwith|1|1|1|1|cell-lsep|0spc>|<cwith|1|1|1|1|cell-valign|b>|<cwith|1|1|1|1|cell-bsep|-9sep>|<cwith|2|2|1|1|cell-tsep|0sep>|<cwith|2|2|1|1|cell-valign|b>|<cwith|2|2|1|1|cell-bsep|-2sep>|<table|<row|<cell|\<vect\>>>|<row|<cell|<with|font-series|bold|<arg|var>>>>>>>>>
</body>

<initial|<\collection>
</collection>>