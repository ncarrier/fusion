/**
 * @file io_src_pid.c
 * @date 29 apr. 2013
 * @author nicolas.carrier@parrot.com
 * @brief Source for watching for a process' death
 *
 * Copyright (C) 2013 Parrot S.A.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <pidwatch.h>

#include <ut_utils.h>
#include <ut_file.h>

#include "io_platform.h"
#include "io_mon.h"
#include "io_src_pid.h"

/**
 * @def to_src
 * @brief Convert a source to it's pid source container
 */
#define to_src_pid(p) ut_container_of(p, struct io_src_pid, src)

/**
 * Source callback, reads the pidwatch event and notifies the client
 * @param src Underlying monitor source of the pid source
 */
static void pid_cb(struct io_src *src)
{
	pid_t pid_ret;
	struct io_src_pid *pid_src = to_src_pid(src);

	/* TODO treat I/O THEN errors */
	if (io_src_has_error(src))
		return;

	pid_ret = pidwatch_wait(src->fd, &pid_src->status);
	assert(pid_ret == pid_src->pid);
	if (-1 == pid_ret)
		return;

	io_src_pid_set_pid(pid_src, IO_SRC_PID_DISABLE);
	pid_src->cb(pid_src, pid_ret, pid_src->status);
}

int io_src_pid_init(struct io_src_pid *pid_src, io_pid_cb *cb)
{
	int pidfd;

	if (NULL == pid_src || NULL == cb)
		return -EINVAL;

	memset(pid_src, 0, sizeof(*pid_src));
	io_src_clean(&(pid_src->src));
	pidfd = pidwatch_create(SOCK_CLOEXEC | SOCK_NONBLOCK);
	if (0 > pidfd)
		goto out;

	pid_src->pid = IO_SRC_PID_DISABLE;
	pid_src->status = 0;
	pid_src->cb = cb;

	/* can fail only on parameters */
	return io_src_init(&(pid_src->src), pidfd, IO_IN, pid_cb);
out:
	io_src_pid_clean(pid_src);

	return -errno;
}

int io_src_pid_set_pid(struct io_src_pid *pid_src, pid_t pid)
{
	int ret;

	if (NULL == pid_src)
		return -EINVAL;

	pid_src->pid = pid;

	ret = pidwatch_set_pid(pid_src->src.fd, pid);
	if (-1 == ret)
		return -errno;

	return 0;
}

pid_t io_src_pid_get_pid(struct io_src_pid *pid_src)
{
	if (pid_src == NULL) {
		errno = EINVAL;
		return IO_SRC_PID_DISABLE;
	}

	if (pid_src->pid)
		errno = ESRCH;

	return pid_src->pid;
}

void io_src_pid_clean(struct io_src_pid *pid)
{
	if (NULL == pid)
		return;

	pid->pid = 0;
	pid->status = 0;
	pid->cb = NULL;
	ut_file_fd_close(&pid->src.fd);

	io_src_clean(&(pid->src));
}
