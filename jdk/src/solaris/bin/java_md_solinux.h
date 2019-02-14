/*
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#ifndef JAVA_MD_SOLINUX_H
#define JAVA_MD_SOLINUX_H

#if defined(HAVE_GETHRTIME) || defined(__FreeBSD__)
/*
 * Support for doing cheap, accurate interval timing.
 */
#ifdef HAVE_GETHRTIME
#include <sys/time.h>
#else /* __FreeBSD__ */
#include <time.h>
#define gethrtime() __extension__ ({ \
    struct timespec tp; \
    clock_gettime(CLOCK_MONOTONIC, &tp); \
    (uint64_t)tp.tv_sec*1000000 + tp.tv_nsec/1000; \
})
#endif /* HAVE_GETHRTIME */
#define CounterGet()              (gethrtime()/1000)
#define Counter2Micros(counts)    (counts)
#else /* ! HAVE_GETHRTIME && ! __FreeBSD__ */
#define CounterGet()              (0)
#define Counter2Micros(counts)    (1)
#endif /* HAVE_GETHRTIME || __FreeBSD__ */

/* pointer to environment */
extern char **environ;

/*
 *      A collection of useful strings. One should think of these as #define
 *      entries, but actual strings can be more efficient (with many compilers).
 */
#ifdef __solaris__
static const char *system_dir   = "/usr/jdk";
static const char *user_dir     = "/jdk";
#elif defined(__FreeBSD__)
static const char *system_dir  = PACKAGE_PATH "/openjdk8";
static const char *user_dir    = "/java";
#else /* !__solaris__, i.e. Linux, AIX,.. */
static const char *system_dir   = "/usr/java";
static const char *user_dir     = "/java";
#endif

#include <dlfcn.h>
#ifdef __solaris__
#include <thread.h>
#else
#include <pthread.h>
#endif

#endif /* JAVA_MD_SOLINUX_H */
