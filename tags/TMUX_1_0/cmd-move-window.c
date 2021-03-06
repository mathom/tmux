/* $Id: cmd-move-window.c,v 1.9 2009-08-16 19:16:27 tcunha Exp $ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <stdlib.h>

#include "tmux.h"

/*
 * Move a window.
 */

int	cmd_move_window_exec(struct cmd *, struct cmd_ctx *);

const struct cmd_entry cmd_move_window_entry = {
	"move-window", "movew",
	"[-dk] " CMD_SRCDST_WINDOW_USAGE,
	0, CMD_CHFLAG('d')|CMD_CHFLAG('k'),
	cmd_srcdst_init,
	cmd_srcdst_parse,
	cmd_move_window_exec,
	cmd_srcdst_free,
	cmd_srcdst_print
};

int
cmd_move_window_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct cmd_srcdst_data	*data = self->data;
	struct session		*src, *dst;
	struct winlink		*wl_src, *wl_dst;
	struct client		*c;
	u_int		 	 i;
	int		 	 destroyed, idx;
	char			*cause;

	if ((wl_src = cmd_find_window(ctx, data->src, &src)) == NULL)
		return (-1);
	if ((idx = cmd_find_index(ctx, data->dst, &dst)) == -2)
		return (-1);

	wl_dst = NULL;
	if (idx != -1)
		wl_dst = winlink_find_by_index(&dst->windows, idx);
	if (wl_dst != NULL) {
		if (wl_dst->window == wl_src->window)
			return (0);

		if (data->chflags & CMD_CHFLAG('k')) {
			/*
			 * Can't use session_detach as it will destroy session
			 * if this makes it empty.
			 */
			session_alert_cancel(dst, wl_dst);
			winlink_stack_remove(&dst->lastw, wl_dst);
			winlink_remove(&dst->windows, wl_dst);

			/* Force select/redraw if current. */
			if (wl_dst == dst->curw) {
				data->chflags &= ~CMD_CHFLAG('d');
				dst->curw = NULL;
			}
		}
	}

	if (idx == -1)
		idx = -1 - options_get_number(&dst->options, "base-index");
	wl_dst = session_attach(dst, wl_src->window, idx, &cause);
	if (wl_dst == NULL) {
		ctx->error(ctx, "attach window failed: %s", cause);
		xfree(cause);
		return (-1);
	}

	destroyed = session_detach(src, wl_src);
	for (i = 0; i < ARRAY_LENGTH(&clients); i++) {
		c = ARRAY_ITEM(&clients, i);
		if (c == NULL || c->session != src)
			continue;
		if (destroyed) {
			c->session = NULL;
			server_write_client(c, MSG_EXIT, NULL, 0);
		} else
			server_redraw_client(c);
	}

	if (data->chflags & CMD_CHFLAG('d'))
		server_status_session(dst);
	else {
		session_select(dst, wl_dst->idx);
		server_redraw_session(dst);
	}
	recalculate_sizes();

	return (0);
}
