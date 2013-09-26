/**
 * @file at_io.c
 * @brief mambo io
 *
 * @date May 2011
 * @author Jean-Baptiste Dubois
 * @copyright Copyright (C) 2011 Parrot S.A.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <io_src_tmr.h>
#include <io_utils.h>

#include "at_io.h"

/* #define AT_IO_DBG */
#ifdef AT_IO_DBG
#define dbg(fmt, ...) at_log_dbg((fmt), __VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

#define at_log_raw(...) do {} while (0)
#define at_log_level(...) 0
#define ATLOG_DEBUG 0

static int at_io_read(int fd, int ign_eof, int log, const char *name,
		void *buffer, size_t size, size_t *length)
{
	ssize_t nbytes;

	*length = 0;
	/* read without blocking */
	do {
		nbytes = read(fd, buffer, size);
	} while (nbytes == -1 && errno == EINTR);

	if (nbytes == -1)
		return -errno;
	if (0 != nbytes) {
		*length = (size_t)(nbytes);

		/* log data read */
		if (log && (at_log_level() >= ATLOG_DEBUG))
			at_log_raw(__func__, ATLOG_DEBUG, buffer, *length,
				   "%s read fd=%d length=%d", name, fd,
				   *length);
	}

	return 0;
}

static void at_io_read_events(struct io_src *read_src)
{
	struct at_io_read_ctx *readctx = rs_container_of(read_src,
			struct at_io_read_ctx, src);
	struct at_io *io = rs_container_of(readctx, struct at_io, readctx);
	size_t length = 0;
	void *buffer;
	size_t size;
	int cbret = 0;
	int eof = 0;
	int ret = 0;
	int fd = io_src_get_fd(read_src);

	/* remove source from loop on error */
	if (io_src_has_error(read_src->events))
		/*
		 * TODO change for an explicit value other than EAGAIN, e.g EIO
		 */
		ret = -1;

	/* do not treat event other than read available */
	if (!io_src_has_in(read_src->events))
		return;

	/* read until no more space in ring buffer or read error */
	readctx->newbytes = 0;
	while (ret == 0 && !eof && rs_rb_get_write_length(&readctx->rb) > 0) {
		buffer = rs_rb_get_write_ptr(&readctx->rb);
		size = rs_rb_get_write_length_no_wrap(&readctx->rb);
		assert(size > 0);
		ret = at_io_read(fd, io->readctx.ign_eof, io->log[AT_IO_RX],
				io->name, buffer, size, &length);
		/* check if first part of ring buffer is full-filled */
		if (ret == 0 && length > 0) {
			readctx->newbytes += length;
			rs_rb_write_incr(&readctx->rb, length);
			/* if free space available in ring buffer read again */
			if (rs_rb_get_write_length(&readctx->rb) > 0)
				continue;

		} else if (ret == 0 && length == 0) {
			/* end of file */
			eof = 1;
		}

		/* notify client if new bytes available */
		if (readctx->newbytes > 0) {
			cbret = (*readctx->cb) (io, &readctx->rb, readctx->newbytes,
					readctx->data);
			/* continue only if client need more data */
			if (cbret != 0)
				return;
		}
	}

	/* log something if read buffer is full */
	if (rs_rb_get_write_length(&readctx->rb) == 0) {
		/* TODO replace with a client notification */
//		at_log_warn("%s fd=%d, io read buffer(%dB) full, data lost!",
//				io->name, fd, rs_rb_get_size(&readctx->rb));
		rs_rb_empty(&readctx->rb);
	}

	/* remove source if end of file or read error
	 * (other than no more data!)
	 */
	if ((eof && !io->readctx.ign_eof) || (ret < 0 && ret != -EAGAIN)) {
		io_mon_remove_source(io->mon, &readctx->src);
		io_src_clean(&readctx->src);
		/* update state and notify client */
		readctx->state = AT_IO_ERROR;
		(*readctx->cb) (io, &readctx->rb, readctx->newbytes, readctx->data);
	}
}

/* enable/disable rx traffic log */
int at_io_log_rx(struct at_io *io, int enable)
{
	if (!io)
		return -EINVAL;

	io->log[AT_IO_RX] = enable;
	return 0;
}

/* enable/disable tx traffic log */
int at_io_log_tx(struct at_io *io, int enable)
{
	if (!io)
		return -EINVAL;

	io->log[AT_IO_TX] = enable;
	return 0;
}

/* get read state */
int at_io_read_state(struct at_io *io)
{
	return io->readctx.state;
}

int at_io_read_start(struct at_io *io, at_io_read_cb_t cb, void *data,
		int clear)
{
	int ret;

	if (!io || !cb)
		return -EINVAL;

	if (io->readctx.state != AT_IO_STOPPED)
		return -EBUSY;

	/*
	 * activate out source, useless at init, but needed after calls to
	 * at_io_read_stop()
	 */
	ret = io_mon_activate_in_source(io->mon, &io->readctx.src, 1);
	if (ret < 0)
		return ret;

	/* set callback info */
	io->readctx.cb = cb;
	io->readctx.data = data;

	/* clear read buffer if needed */
	if (clear)
		rs_rb_empty(&io->readctx.rb);

	/* update read state */
	io->readctx.state = AT_IO_STARTED;
	return 0;
}

int at_io_read_stop(struct at_io *io)
{
	if (NULL == io)
		return -EINVAL;

	if (io->readctx.state != AT_IO_STARTED)
		return -EBUSY;

	/* reset callback info */
	io->readctx.cb = NULL;
	io->readctx.data = NULL;

	/* update state */
	io->readctx.state = AT_IO_STOPPED;

	/* unregister read source in loop */
	return io_mon_remove_source(io->mon, &io->readctx.src);
}

static int at_io_write(int fd, int log, const char *name, const void *buffer,
		size_t size, size_t *length)
{
	int ret = 0;
	ssize_t nbytes;

	*length = 0;
	/* write without blocking */
	do {
		nbytes = write(fd, buffer, size);
	} while (nbytes == -1 && errno == EINTR);

	dbg("write(%d, %p, %d) ret:%d %s", fd, buffer, size, nbytes,
		nbytes == -1 ? strerror(errno) : "");

	if (nbytes == -1)
		return -errno;
	*length = (size_t)(nbytes);

	/* log data written */
	if (log && (at_log_level() >= ATLOG_DEBUG))
		at_log_raw(__func__, ATLOG_DEBUG, buffer, *length,
		"%s write fd=%d length=%d", name, fd, *length);

	return ret;
}

static void at_io_write_process_next(struct at_io *io)
{
	struct rs_node *first;
	struct at_io_write_ctx *ctx = &io->writectx;

	/* reset current buffer info */
	ctx->current = NULL;
	ctx->nbwritten = 0;
	ctx->nbeagain = 0;

	first = rs_dll_pop(&ctx->buffers);
	if (first) {
		/* get next write buffer */
		ctx->current = rs_container_of(first, struct at_io_write_buffer,
				node);
		/* add fd object in loop if not already done */
		io_mon_activate_out_source(io->mon, &ctx->src, 1);

		/* set write timer */
		io_src_tmr_set(&ctx->timer, ctx->timeout);
	} else {
		/* no more buffer, clear timer */
		io_src_tmr_set(&ctx->timer, IO_SRC_TMR_DISARM);
		/* remove fd object if added */
		io_mon_activate_out_source(io->mon, &ctx->src, 0);
	}
}

static void at_io_write_timer(struct io_src_tmr *timer, uint64_t *nbexpired)
{
	struct at_io_write_ctx *ctx = rs_container_of(timer,
			struct at_io_write_ctx, timer);
	struct at_io *io = rs_container_of(ctx, struct at_io, writectx);
	struct at_io_write_buffer *buffer;

	/* get current write buffer */
	buffer = ctx->current;
	if (!buffer) {
		io_src_tmr_set(&ctx->timer, IO_SRC_TMR_DISARM);
		return;
	}

	/* process next buffer */
	at_io_write_process_next(io);

	/* notify buffer cb */
	(*buffer->cb)(buffer, AT_IO_WRITE_TIMEOUT);
}

static void at_io_write_events(struct io_src *write_src)
{
	struct at_io_write_ctx *writectx = rs_container_of(write_src,
			struct at_io_write_ctx, src);
	struct at_io *io = rs_container_of(writectx, struct at_io, writectx);
	struct at_io_write_buffer *buffer;
	enum at_io_write_status status;
	size_t length = 0;
	int ret = 0;

	/* remove source from loop on error */
	if (io_src_has_error(write_src->events)) {
		io_mon_remove_source(io->mon, &writectx->src);
		io_src_clean(&writectx->src);
		return;
	}

	/* do not treat event other than write ready */
	if (!io_src_has_out(write_src->events))
		return;

	/* get current write buffer */
	buffer = writectx->current;
	if (!buffer) { /* TODO can this really happen ? replace by an assert? */
		io_mon_activate_out_source(io->mon, &writectx->src, 0);
		return;
	}

	/* write current buffer */
	while (ret == 0 && writectx->nbwritten < buffer->length) {
		ret = at_io_write(write_src->fd, io->log[AT_IO_TX], io->name,
				(uint8_t *)buffer->address + writectx->nbwritten,
				buffer->length - writectx->nbwritten, &length);
		if (ret == 0) {
			/* clear eagain flags */
			writectx->nbeagain = 0;
			writectx->nbwritten += length;
		} else if (ret == -EAGAIN) {
			writectx->nbeagain++;
		}
	}

	/* if we received more than 20 EAGAIN with POLLOUT set,
	 * a PROBLEM occurs in driver */
	if (writectx->nbeagain >= 20)
		ret = -ENOBUFS;

	/* wait for a next write ready if needed else buffer process completed*/
	if (ret != -EAGAIN) {

		at_io_write_process_next(io);

		/* notify buffer cb */
		status = ret == 0 ? AT_IO_WRITE_OK : AT_IO_WRITE_ERROR;
		(*buffer->cb)(buffer, status);
	}
}

static void at_io_write_default_cb(struct at_io_write_buffer *buffer,
	enum at_io_write_status status)
{

}


/*  add write buffer in queue */
int at_io_write_add(struct at_io *io, struct at_io_write_buffer *buffer)
{
	int ret = 0;
	struct at_io_write_ctx *ctx = &io->writectx;

	if (!buffer->address || buffer->length == 0)
		return -EINVAL;

	if (!buffer->cb) {
		buffer->cb = &at_io_write_default_cb;
		buffer->data = io;
	}

	rs_dll_enqueue(&ctx->buffers, &buffer->node);
	if (ctx->current == NULL)
		at_io_write_process_next(io);

	return ret;
}

/* abort all write buffers in io write queue
 * (buffer cb invoked with status AT_IO_WRITE_ABORTED) */
int at_io_write_abort(struct at_io *io)
{
	struct at_io_write_ctx *ctx = &io->writectx;
	struct at_io_write_buffer *buffer = NULL;
	struct at_io_write_buffer *tmp;
	struct rs_node *node;

	buffer = ctx->current;

	/* TODO: how to be safe on io destroy call in write cb here ? */
	if (buffer) {
		(*buffer->cb) (buffer, AT_IO_WRITE_ABORTED);
		ctx->current = NULL;
		ctx->nbwritten = 0;
	}

	while ((node = rs_dll_pop(&ctx->buffers))) {
		tmp = rs_container_of(node, struct at_io_write_buffer, node);
		(*buffer->cb) (buffer, AT_IO_WRITE_ABORTED);
	}

	at_io_write_process_next(io);
	return 0;
}

int at_io_create(struct at_io *io, struct io_mon *mon, const char *name,
		int fd_in, int fd_out, int ign_eof)
{
	int ret;

	if (NULL == io || NULL == mon || rs_str_is_invalid(name))
		return -EINVAL;

	if (fd_in < 0 || fd_out < 0)
		return -EINVAL;

	memset(io, 0, sizeof(*io));

	io->fds[AT_IO_RX] = fd_in;
	/* TODO crappy way to support full duplex file descriptors */
	if (fd_out == fd_in) {
		io->fds[AT_IO_TX] = dup(fd_out);
		io->dupped = 1;
	} else {
		io->fds[AT_IO_TX] = fd_out;
		io->dupped = 0;
	}

	/* disable io log by default */
	io->log[AT_IO_RX] = 0;
	io->log[AT_IO_TX] = 0;

	/* create 2KB ring buffer for read */
	ret = rs_rb_init(&io->readctx.rb, io->readctx.rb_buffer,
			AT_IO_RB_BUFFER_SIZE);
	if (ret < 0)
		return ret;

	/* TODO split out creation/initialization of read and write contexts */

	/* add read fd object */
	io_src_init(&io->readctx.src, io->fds[AT_IO_RX], IO_IN,
			&at_io_read_events);

	/* set read state to disable */
	io->readctx.state = AT_IO_STOPPED;
	io->readctx.cb = NULL;
	io->readctx.data = NULL;
	io->readctx.ign_eof = ign_eof;

	/* create write timer */
	ret = io_src_tmr_init(&io->writectx.timer, &at_io_write_timer);
	if (ret < 0)
		goto free_rb;

	io->mon = mon;
	io->writectx.current = NULL;
	io->writectx.nbwritten = 0;

	/* init write buffer queue */
	rs_dll_init(&io->writectx.buffers, NULL);

	/* create a write source with fd_out */
	io_src_init(&io->writectx.src, io->fds[AT_IO_TX], IO_OUT,
			&at_io_write_events);

	/* set default write ready timeout to 10s */
	io->writectx.timeout = 10000;
	io->writectx.state = AT_IO_STARTED;
	io->name = strdup(name);

	ret = io_mon_add_sources(mon,
			&io->writectx.timer.src,
			&io->readctx.src,
			&io->writectx.src,
			NULL /* NULL guard */
			);
	if (0 != ret)
		goto free_rb;

	return 0;

	/* TODO better error handling */
free_rb:
	rs_rb_clean(&io->readctx.rb);

	return ret;
}

int at_io_destroy(struct at_io *io)
{
	/* stop read if started */
	if (io->readctx.state == AT_IO_STARTED)
		at_io_read_stop(io);

	io_mon_remove_source(io->mon, &io->writectx.timer.src);
	io_mon_remove_source(io->mon, &io->writectx.src);
	rs_rb_clean(&io->readctx.rb);
	io_src_clean(&io->readctx.src);

	/* destroy write */
	at_io_write_abort(io);

	/* TODO io_mon_remove_source(&io->loop->mon, &io->writectx.timer.src) */
	io_src_tmr_clean(&io->writectx.timer);
	free(io->name);
	if (io->dupped)
		io_close(io->fds + AT_IO_TX);
	memset(io, 0, sizeof(*io));

	return 0;
}
