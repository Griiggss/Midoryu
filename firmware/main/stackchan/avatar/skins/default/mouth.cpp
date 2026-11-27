/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "default.h"

using namespace stackchan::avatar;

// Midoryu: the mouth is drawn by DragonFace. Weight (0..100) is how open it is while speaking.

DefaultMouth::DefaultMouth(lv_obj_t* parent, lv_color_t primaryColor, lv_color_t secondaryColor)
{
    (void)parent;
    (void)primaryColor;
    (void)secondaryColor;
    setWeight(0);
}
