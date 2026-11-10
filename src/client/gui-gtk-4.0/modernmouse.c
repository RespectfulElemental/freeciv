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

/* Fork-only: mouse control presets for the map view, chosen with the
   client option gui_gtk4_mouse_controls.

   Classic: upstream behaviour; every hook here does nothing.

   Modern
   - A plain left press is not acted on right away. If the pointer moves
     more than DRAG_THRESHOLD pixels before release, the map pans with it;
     otherwise the release acts as an ordinary left click at the press
     position (select unit, open city, ...).
   - Exception: with "Keyboardless goto" on, a press on a unit in focus
     that stands alone outside a city keeps upstream's behaviour (act on
     press, drag = go-to). That is exactly the case where do_map_click()
     arms drag-goto; in a city or a stack the press would only open the
     city dialog or unit list, so those presses pan or click instead.
   - Right-drag still draws the selection box (upstream code). A box that
     never got past the drag threshold counts as a click.
   - Right-click sends the units in focus to the tile (go-to). In a hover
     mode it cancels like Esc; with cities highlighted it clears them. It
     never recenters the map.

   Command: left chooses and views, right gives orders.
   - Plain left: as Modern (click on release, drag pans), but without the
     drag-goto exception. In a hover mode (go-to etc.) a left press
     cancels it, like Esc.
   - Ctrl+left: a click quickselects a sea unit, a drag draws the
     selection box. Ctrl+Shift+left: a click adjusts city workers, a drag
     adds to the selection. Both act on release.
   - Plain right: holding it for RIGHT_HOLD_MS or dragging past the
     threshold starts a go-to for the units in focus; the release gives
     the order at the pointer's tile (nothing on the units' own tile). A
     plain right-click alone gives no order; it clears city highlights.
     In a hover mode started from the keyboard, the right release
     confirms the target (as left-click does in the other presets).
   - Middle-click centers the map there and shows the tile info popup.
   - Modifier clicks other than Ctrl+left work as upstream.

   GtkGestureClick still emits "released" after a drag past its threshold
   (it only emits "stopped" then, with n_press 0), which upstream's
   keyboardless goto and selection box rely on too; so do we. */

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <gtk/gtk.h>

/* common */
#include "map.h"
#include "tile.h"
#include "unit.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "control.h"
#include "mapctrl_common.h"
#include "mapview_common.h"
#include "options.h"
#include "zoom.h"

/* client/gui-gtk-4.0 */
#include "gui_main.h"
#include "mapview.h"

#include "modernmouse.h"

/* Pixels the pointer may move and still count as a click. */
#define DRAG_THRESHOLD 5
/* Command: how long a right press must be held to start an order. */
#define RIGHT_HOLD_MS 250

/* What a deferred left press does on release, or when dragged. */
enum left_kind {
  LEFT_NONE,        /* No deferred press. */
  LEFT_PLAIN,       /* Click, or pan when dragged. */
  LEFT_CTRL,        /* Command: quickselect sea unit, or selection box. */
  LEFT_CTRL_SHIFT   /* Command: adjust workers, or append to selection. */
};

static enum left_kind left_kind = LEFT_NONE;
static bool left_dragging = FALSE; /* Past the threshold: pan or box. */
static double left_x, left_y;      /* Where it was pressed. */
static int origin_x, origin_y;     /* Map scroll position at that time. */

static bool right_pending = FALSE; /* Plain right press awaiting release. */
static double right_x, right_y;
static bool right_confirms = FALSE; /* Command: pressed in a hover mode. */
static bool right_order = FALSE;    /* Command: this press started a go-to. */
static guint right_hold_timer = 0;

/**********************************************************************//**
  The active preset (enum GUI_GTK_MOUSE_*).
**************************************************************************/
static int preset(void)
{
  return GUI_GTK_OPTION(mouse_controls);
}

/**********************************************************************//**
  Whether (x, y) is more than the drag threshold away from (x0, y0).
**************************************************************************/
static bool past_threshold(double x0, double y0, double x, double y)
{
  double dx = x - x0, dy = y - y0;

  return dx * dx + dy * dy > DRAG_THRESHOLD * DRAG_THRESHOLD;
}

/**********************************************************************//**
  Whether one of the units in focus stands on ptile.
**************************************************************************/
static bool focus_unit_on_tile(const struct tile *ptile)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (unit_tile(punit) == ptile) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/**********************************************************************//**
  Whether a go-to to ptile would move anything: some unit in focus stands
  elsewhere.
**************************************************************************/
static bool focus_unit_elsewhere(const struct tile *ptile)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (unit_tile(punit) != ptile) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/**********************************************************************//**
  Finish a go-to that is in HOVER_GOTO: send it to ptile if that moves
  anything, then leave goto mode. Same steps as a keyboardless goto (see
  release_goto_button()).
**************************************************************************/
static void finish_goto(struct tile *ptile)
{
  if (ptile != NULL && focus_unit_elsewhere(ptile)) {
    do_unit_goto(ptile);
  }
  clear_hover_state();
  update_unit_info_label(get_units_in_focus());
}

/**********************************************************************//**
  Remember a deferred left press.
**************************************************************************/
static void left_defer(enum left_kind kind, double x, double y)
{
  left_kind = kind;
  left_dragging = FALSE;
  left_x = x;
  left_y = y;
  get_mapview_scroll_pos(&origin_x, &origin_y);
}

/**********************************************************************//**
  Command: a left press, checked before mapctrl.c's modifier handling.
  In a hover mode it cancels the order. With Ctrl (and maybe Shift) it is
  deferred: it acts on release (click) or draws the selection box (drag).
  Returns TRUE if taken over.
**************************************************************************/
bool modernmouse_left_press_early(double x, double y,
                                  GdkModifierType state)
{
  left_kind = LEFT_NONE;
  left_dragging = FALSE;

  if (preset() != GUI_GTK_MOUSE_COMMAND || (state & GDK_ALT_MASK)) {
    return FALSE;
  }

  if (hover_state != HOVER_NONE) {
    /* Left cancels an order, whatever the modifiers. */
    key_cancel_action();
    return TRUE;
  }

  if (!(state & GDK_CONTROL_MASK)) {
    return FALSE;
  }

  left_defer((state & GDK_SHIFT_MASK) ? LEFT_CTRL_SHIFT : LEFT_CTRL, x, y);

  return TRUE;
}

/**********************************************************************//**
  A plain left press (no modifiers, no area/rally/infra mode) on the map.
  Returns TRUE if the preset takes it over; the click then happens on
  release. Returns FALSE to let mapctrl.c act on the press as usual.
**************************************************************************/
bool modernmouse_left_press(double x, double y, struct tile *ptile)
{
  left_kind = LEFT_NONE;
  left_dragging = FALSE;

  switch (preset()) {
  case GUI_GTK_MOUSE_MODERN:
    if (hover_state != HOVER_NONE) {
      return FALSE;
    }
    if (gui_options.keyboardless_goto && ptile != NULL
        && tile_city(ptile) == NULL && unit_list_size(ptile->units) == 1
        && focus_unit_on_tile(ptile)) {
      /* Drag from the selected unit = go-to, as upstream. */
      return FALSE;
    }
    break;
  case GUI_GTK_MOUSE_COMMAND:
    if (hover_state != HOVER_NONE) {
      /* Left cancels an order. */
      key_cancel_action();
      return TRUE;
    }
    break;
  default:
    return FALSE;
  }

  left_defer(LEFT_PLAIN, x, y);

  return TRUE;
}

/**********************************************************************//**
  Command: start a go-to for the units in focus, from a right hold/drag.
**************************************************************************/
static void command_start_order(void)
{
  if (right_order || hover_state != HOVER_NONE
      || !can_client_issue_orders() || get_num_units_in_focus() == 0) {
    return;
  }

  request_unit_goto(ORDER_LAST, ACTION_NONE, -1);
  right_order = (hover_state == HOVER_GOTO);
}

/**********************************************************************//**
  Command: stop waiting for a right hold.
**************************************************************************/
static void stop_hold_timer(void)
{
  if (right_hold_timer != 0) {
    g_source_remove(right_hold_timer);
    right_hold_timer = 0;
  }
}

/**********************************************************************//**
  Command: the right button has been held long enough.
**************************************************************************/
static gboolean right_hold_cb(gpointer data)
{
  right_hold_timer = 0;
  if (right_pending && !right_confirms) {
    command_start_order();
  }

  return G_SOURCE_REMOVE;
}

/**********************************************************************//**
  The pointer moved over the map: pan, draw the box, or start an order.
**************************************************************************/
void modernmouse_motion(double x, double y, GdkModifierType state)
{
  if (right_pending && !right_confirms && !right_order) {
    if (!(state & GDK_BUTTON3_MASK)) {
      /* The release went elsewhere. */
      right_pending = FALSE;
      stop_hold_timer();
    } else if (past_threshold(right_x, right_y, x, y)) {
      stop_hold_timer();
      command_start_order();
    }
  }

  if (left_kind == LEFT_NONE) {
    return;
  }

  if (!(state & GDK_BUTTON1_MASK)) {
    /* The release went elsewhere (e.g. the gesture was cancelled). */
    if (left_kind != LEFT_PLAIN && left_dragging) {
      cancel_selection_rectangle();
    }
    left_kind = LEFT_NONE;
    left_dragging = FALSE;
    return;
  }

  if (!left_dragging) {
    if (!past_threshold(left_x, left_y, x, y)
        || !can_client_change_view()) {
      return;
    }
    left_dragging = TRUE;
    if (left_kind != LEFT_PLAIN) {
      anchor_selection_rectangle((int) left_x, (int) left_y);
    }
  }

  if (left_kind == LEFT_PLAIN) {
    /* Relative to where the drag started, so rounding never accumulates. */
    set_mapview_scroll_pos(origin_x - (int) (x - left_x),
                           origin_y - (int) (y - left_y), mouse_zoom);
  } else {
    update_selection_rectangle(x, y);
  }
}

/**********************************************************************//**
  The left button was released at (x, y). Returns TRUE if a deferred plain
  press should now act as a plain click; (*click_x, *click_y) is then
  where it was pressed. Other deferred presses are finished here.
**************************************************************************/
bool modernmouse_left_release(double x, double y,
                              double *click_x, double *click_y)
{
  enum left_kind kind = left_kind;
  bool dragged = left_dragging;
  bool shift = (kind == LEFT_CTRL_SHIFT);

  left_kind = LEFT_NONE;
  left_dragging = FALSE;

  switch (kind) {
  case LEFT_NONE:
    return FALSE;
  case LEFT_PLAIN:
    if (dragged) {
      return FALSE;
    }
    *click_x = left_x;
    *click_y = left_y;
    return TRUE;
  case LEFT_CTRL:
  case LEFT_CTRL_SHIFT:
    if (!dragged) {
      if (shift) {
        adjust_workers_button_pressed((int) left_x, (int) left_y);
      } else {
        action_button_pressed((int) left_x, (int) left_y, SELECT_SEA);
      }
    } else if (rectangle_active) {
      /* Upstream's box release; it only recenters when no box is up. */
      release_right_button((int) x, (int) y, shift);
    } else {
      /* Dragged, but never past a tile: no box, no click. */
      cancel_selection_rectangle();
      rbutton_down = FALSE;
    }
    return FALSE;
  }

  return FALSE;
}

/**********************************************************************//**
  A right press on the map, checked before mapctrl.c's handling. Returns
  TRUE if the preset takes it over.
**************************************************************************/
bool modernmouse_right_press(double x, double y, GdkModifierType state)
{
  right_pending = FALSE;
  right_order = FALSE;
  right_confirms = FALSE;
  stop_hold_timer();

  if (preset() != GUI_GTK_MOUSE_COMMAND
      || (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_ALT_MASK))) {
    return FALSE;
  }

  right_pending = TRUE;
  right_x = x;
  right_y = y;
  right_confirms = (hover_state != HOVER_NONE);
  if (!right_confirms) {
    right_hold_timer = g_timeout_add(RIGHT_HOLD_MS, right_hold_cb, NULL);
  }

  return TRUE;
}

/**********************************************************************//**
  Modern: a plain right press anchored the selection box at (x, y).
**************************************************************************/
void modernmouse_right_anchor(double x, double y)
{
  if (preset() == GUI_GTK_MOUSE_MODERN) {
    right_pending = TRUE;
    right_x = x;
    right_y = y;
  }
}

/**********************************************************************//**
  Command: the right button was released at (x, y).
**************************************************************************/
static bool command_right_release(double x, double y)
{
  struct tile *ptile = canvas_pos_to_tile(x, y, mouse_zoom);
  bool pressed = right_pending;
  bool order = right_order;
  bool confirms = right_confirms;

  right_pending = FALSE;
  right_order = FALSE;
  right_confirms = FALSE;
  stop_hold_timer();

  if (!pressed) {
    /* A modifier press handled upstream, e.g. Shift+right falling
     * through to the selection box. */
    if (rbutton_down && rectangle_active) {
      return FALSE;
    }
    cancel_selection_rectangle();
    rbutton_down = FALSE;
    return TRUE;
  }

  if (order) {
    if (hover_state == HOVER_GOTO) {
      /* Not cancelled meanwhile (left-click, Esc). */
      finish_goto(ptile);
    }
  } else if (confirms) {
    if (hover_state != HOVER_NONE && ptile != NULL) {
      action_button_pressed((int) x, (int) y, SELECT_POPUP);
    }
  } else {
    /* A plain right-click gives no order. */
    cancel_tile_hiliting();
  }

  return TRUE;
}

/**********************************************************************//**
  Modern: the right button was released at (x, y).
**************************************************************************/
static bool modern_right_release(double x, double y)
{
  bool pressed = right_pending;
  double cx = pressed ? right_x : x, cy = pressed ? right_y : y;

  right_pending = FALSE;

  if (!rbutton_down && hover_state == HOVER_NONE) {
    /* Upstream does nothing here either. */
    return FALSE;
  }

  if (rectangle_active) {
    if (!pressed || past_threshold(right_x, right_y, x, y)) {
      /* A real selection box. */
      return FALSE;
    }
    /* Jitter across a tile edge: still a click. */
    cancel_selection_rectangle();
  }
  rbutton_down = FALSE;
  rectangle_active = FALSE;

  if (hover_state != HOVER_NONE) {
    key_cancel_action();
  } else if (tiles_hilited_cities) {
    cancel_tile_hiliting();
  } else {
    struct tile *ptile = canvas_pos_to_tile(cx, cy, mouse_zoom);

    if (ptile != NULL && can_client_issue_orders()
        && focus_unit_elsewhere(ptile)) {
      request_unit_goto(ORDER_LAST, ACTION_NONE, -1);
      if (hover_state == HOVER_GOTO) {
        finish_goto(ptile);
      }
    }
  }

  return TRUE;
}

/**********************************************************************//**
  The right button was released at (x, y). Returns TRUE if the preset
  handled it, FALSE to let upstream's code run.
**************************************************************************/
bool modernmouse_right_release(double x, double y)
{
  switch (preset()) {
  case GUI_GTK_MOUSE_MODERN:
    return modern_right_release(x, y);
  case GUI_GTK_MOUSE_COMMAND:
    return command_right_release(x, y);
  default:
    right_pending = FALSE;
    return FALSE;
  }
}

/**********************************************************************//**
  Whether a plain middle-click should center the map on the clicked tile
  (before showing the tile info).
**************************************************************************/
bool modernmouse_middle_centers(void)
{
  return preset() == GUI_GTK_MOUSE_COMMAND;
}
