/**
 * @file ev_dash_typography.c
 */

#include "ev_dash_typography.h"
#include "ev_dash_gen.h"

int32_t ev_dash_letter_space_for_font(const lv_font_t * font)
{
    if(font == font_display) return FONT_LETTER_SPACE_DISPLAY;
    if(font == font_heading) return FONT_LETTER_SPACE_HEADING;
    if(font == font_subhead) return FONT_LETTER_SPACE_SUBHEAD;
    if(font == font_body)    return FONT_LETTER_SPACE_BODY;
    if(font == font_small)   return FONT_LETTER_SPACE_SMALL;
    return 0;
}

static void apply_letter_spacing_obj(lv_obj_t * obj)
{
    const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
    if(font != NULL) {
        lv_obj_set_style_text_letter_space(obj, ev_dash_letter_space_for_font(font), LV_PART_MAIN);
    }

    uint32_t i;
    uint32_t count = lv_obj_get_child_count(obj);
    for(i = 0; i < count; i++) {
        apply_letter_spacing_obj(lv_obj_get_child(obj, i));
    }
}

void ev_dash_apply_letter_spacing(lv_obj_t * root)
{
    if(root == NULL) return;
    apply_letter_spacing_obj(root);
}
