/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "../../avatar/avatar.h"
#include "../../avatar/elements/feature.h"
#include "dragon_face.h"
#include <lvgl.h>
#include <smooth_lvgl.hpp>
#include <memory>

namespace stackchan::avatar {

/**
 * @brief
 *
 */
class DefaultAvatar : public Avatar {
public:
    lv_color_t primaryColor   = lv_color_white();
    lv_color_t secondaryColor = lv_color_black();

    ~DefaultAvatar();

    void init(lv_obj_t* parent, const lv_font_t* font = &lv_font_montserrat_16);
    uitk::lvgl_cpp::Container* getPanel() const;

    /**
     * @brief Midoryu: the face is drawn by DragonFace into an LVGL canvas
     */
    void update() override;

private:
    void render_face(uint32_t now);

    std::unique_ptr<uitk::lvgl_cpp::Container> _pannel;
    std::unique_ptr<DragonFace> _face;
    lv_obj_t* _canvas          = nullptr;
    lv_draw_buf_t* _draw_buf   = nullptr;
    bool _dirty                = true;
    uint32_t _last_render_ms   = 0;
};

/**
 * @brief
 *
 */
class DefaultEyes : public Feature {
public:
    DefaultEyes(lv_obj_t* parent, lv_color_t primaryColor, lv_color_t secondaryColor, bool isLeftEye);

    // The face itself is drawn by DragonFace; eyes only keep state that modifiers write to
    void setEmotion(const Emotion& emotion) override;

private:
    bool _is_left_eye = false;
};

/**
 * @brief
 *
 */
class DefaultMouth : public Feature {
public:
    DefaultMouth(lv_obj_t* parent, lv_color_t primaryColor, lv_color_t secondaryColor);
};

/**
 * @brief
 *
 */
class DefaultSpeechBubble : public SpeechBubble {
public:
    DefaultSpeechBubble(lv_obj_t* parent, lv_color_t primaryColor, lv_color_t secondaryColor, const lv_font_t* font);
    ~DefaultSpeechBubble();

    void setSpeech(std::string_view text) override;
    void clearSpeech() override;
    void setVisible(bool visible) override;
    void setTextFont(void* font) override;

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _container;
    std::unique_ptr<uitk::lvgl_cpp::Image> _arrow;
    std::unique_ptr<uitk::lvgl_cpp::Container> _bubble;
    std::unique_ptr<uitk::lvgl_cpp::Label> _text;
};

}  // namespace stackchan::avatar
