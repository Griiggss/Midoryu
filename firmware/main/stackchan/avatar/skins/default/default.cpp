/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "default.h"
#include <esp_timer.h>
#include <algorithm>
#include <cstring>

using namespace uitk::lvgl_cpp;
using namespace stackchan::avatar;

static const int kRenderIntervalMs = 66;  // ~15 fps cap: keeps the LVGL lock free for audio/network

DefaultAvatar::~DefaultAvatar()
{
    // Children (canvas) die with the panel; free the pixel buffer after the canvas is gone
    _key_elements.speechBubble.reset();
    _pannel.reset();
    if (_draw_buf) {
        lv_draw_buf_destroy(_draw_buf);
        _draw_buf = nullptr;
    }
    _canvas = nullptr;
}

void DefaultAvatar::init(lv_obj_t* parent, const lv_font_t* font)
{
    // Midoryu palette: dark ink bubble with mint text on a mint screen
    primaryColor   = lv_color_make(8, 38, 30);
    secondaryColor = lv_color_make(150, 245, 195);

    _pannel = std::make_unique<Container>(parent);
    _pannel->align(LV_ALIGN_CENTER, 0, 0);
    _pannel->setSize(320, 240);
    _pannel->setRadius(0);
    _pannel->setBorderWidth(0);
    _pannel->setBgColor(secondaryColor);
    _pannel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _pannel->setPadding(0, 0, 0, 0);

    // The dragon face is rendered into a 320x240 RGB565 canvas (2x nearest-neighbour from 160x120 pixels)
    _face     = std::make_unique<DragonFace>();
    _draw_buf = lv_draw_buf_create(320, 240, LV_COLOR_FORMAT_RGB565, 0);
    if (_draw_buf) {
        _canvas = lv_canvas_create(_pannel->get());
        lv_canvas_set_draw_buf(_canvas, _draw_buf);
        lv_obj_set_pos(_canvas, 0, 0);
        lv_obj_remove_flag(_canvas, LV_OBJ_FLAG_CLICKABLE);  // let touches reach the panel
        lv_obj_remove_flag(_canvas, LV_OBJ_FLAG_SCROLLABLE);
    }

    _key_elements.leftEye  = std::make_unique<DefaultEyes>(_pannel->get(), primaryColor, secondaryColor, true);
    _key_elements.rightEye = std::make_unique<DefaultEyes>(_pannel->get(), primaryColor, secondaryColor, false);
    _key_elements.mouth    = std::make_unique<DefaultMouth>(_pannel->get(), primaryColor, secondaryColor);
    _key_elements.speechBubble =
        std::make_unique<DefaultSpeechBubble>(_pannel->get(), primaryColor, secondaryColor, font);

    render_face((uint32_t)(esp_timer_get_time() / 1000));
}

Container* DefaultAvatar::getPanel() const
{
    if (_pannel) {
        return _pannel.get();
    }
    return NULL;
}

void DefaultAvatar::update()
{
    Avatar::update();

    if (!_face || !_canvas) {
        return;
    }
    render_face((uint32_t)(esp_timer_get_time() / 1000));
}

void DefaultAvatar::render_face(uint32_t now)
{
    auto& l = leftEye();
    auto& r = rightEye();
    auto& m = mouth();

    auto open_of = [](Feature& f) {
        // Weight 100 = open, ~25 = blink. Map 20..100 to 0..1
        float v = (f.getWeight() - 20) / 80.0f;
        return std::min(1.0f, std::max(0.0f, v));
    };
    // Same range as the stock avatar: position -100..100 moves a part by +-16 px
    auto off = [](int v) { return v * 16.0f / 100.0f; };

    DragonFace::Input in;
    in.mood         = (int)_emotion;
    in.eyeOpenL     = open_of(l);
    in.eyeOpenR     = open_of(r);
    in.eyeVisibleL  = l.getVisible();
    in.eyeVisibleR  = r.getVisible();
    in.eyeOffLX     = off(l.getPosition().x);
    in.eyeOffLY     = off(l.getPosition().y);
    in.eyeOffRX     = off(r.getPosition().x);
    in.eyeOffRY     = off(r.getPosition().y);
    in.eyeSizeL     = l.getSize() / 100.0f;
    in.eyeSizeR     = r.getSize() / 100.0f;
    in.mouthOpen    = m.getWeight() / 100.0f;
    in.mouthOffX    = off(m.getPosition().x);
    in.mouthOffY    = off(m.getPosition().y);
    in.mouthVisible = m.getVisible();

    if (_face->update(in, now)) {
        _dirty = true;
    }
    if (!_dirty || (now - _last_render_ms) < (uint32_t)kRenderIntervalMs) {
        return;
    }
    _last_render_ms = now;
    _dirty          = false;

    _face->render();

    // 160x120 indexed -> 320x240 RGB565
    uint16_t lut[DragonFace::kPaletteLen];
    const auto* pal = _face->palette();
    for (int i = 0; i < DragonFace::kPaletteLen; i++) {
        lut[i] = (uint16_t)(((pal[i][0] & 0xF8) << 8) | ((pal[i][1] & 0xFC) << 3) | (pal[i][2] >> 3));
    }
    const uint8_t* idx = _face->indices();
    uint8_t* dst       = (uint8_t*)_draw_buf->data;
    uint32_t stride    = _draw_buf->header.stride;
    for (int y = 0; y < DragonFace::kHeight; y++) {
        uint32_t* row = (uint32_t*)(dst + (size_t)(y * 2) * stride);
        const uint8_t* src = idx + y * DragonFace::kWidth;
        for (int x = 0; x < DragonFace::kWidth; x++) {
            uint32_t c = lut[src[x]];
            row[x]     = c | (c << 16);
        }
        std::memcpy(dst + (size_t)(y * 2 + 1) * stride, row, 320 * sizeof(uint16_t));
    }
    lv_obj_invalidate(_canvas);
}
