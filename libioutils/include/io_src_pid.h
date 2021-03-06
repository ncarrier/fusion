/**
 * @file io_src_pid.h
 * @date 29 apr. 2013
 * @author nicolas.carrier@parrot.com
 * @brief Source for watching for a process' death
 *
 * Copyright (C) 2013 Parrot S.A.
 */
#ifndef IO_SRC_PID_H_
#define IO_SRC_PID_H_
#include "io_src.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def IO_SRC_PID_DISABLE
 * @brief Value to disable a pid source temporarily, with io_src_pid_set_pid()
 * This value equals to init's pid, because this process is unlikely to generate
 * death of process event...
 */
#define IO_SRC_PID_DISABLE 1

/**
 * @struct io_src_pid
 * @brief Pid source type
 */
struct io_src_pid;

/**
 * @typedef io_pid_cb
 * @brief Called when the monitored process has died
 * @param pid_src Signal source
 * @param pid Pid of the process which has just died
 * @param status Status of the process, same as that of waitpid(2)
 * @note the pid field of the io_src_pid context is invalid in the context of
 * the callback. One _must_ use the pid parameter instead
 */
typedef void (io_pid_cb)(struct io_src_pid *pid_src, pid_t pid, int status);

/**
 * @struct io_src_pid
 * @brief Pid source type
 */
struct io_src_pid {
	/** inner monitor source */
	struct io_src src;
	/** pid of the process being monitored */
	pid_t pid;
	/** status of the process when it dies. Same semantic as waitpid's */
	int status;
	/** user callback, notified when one of the registered signals occur */
	io_pid_cb *cb;
};

/**
 * Initializes a pid source.
 * @param pid_src Pid source to initialize
 * @param cb User calback, notified when the process dies
 * @return errno compatible negative value
 */
int io_src_pid_init(struct io_src_pid *pid_src, io_pid_cb *cb);

/**
 * Configures up a pid_src to monitor a given pid
 * @param pid_src Pid source to configure
 * @param pid Pid of the process to monitor, IO_SRC_PID_DISABLE to temporarily
 * disable the monitoring
 * @return errno-compatible negative value on error, 0 otherwise. -ESRCH if the
 * process doesn't exist anymore, or that it waits to be wait(2)-ed for.
 */
int io_src_pid_set_pid(struct io_src_pid *pid_src, pid_t pid);

/**
 * Returns the pid currently registered in the source
 * @param pid_src Pid source
 * @return pid of the process monitored, IO_SRC_PID_DISABLE with errno set
 * suitably. In particular ESRCH if no pid is monitored.
 */
pid_t io_src_pid_get_pid(struct io_src_pid *pid_src);

/**
 * Returns the underlying io_src of the pid source
 * @param pid Pid source
 * @return io_src of the pid source
 */
static inline struct io_src *io_src_pid_get_source(struct io_src_pid *pid)
{
	return NULL == pid ? NULL : &pid->src;
}

/**
 * Cleans up a pid source, by properly closing fd, zeroing fields etc...
 * @param pid Pid source
 */
void io_src_pid_clean(struct io_src_pid *pid);

#ifdef __cplusplus
}
#endif

#endif /* IO_SRC_PID_H_ */
