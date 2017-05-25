/* PipeWire
 * Copyright (C) 2015 Wim Taymans <wim.taymans@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <pipewire/client/pipewire.h>
#include <pipewire/server/core.h>
#include <pipewire/server/module.h>

#include "daemon-config.h"

int
main (int argc, char *argv[])
{
  struct pw_core *core;
  struct pw_main_loop *loop;
  struct pw_daemon_config *config;
  char *err = NULL;

  pw_init (&argc, &argv);

  /* parse configuration */
  config = pw_daemon_config_new ();
  if (!pw_daemon_config_load (config, &err)) {
    pw_log_error ("failed to parse config: %s", err);
    free (err);
    return -1;
  }

  loop = pw_main_loop_new ();
#if 1
  {
    struct spa_source *source;
    source = pw_loop_add_event (loop->loop, NULL, NULL);
    pw_log_set_trace_event (source);
  }
#endif

  core = pw_core_new (loop, NULL);

  pw_daemon_config_run_commands (config, core);

  pw_main_loop_run (loop);

  pw_main_loop_destroy (loop);

  pw_core_destroy (core);

  return 0;
}