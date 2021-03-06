/**
 * @file io_platform.h
 * @date Nov 26, 2012
 * @author nicolas.carrier@parrot.com
 * @brief Compatibility module, define constants and function lacking on some
 * ancient tool chains
 *
 * Copyright (C) 2012 Parrot S.A.
 */

#ifndef IO_PLATFORM_H_
#define IO_PLATFORM_H_
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <signal.h>

/* Give us a chance to use regular definitions: */
#include <fcntl.h>       /* O_CLOEXEC */
#include <sys/epoll.h>   /* EPOLL_CLOEXEC */
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/inotify.h>

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef O_CLOEXEC
#ifdef __arm__
	/* value taken from linux kernel header
	 * include/asm-generic/fcntl.h */
	#define O_CLOEXEC 02000000
#else
	#error O_CLOEXEC not defined !
#endif
#endif

/* for epoll_create1 */
#ifndef EPOLL_CLOEXEC
/**
 * @def EPOLL_CLOEXEC
 * @brief Set the flag O_CLOEXEC at poll fd's creation
 */
#define EPOLL_CLOEXEC O_CLOEXEC
#endif

/* for signalfd */
#ifndef SFD_CLOEXEC
/**
 * @def SFD_CLOEXEC
 * @brief Set the flag O_CLOEXEC at signal fd's creation
 */
#define SFD_CLOEXEC O_CLOEXEC
#endif

/* for signalfd */
#ifndef SFD_NONBLOCK
/**
 * @def SFD_NONBLOCK
 * @brief Set the flag O_NONBLOCK at signal fd's creation
 */
#define SFD_NONBLOCK O_NONBLOCK
#endif

/* for timerfd */
#ifndef TFD_NONBLOCK
/**
 * @def TFD_NONBLOCK
 * @brief Set the flag O_NONBLOCK at timer's creation
 */
#define TFD_NONBLOCK O_NONBLOCK
#endif

#ifndef TFD_CLOEXEC
/**
 * @def TFD_CLOEXEC
 * @brief Set the flag O_CLOEXEC at timer's creation
 */
#define TFD_CLOEXEC O_CLOEXEC
#endif

/* for socket */
#ifndef SOCK_CLOEXEC
/**
 * @def SOCK_CLOEXEC
 * @brief Set the flag O_CLOEXEC at socket's creation
 */
#define SOCK_CLOEXEC O_CLOEXEC
#endif

#ifndef SOCK_NONBLOCK
/**
 * @def SOCK_NONBLOCK
 * @brief Set the flag O_NONBLOCK at socket's creation
 */
#define SOCK_NONBLOCK O_NONBLOCK
#endif

/* for inotify */
#ifndef IN_CLOEXEC
/**
 * @def IN_CLOEXEC
 * @brief Set the flag O_CLOEXEC at inotify fd's creation
 */
#define IN_CLOEXEC O_CLOEXEC
#endif

#ifndef IN_NONBLOCK
/**
 * @def IN_NONBLOCK
 * @brief Set the flag O_NONBLOCK at inotify fd's creation
 */
#define IN_NONBLOCK O_NONBLOCK
#endif

/**
 * Wrapper around dup3, defining it on toolchains where it is missing
 * @see dup3
 */
int io_dup3(int oldfd, int newfd, int flags);

/**
 * Wrapper around epoll_create1, defining it on toolchains where it is missing
 * @see epoll_create1
 * @param flags can be EPOLL_CLOEXEC
 * @return -1 is returned, with errno set, file descriptor created on success
 */
int io_epoll_create1(int flags);

/**
 * Wrapper around signalfd which performs separately the opening of the file
 * descriptor and the setting of the flags, if an error is encountered when
 * performing both two at once.
 * @see signalfd
 */
int io_signalfd(int fd, const sigset_t *mask, int flags);

/**
 * Wrapper around io_pipe2, defining it on toolchains where it is missing
 * @see pipe2
 */
int io_pipe2(int pipefd[2], int flags);

/**
 * Wrapper around io_pipe1, defining it on toolchains where it is missing
 * @see pipe2
 */
int io_inotify_init1(int flags);

#ifdef __cplusplus
}
#endif

#endif /* IO_PLATFORM_H_ */
