# Speedometer image assets

## Source PNGs (committed)

| File | Role |
|---|---|
| `sc_dial_speed_face.png` | Dial face artwork |
| `sc_dial_speed_face_border.png` | Outer ring / border |
| `sc_dial_speed_needle.png` | Needle sprite (23×191) |

## Generated embeds (gitignored — regenerate after clone)

```powershell
.\tools\gen_image_data_lvgl.ps1
```

Produces:

- `dial_speed_dial_data.c` — composited 439×439 face + border
- `dial_speed_needle_data.c` — needle bitmap
- `ev_dash_assets.h` — embed metadata (`EV_DASH_ASSETS_ID`, sizes, colour format)

Debug solids (optional): `.\tools\gen_debug_draw_test.ps1` → `debug_img_*_data.c`

See the [root README](../../README.md) for the full asset pipeline and known P4 draw issue.
