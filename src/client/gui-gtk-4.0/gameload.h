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
#ifndef FC__GAMELOAD_H
#define FC__GAMELOAD_H

/* utility */
#include "support.h"            /* bool type */

/* Fork-only: Load Game menu item, quicksave/quickload, and skipping the
 * pregame lobby after loading a saved game into a local server. */

void gameload_load_game(void);
void gameload_quicksave(void);
void gameload_quickload(void);

void gameload_set_load_to_lobby(bool to_lobby);
bool gameload_get_load_to_lobby(void);

void gameload_game_loaded(void);
void gameload_conn_list_changed(void);

#endif /* FC__GAMELOAD_H */
