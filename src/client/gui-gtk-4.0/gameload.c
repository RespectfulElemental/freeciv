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

/* Fork-only file. Load Game menu item, quicksave (F5) and quickload (F9),
 * and starting a loaded single-player game without stopping in the
 * pregame lobby.
 *
 * The server refuses /load once a game is running, so loading from inside
 * a game restarts the local server first. The restarted server is started
 * plain and sent "/load <file>" after it answers, rather than started with
 * --file: a server that loads at startup does so before any client is
 * connected, so the client would never get the game_load packet. */

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <string.h>

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "featured_text.h"
#include "game.h"
#include "player.h"

/* client */
#include "chatline_common.h"
#include "client_main.h"
#include "clinet.h"
#include "connectdlg_common.h"
#include "pages_g.h"
#include "update_queue.h"

/* client/gui-gtk-4.0 */
#include "gui_main.h"
#include "gui_stuff.h"

#include "gameload.h"

/* Saved as "<saves dir>/quicksave.sav[.zst|.xz|...]" by the server. */
#define QUICKSAVE_NAME "quicksave"

/* Send /start once we are attached to our player after a load. */
static bool autostart_armed = FALSE;

/* "Load to lobby" checkbox on the load page: keep the old behaviour. */
static bool load_to_lobby = FALSE;

/* File to /load once the restarted local server has answered. */
static char *pending_load = NULL;

/* What an OK in the confirmation dialog does. */
enum confirm_action {
  CONFIRM_LOAD_GAME,
  CONFIRM_QUICKLOAD
};

static void load_game_confirmed(void);
static void quickload_confirmed(void);

/**********************************************************************//**
  Response callback of confirm_then().
**************************************************************************/
static void confirm_response(GtkWidget *dialog, gint response,
                             gpointer data)
{
  enum confirm_action act = GPOINTER_TO_INT(data);

  gtk_window_destroy(GTK_WINDOW(dialog));
  if (response != GTK_RESPONSE_OK) {
    return;
  }

  switch (act) {
  case CONFIRM_LOAD_GAME:
    load_game_confirmed();
    break;
  case CONFIRM_QUICKLOAD:
    quickload_confirmed();
    break;
  }
}

/**********************************************************************//**
  Ask an OK/Cancel question; do 'act' on OK.
**************************************************************************/
static void confirm_then(const char *question, enum confirm_action act)
{
  GtkWidget *dialog
    = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_QUESTION,
                             GTK_BUTTONS_OK_CANCEL, "%s", question);

  setup_dialog(dialog, toplevel);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
  g_signal_connect(dialog, "response",
                   G_CALLBACK(confirm_response), GINT_TO_POINTER(act));
  gtk_window_present(GTK_WINDOW(dialog));
}

/**********************************************************************//**
  TRUE if we're connected to a local server we started, with hack access.
**************************************************************************/
static bool local_server_with_hack(void)
{
  return client.conn.used && is_server_running()
         && can_client_access_hack();
}

/**********************************************************************//**
  Update-queue callback: send the pending /load to the restarted server.
**************************************************************************/
static void send_pending_load(void *data)
{
  if (pending_load != NULL) {
    send_chat_printf("/load %s", pending_load);
    FC_FREE(pending_load);
  }
}

/**********************************************************************//**
  End the current connection (and the local server), start a fresh local
  server, and load 'path' into it.
**************************************************************************/
static void restart_server_and_load(const char *path)
{
  char *file = fc_strdup(path);

  if (client.conn.used) {
    /* Also asks the local server to quit. */
    disconnect_from_server(FALSE);
  }

  free(pending_load);
  pending_load = file;

  if (client_start_server()) {
    update_queue_connect_processing_finished(
        client.conn.client.last_request_id_used, send_pending_load, NULL);
  } else {
    FC_FREE(pending_load);
  }
}

/**********************************************************************//**
  After confirmation: end the running game and go to the load page.
**************************************************************************/
static void load_game_confirmed(void)
{
  if (client.conn.used) {
    disconnect_from_server(FALSE);
  }
  client_start_server_and_set_page(PAGE_LOAD);
}

/**********************************************************************//**
  Game -> Load Game (Ctrl+L).
**************************************************************************/
void gameload_load_game(void)
{
  if (!client.conn.used) {
    client_start_server_and_set_page(PAGE_LOAD);
  } else if (!is_server_running()) {
    output_window_append(ftc_client,
                         _("Loading a game needs a local server. "
                           "Leave this server first."));
  } else if (client_state() == C_S_PREPARING) {
    set_client_page(PAGE_LOAD);
  } else {
    confirm_then(_("Loading a game will end the current one!"),
                 CONFIRM_LOAD_GAME);
  }
}

/**********************************************************************//**
  Game -> Quicksave (F5).
**************************************************************************/
void gameload_quicksave(void)
{
  if (!local_server_with_hack() || client_state() < C_S_RUNNING) {
    output_window_append(ftc_client,
                         _("Quicksave works only in a local game."));
    return;
  }

  send_save_game(QUICKSAVE_NAME);
}

/**********************************************************************//**
  Full path of the newest quicksave, or NULL. Free the result.
**************************************************************************/
static char *find_quicksave(void)
{
  struct fileinfo_list *files;
  const struct fileinfo *best = NULL;
  char *path = NULL;

  /* fileinfolist_infix() clips each name at the infix, so
   * "quicksave.sav.zst" is listed as "quicksave". */
  files = fileinfolist_infix(get_save_dirs(), ".sav", FALSE);
  if (files == NULL) {
    return NULL;
  }

  fileinfo_list_iterate(files, pfile) {
    if (strcmp(pfile->name, QUICKSAVE_NAME) == 0
        && (best == NULL || pfile->mtime > best->mtime)) {
      best = pfile;
    }
  } fileinfo_list_iterate_end;

  if (best != NULL) {
    path = fc_strdup(best->fullname);
  }
  fileinfo_list_destroy(files);

  return path;
}

/**********************************************************************//**
  After confirmation: load the newest quicksave.
**************************************************************************/
static void quickload_confirmed(void)
{
  char *path = find_quicksave();

  if (path == NULL) {
    output_window_append(ftc_client, _("No quicksave found."));
    return;
  }

  if (local_server_with_hack() && client_state() == C_S_PREPARING) {
    /* Server is still in pregame: it accepts /load directly. */
    send_chat_printf("/load %s", path);
  } else {
    restart_server_and_load(path);
  }
  free(path);
}

/**********************************************************************//**
  Game -> Quickload (F9).
**************************************************************************/
void gameload_quickload(void)
{
  char *path;

  if (client.conn.used && !is_server_running()) {
    output_window_append(ftc_client,
                         _("Quickload works only in a local game."));
    return;
  }

  path = find_quicksave();
  if (path == NULL) {
    output_window_append(ftc_client, _("No quicksave found."));
    return;
  }
  free(path);

  if (client.conn.used && client_state() >= C_S_RUNNING) {
    confirm_then(_("Load the quicksave? Progress since then will be lost."),
                 CONFIRM_QUICKLOAD);
  } else {
    quickload_confirmed();
  }
}

/**********************************************************************//**
  Set by the "Load to lobby" checkbox on the load page.
**************************************************************************/
void gameload_set_load_to_lobby(bool to_lobby)
{
  load_to_lobby = to_lobby;
}

/**********************************************************************//**
  Current "Load to lobby" setting.
**************************************************************************/
bool gameload_get_load_to_lobby(void)
{
  return load_to_lobby;
}

/**********************************************************************//**
  Called from handle_game_load() after a saved game (not a new game or
  scenario) loaded successfully.

  The server sends game_load before it attaches connections to players,
  so we can't /start yet. It attaches by username, and player info
  (with usernames) arrives before game_load, so check here whether the
  attach will happen; if not, stay in the lobby so the user can pick.
**************************************************************************/
void gameload_game_loaded(void)
{
  bool have_player = FALSE;

  autostart_armed = FALSE;

  if (load_to_lobby || !local_server_with_hack()) {
    return;
  }

  players_iterate(pplayer) {
    if (strcmp(pplayer->username, client.conn.username) == 0) {
      have_player = TRUE;
      break;
    }
  } players_iterate_end;

  autostart_armed = have_player;
}

/**********************************************************************//**
  Called whenever the connection list changes. Sends /start once we're
  attached to our player after a load.
**************************************************************************/
void gameload_conn_list_changed(void)
{
  if (!autostart_armed) {
    return;
  }

  if (!client.conn.used || client_state() != C_S_PREPARING) {
    autostart_armed = FALSE;
    return;
  }

  if (client.conn.playing != NULL && !client.conn.observer) {
    autostart_armed = FALSE;
    send_chat("/start");
  }
}
