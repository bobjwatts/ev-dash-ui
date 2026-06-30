#include "speedometer_assets.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
static const char * SM_ASSETS_TAG = "sm_assets";
#endif

static int image_scale_to_fit(int src_px, int target_px)
{
    if (src_px <= 0 || target_px <= 0) {
        return 256;
    }
    return (target_px * 256) / src_px;
}

void speedometer_image_fit_to_target(lv_obj_t * img,
                                     const lv_image_dsc_t * src,
                                     int target_w,
                                     int target_h,
                                     const char * label)
{
    if (img == NULL || src == NULL) {
        return;
    }

    int scale_x = image_scale_to_fit(src->header.w, target_w);
    int scale_y = image_scale_to_fit(src->header.h, target_h);
    int scale = LV_MIN(scale_x, scale_y);

    if (src->header.w != target_w || src->header.h != target_h) {
        LV_LOG_WARN("%s embedded %dx%d -> display %dx%d (scale=%d, rebuild/flash if unexpected)",
                    label,
                    (int)src->header.w,
                    (int)src->header.h,
                    target_w,
                    target_h,
                    scale);
    }

    lv_image_set_scale(img, scale);
}

void speedometer_needle_set_pivot(lv_obj_t * needle_img, const lv_image_dsc_t * src)
{
    if (needle_img == NULL || src == NULL) {
        return;
    }

    lv_obj_set_style_transform_pivot_x(needle_img, src->header.w / 2, 0);
    lv_obj_set_style_transform_pivot_y(needle_img,
                                       src->header.h - SPEEDOMETER_NEEDLE_PIVOT_BOTTOM,
                                       0);
}

void speedometer_image_set_1to1(lv_obj_t * img, const lv_image_dsc_t * src, const char * label)
{
    if (img == NULL || src == NULL) {
        return;
    }

    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CENTER);
    lv_obj_set_size(img, src->header.w, src->header.h);
    lv_image_set_scale(img, 256);
}

void speedometer_dial_image_set(lv_obj_t * img, const lv_image_dsc_t * src)
{
    if (img == NULL || src == NULL) {
        return;
    }

    /* Native-size embed; widget matches display (439). Source: tools/gen_image_data_lvgl.ps1 */
    speedometer_image_set_1to1(img, src, "dial");
}

#if SPEEDOMETER_DEBUG_SIZE_RINGS

static void debug_size_ring_create(lv_obj_t * parent, int diameter_px, lv_color_t color)
{
    lv_obj_t * ring = lv_obj_create(parent);
    lv_obj_set_size(ring, diameter_px, diameter_px);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_border_color(ring, color, 0);
    lv_obj_set_style_border_opa(ring, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(ring, 0, 0);
    lv_obj_set_align(ring, LV_ALIGN_CENTER);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(ring);
}

static void debug_add_size_rings(lv_obj_t * parent)
{
    debug_size_ring_create(parent, SPEEDOMETER_DIAL_DISPLAY_PX, lv_color_hex(0xFF00FF));
    debug_size_ring_create(parent, SPEEDOMETER_ARC_TRACK_PX, lv_color_hex(0x00FF88));
    debug_size_ring_create(parent, SPEEDOMETER_FACE_DISPLAY_PX, lv_color_hex(0xFFAA00));
}

void speedometer_debug_overlay_rings(lv_obj_t * speedometer)
{
    debug_add_size_rings(speedometer);
}

void speedometer_debug_log_dial_layout(lv_obj_t * speedometer)
{
    lv_obj_t * dial_img = lv_obj_get_child(speedometer, 0);
    if (dial_img == NULL) {
        return;
    }

    lv_area_t img_coords;
    lv_area_t parent_coords;
    lv_point_t pivot;
    lv_obj_get_coords(dial_img, &img_coords);
    lv_obj_get_coords(speedometer, &parent_coords);
    lv_image_get_pivot(dial_img, &pivot);
#ifdef ESP_PLATFORM
    ESP_LOGI(SM_ASSETS_TAG,
             "dial img (%d,%d)-(%d,%d) parent (%d,%d)-(%d,%d) scale=%d pivot=(%d,%d)",
             (int)img_coords.x1, (int)img_coords.y1, (int)img_coords.x2, (int)img_coords.y2,
             (int)parent_coords.x1, (int)parent_coords.y1, (int)parent_coords.x2, (int)parent_coords.y2,
             (int)lv_image_get_scale(dial_img),
             (int)pivot.x, (int)pivot.y);
#else
    LV_LOG_USER("dial img coords (%d,%d)-(%d,%d) parent (%d,%d)-(%d,%d) scale=%d pivot=(%d,%d)",
                (int)img_coords.x1, (int)img_coords.y1, (int)img_coords.x2, (int)img_coords.y2,
                (int)parent_coords.x1, (int)parent_coords.y1, (int)parent_coords.x2, (int)parent_coords.y2,
                (int)lv_image_get_scale(dial_img),
                (int)pivot.x, (int)pivot.y);
#endif
}

void speedometer_debug_layout_screen(lv_obj_t * screen, lv_obj_t * speedometer)
{
    extern const lv_image_dsc_t debug_img_100_data;
    extern const lv_image_dsc_t debug_img_439_data;

    lv_obj_align(speedometer, LV_ALIGN_LEFT_MID, 40, 0);

    lv_obj_t * left_lbl = lv_label_create(screen);
    lv_label_set_text(left_lbl, "dial PNG");
    lv_obj_set_style_text_color(left_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(left_lbl, speedometer, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lv_obj_t * ref = lv_obj_create(screen);
    lv_obj_set_size(ref, SPEEDOMETER_DIAL_DISPLAY_PX, SPEEDOMETER_DIAL_DISPLAY_PX);
    lv_obj_set_style_bg_opa(ref, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ref, 0, 0);
    lv_obj_remove_flag(ref, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ref, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_align(ref, LV_ALIGN_RIGHT_MID, -40, 0);

#if defined(EV_DASH_DEBUG_DRAW_TEST) && EV_DASH_DEBUG_DRAW_TEST
    lv_obj_t * img439 = lv_image_create(ref);
    lv_image_set_src(img439, &debug_img_439_data);
    speedometer_image_set_1to1(img439, &debug_img_439_data, "dbg439");
    lv_obj_set_align(img439, LV_ALIGN_CENTER);

    lv_obj_t * img100 = lv_image_create(ref);
    lv_image_set_src(img100, &debug_img_100_data);
    speedometer_image_set_1to1(img100, &debug_img_100_data, "dbg100");
    lv_obj_set_align(img100, LV_ALIGN_TOP_LEFT);
#endif

    debug_add_size_rings(ref);

    lv_obj_t * right_lbl = lv_label_create(screen);
    lv_label_set_text(right_lbl,
#if defined(EV_DASH_DEBUG_DRAW_TEST) && EV_DASH_DEBUG_DRAW_TEST
                     ""
#else
                     ""
#endif
                     );
    lv_obj_set_style_text_color(right_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(right_lbl, ref, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lv_obj_t * legend = lv_label_create(screen);
    lv_label_set_text(legend,
                      "");
    lv_obj_set_style_text_color(legend, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(legend, LV_ALIGN_BOTTOM_MID, 0, -8);

    lv_obj_update_layout(screen);
    speedometer_debug_log_dial_layout(speedometer);
#if defined(EV_DASH_DEBUG_DRAW_TEST) && EV_DASH_DEBUG_DRAW_TEST
    LV_LOG_USER("draw-test: dbg100=%dx%d dbg439=%dx%d",
                (int)debug_img_100_data.header.w, (int)debug_img_100_data.header.h,
                (int)debug_img_439_data.header.w, (int)debug_img_439_data.header.h);
#endif
}

#endif
