/* $Id: cmd-set-buffer.c,v 1.10 2009-09-07 23:48:54 tcunha Exp $ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
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

#include <string.h>

#include "tmux.h"

/*
 * Add or set a session paste buffer.
 */

int	cmd_set_buffer_exec(struct cmd *, struct cmd_ctx *);

const struct cmd_entry cmd_set_buffer_entry = {
	"set-buffer", "setb",
	CMD_BUFFER_SESSION_USAGE " data",
	CMD_ARG1, 0,
	cmd_buffer_init,
	cmd_buffer_parse,
	cmd_set_buffer_exec,
	cmd_buffer_free,
	cmd_buffer_print
};

int
cmd_set_buffer_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct cmd_buffer_data	*data = self->data;
	struct session		*s;
	u_int			 limit;
	u_char			*pdata;
	size_t			 psize;

	if ((s = cmd_find_session(ctx, data->target)) == NULL)
		return (-1);
	limit = options_get_number(&s->options, "buffer-limit");

	pdata = xstrdup(data->arg);
	psize = strlen(pdata);

	if (data->buffer == -1) {
		paste_add(&s->buffers, pdata, psize, limit);
		return (0);
	}
	if (paste_replace(&s->buffers, data->buffer, pdata, psize) != 0) {
		ctx->error(ctx, "no buffer %d", data->buffer);
		xfree(pdata);
		return (-1);
	}
	return (0);
}
