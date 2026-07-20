Big Shoulders Bold (`BigShoulders-Bold.ttf`) — role fonts at 12 / 16 / 18 / 25 / 54 px.

Glyph range: ASCII + `°` (U+00B0) + blinker chevrons `‹`/`›` (U+2039/U+203A). Figma uses vector triangles for blinkers; Big Shoulders has no ◄/► glyphs.

Regenerate with:

```powershell
.\tools\gen_fonts.ps1
```

(`lved-cli generate ui -ss` also rebuilds fonts but only ASCII; re-run `gen_fonts.ps1` afterward.)

Letter spacing (10% of nominal size) is applied at runtime via `ev_dash_apply_letter_spacing()`.
