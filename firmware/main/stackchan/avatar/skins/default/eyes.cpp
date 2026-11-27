/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "default.h"

using namespace stackchan::avatar;

// Midoryu: the eyes are drawn by DragonFace (see dragon_face.cpp). This class only keeps
// the state (weight, position, size, visibility) that blink, idle-motion, breath and
// the other modifiers write to.

DefaultEyes::DefaultEyes(lv_obj_t* parent, lv_color_t primaryColor, lv_color_t secondaryColor, bool isLeftEye)
{
    (void)parent;
    (void)primaryColor;
    (void)secondaryColor;
    _is_left_eye = isLeftEye;
    setWeight(100);
}

void DefaultEyes::setEmotion(const Emotion& emotion)
{
    (void)emotion;
    if (getIgnoreEmotion()) {
        return;
    }
    // Per-emotion eye shape lives in DragonFace; weight is only the "eyelid" the blink modifier closes.
    setWeight(100);
}
