#ifndef SPEEDOMETER_ASSETS_H
#define SPEEDOMETER_ASSETS_H

#include "lvgl.h"
#include "../images/ev_dash_assets.h"

/* On-screen vs embedded dial size (see tools/gen_image_data.ps1 -DialEmbedPx). */
#define SPEEDOMETER_DIAL_DISPLAY_PX      EV_DASH_DIAL_DISPLAY_W
#define SPEEDOMETER_DIAL_EMBED_PX        EV_DASH_DIAL_EMBED_W
#define SPEEDOMETER_FACE_DISPLAY_PX      409
#define SPEEDOMETER_ARC_TRACK_PX           380
#define SPEEDOMETER_NEEDLE_DISPLAY_W     23
#define SPEEDOMETER_NEEDLE_DISPLAY_H     191
#define SPEEDOMETER_NEEDLE_PIVOT_BOTTOM  38

/* Overlay concentric vector rings on screen_main to compare PNG scale on device. */
#ifndef SPEEDOMETER_DEBUG_SIZE_RINGS
#define SPEEDOMETER_DEBUG_SIZE_RINGS     1
#endif

/* 1 = one centred speedometer only (no side-by-side debug panel). */
#ifndef SPEEDOMETER_DEBUG_SIMPLE_CENTER
#define SPEEDOMETER_DEBUG_SIMPLE_CENTER  1
#endif

/* 1 = replace dial art with solid 439x439 RGB565 (tools/gen_debug_draw_test.ps1). */
#ifndef SPEEDOMETER_DEBUG_DIAL_SOLID
#define SPEEDOMETER_DEBUG_DIAL_SOLID     1
#endif

#ifndef SPEEDOMETER_DEBUG_HIDE_READOUT
#define SPEEDOMETER_DEBUG_HIDE_READOUT   0
#endif

/**
 * Scale an lv_image to the target pixel size and log if the embedded asset
 * differs (stale firmware / old large PNG bake still linked).
 */
void speedometer_image_fit_to_target(lv_obj_t * img,
                                     const lv_image_dsc_t * src,
                                     int target_w,
                                     int target_h,
                                     const char * label);

void speedometer_needle_set_pivot(lv_obj_t * needle_img, const lv_image_dsc_t * src);

/** Configure lv_image for 1:1 pixel draw at native embed size. */
void speedometer_image_set_1to1(lv_obj_t * img, const lv_image_dsc_t * src, const char * label);

/**
 * Dial setup after tools/gen_image_data_lvgl.ps1 (official LVGLImage.py embed).
 */
void speedometer_dial_image_set(lv_obj_t * img, const lv_image_dsc_t * src);

#if SPEEDOMETER_DEBUG_SIZE_RINGS
/** Overlay vector rings on top of PNG layers inside the speedometer widget. */
void speedometer_debug_overlay_rings(lv_obj_t * speedometer);

/** Side-by-side layout: PNG speedometer left, draw-test + vector reference right. */
void speedometer_debug_layout_screen(lv_obj_t * screen, lv_obj_t * speedometer);

/** Log dial lv_image layout after screen build (serial monitor). */
void speedometer_debug_log_dial_layout(lv_obj_t * speedometer);
#endif

#endif
