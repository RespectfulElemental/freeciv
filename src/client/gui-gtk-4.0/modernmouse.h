/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__MODERNMOUSE_H
#define FC__MODERNMOUSE_H

#include <gtk/gtk.h>

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "fc_types.h"

/* Fork-only: mouse control presets for the map view (option
 * gui_gtk4_mouse_controls: Classic, Modern, Command). See the comment at
 * the top of modernmouse.c. The hooks are called from mapctrl.c; each
 * returns FALSE (or does nothing) under Classic. */

bool modernmouse_left_press_early(double x, double y,
                                  GdkModifierType state);
bool modernmouse_left_press(double x, double y, struct tile *ptile);
void modernmouse_motion(double x, double y, GdkModifierType state);
bool modernmouse_left_release(double x, double y,
                              double *click_x, double *click_y);

bool modernmouse_right_press(double x, double y, GdkModifierType state);
void modernmouse_right_anchor(double x, double y);
bool modernmouse_right_release(double x, double y);

bool modernmouse_middle_centers(void);

#endif /* FC__MODERNMOUSE_H */
