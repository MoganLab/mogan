<TeXmacs|1.99.16>

<style|<tuple|source|std>>

<\body>
  <active*|<\src-title>
    <src-package|shortcut-editor|1.0>

    <\src-purpose>
      Internal style package for editing keyboard shortcuts.
    </src-purpose>

    <src-copyright|2020|Joris van der Hoeven>

    <\src-license>
      This software falls under the <hlink|GNU general public license,
      version 3 or later|$TEXMACS_PATH/LICENSE>. It comes WITHOUT ANY
      WARRANTY WHATSOEVER. You should have received a copy of the license
      which the software. If not, see <hlink|http://www.gnu.org/licenses/gpl-3.0.html|http://www.gnu.org/licenses/gpl-3.0.html>.
    </src-license>
  </src-title>>

  <use-module|(doc tmdoc-markup)>

  <use-module|(source shortcut-edit)>

  <\active*>
    <\src-comment>
      Special tag for keyboard shortcut editing.
    </src-comment>
  </active*>

  <assign|shortcut-editor-field-color|<if|<equal|<value|color>|white>|#2c2c2c|white>>

  <assign|shortcut-editor-light-border|<if|<equal|<value|color>|white>|#e0e0e0|#b0b0b0>>

  <assign|shortcut-editor-dark-border|<if|<equal|<value|color>|white>|#e0e0e0|#707070>>

  <assign|bg-color|<value|shortcut-editor-field-color>>

  <assign|canvas-color|<value|shortcut-editor-field-color>>

  <assign|shortcut-key-ornament-color|<value|shortcut-editor-field-color>>

  <assign|shortcut-key-sunny-color|<value|shortcut-editor-light-border>>

  <assign|shortcut-key-shadow-color|<value|shortcut-editor-dark-border>>

  <assign|shortcut-key-border|<if|<equal|<value|color>|white>|1ln|2ln>>

  <assign|render-key|<macro|key|<active*|<move|<small|<with|font-family|tt|<with|ornament-color|<value|shortcut-key-ornament-color>|ornament-sunny-color|<value|shortcut-key-sunny-color>|ornament-shadow-color|<value|shortcut-key-shadow-color>|ornament-hpadding|2ln|ornament-vpadding|2ln|ornament-border|<value|shortcut-key-border>|<ornament|<compound|inflate|<arg|key>>>>>>||0.075ex>>>>

  <assign|render-keys|<macro|keys|<extern|tmdoc-render-keys|<quote-arg|keys>>>>

  <assign|preview-shortcut|<macro|shortcut|<if|<equal|<arg|shortcut>|>|<arg|shortcut>|<extern|tmdoc-key|<quote-arg|shortcut>>>>>

  <drd-props|preview-shortcut|arity|1>

  \;
</body>

<\initial>
  <\collection>
    <associate|preamble|true>
  </collection>
</initial>

