/*
 * Copyright (c) 1997, 2018, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
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
 *
 */

#ifndef SHARE_VM_UTILITIES_GLOBALDEFINITIONS_VISCPP_HPP
#define SHARE_VM_UTILITIES_GLOBALDEFINITIONS_VISCPP_HPP

#include "prims/jni.h"

// This file holds compiler-dependent includes,
// globally used constants & types, class (forward)
// declarations and a few frequently used utility functions.

# include <ctype.h>
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
# include <stddef.h>// for offsetof
# include <io.h>    // for stream.cpp
# include <float.h> // for _isnan
# include <stdio.h> // for va_list
# include <time.h>
# include <fcntl.h>
# include <limits.h>
// Need this on windows to get the math constants (e.g., M_PI).
#define _USE_MATH_DEFINES
# include <math.h>

// 4810578: varargs unsafe on 32-bit integer/64-bit pointer architectures
// When __cplusplus is defined, NULL is defined as 0 (32-bit constant) in
// system header files.  On 32-bit architectures, there is no problem.
// On 64-bit architectures, defining NULL as a 32-bit constant can cause
// problems with varargs functions: C++ integral promotion rules say for
// varargs, we pass the argument 0 as an int.  So, if NULL was passed to a
// varargs function it will remain 32-bits.  Depending on the calling
// convention of the machine, if the argument is passed on the stack then
// only 32-bits of the "NULL" pointer may be initialized to zero.  The
// other 32-bits will be garbage.  If the varargs function is expecting a
// pointer when it extracts the argument, then we may have a problem.
//
// Solution: For 64-bit architectures, redefine NULL as 64-bit constant 0.
#ifdef _LP64
#undef NULL
// 64-bit Windows uses a P64 data model (not LP64, although we define _LP64)
// Since longs are 32-bit we cannot use 0L here.  Use the Visual C++ specific
// 64-bit integer-suffix (i64) instead.
#define NULL 0i64
#else
#ifndef NULL
#define NULL 0
#endif
#endif

// NULL vs NULL_WORD:
// On Linux NULL is defined as a special type '__null'. Assigning __null to
// integer variable will cause gcc warning. Use NULL_WORD in places where a
// pointer is stored as integer value.
#define NULL_WORD NULL

// MS Visual Studio 10 doesn't seem to have INT64_C and UINT64_C even with
// __STDC_CONSTANT_MACROS defined.
#if _MSC_VER <= 1600
#define INT64_C(c)  (c ## i64)
#define UINT64_C(c) (c ## ui64)
#endif

// Compiler-specific primitive types
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#ifdef _WIN64
typedef unsigned __int64 uintptr_t;
#else
typedef unsigned int uintptr_t;
#endif
typedef signed   __int8  int8_t;
typedef signed   __int16 int16_t;
typedef signed   __int32 int32_t;
typedef signed   __int64 int64_t;
#ifdef _WIN64
typedef signed   __int64 intptr_t;
typedef signed   __int64 ssize_t;
#else
typedef signed   int intptr_t;
typedef signed   int ssize_t;
#endif

#ifndef UINTPTR_MAX
#ifdef _WIN64
#define UINTPTR_MAX _UI64_MAX
#else
#define UINTPTR_MAX _UI32_MAX
#endif
#endif

//----------------------------------------------------------------------------------------------------
// Additional Java basic types

typedef unsigned char    jubyte;
typedef unsigned short   jushort;
typedef unsigned int     juint;
typedef unsigned __int64 julong;


//----------------------------------------------------------------------------------------------------
// Non-standard stdlib-like stuff:
inline int strcasecmp(const char *s1, const char *s2) { return _stricmp(s1,s2); }
inline int strncasecmp(const char *s1, const char *s2, size_t n) {
  return _strnicmp(s1,s2,n);
}


//----------------------------------------------------------------------------------------------------
// Debugging

#if _WIN64
extern "C" void breakpoint();
#define BREAKPOINT ::breakpoint()
#else
#define BREAKPOINT __asm { int 3 }
#endif

//----------------------------------------------------------------------------------------------------
// Checking for nanness

inline int g_isnan(jfloat  f)                    { return _isnan(f); }
inline int g_isnan(jdouble f)                    { return _isnan(f); }

//----------------------------------------------------------------------------------------------------
// Checking for finiteness

inline int g_isfinite(jfloat  f)                 { return _finite(f); }
inline int g_isfinite(jdouble f)                 { return _finite(f); }

//----------------------------------------------------------------------------------------------------
// Constant for jlong (specifying an long long constant is C++ compiler specific)

// Build a 64bit integer constant on with Visual C++
#define CONST64(x)  (x ## i64)
#define UCONST64(x) ((uint64_t)CONST64(x))

const jlong min_jlong = CONST64(0x8000000000000000);
const jlong max_jlong = CONST64(0x7fffffffffffffff);

//----------------------------------------------------------------------------------------------------
// Miscellaneous

// Visual Studio 2005 deprecates POSIX names - use ISO C++ names instead
#if _MSC_VER >= 1400
#define open _open
#define close _close
#define read  _read
#define write _write
#define lseek _lseek
#define unlink _unlink
#define strdup _strdup
#endif

#if _MSC_VER < 1800
// Fixes some wrong warnings about 'this' : used in base member initializer list
#pragma warning( disable : 4355 )
#endif

#pragma warning( disable : 4100 ) // unreferenced formal parameter
#pragma warning( disable : 4127 ) // conditional expression is constant
#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#pragma warning( disable : 4244 ) // possible loss of data
#pragma warning( disable : 4512 ) // assignment operator could not be generated
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union (needed in windows.h)
#pragma warning( disable : 4511 ) // copy constructor could not be generated
#pragma warning( disable : 4291 ) // no matching operator delete found; memory will not be freed if initialization thows an exception
#ifdef CHECK_UNHANDLED_OOPS
#pragma warning( disable : 4521 ) // class has multiple copy ctors of a single type
#pragma warning( disable : 4522 ) // class has multiple assignment operators of a single type
#endif // CHECK_UNHANDLED_OOPS
#if _MSC_VER >= 1400
#pragma warning( disable : 4996 ) // unsafe string functions. Same as define _CRT_SECURE_NO_WARNINGS/_CRT_SECURE_NO_DEPRICATE
#endif

// Portability macros
#define PRAGMA_INTERFACE
#define PRAGMA_IMPLEMENTATION
#define PRAGMA_IMPLEMENTATION_(arg)
#define VALUE_OBJ_CLASS_SPEC    : public _ValueObj

// Formatting.
#define FORMAT64_MODIFIER "I64"

// Visual Studio doesn't provide inttypes.h so provide appropriate definitions here.
// The 32 bits ones might need I32 but seem to work ok without it.
#define PRId32       "d"
#define PRIu32       "u"
#define PRIx32       "x"

#define PRId64       "I64d"
#define PRIu64       "I64u"
#define PRIx64       "I64x"

#ifdef _LP64
#define PRIdPTR       "I64d"
#define PRIuPTR       "I64u"
#define PRIxPTR       "I64x"
#else
#define PRIdPTR       "d"
#define PRIuPTR       "u"
#define PRIxPTR       "x"
#endif

#define offset_of(klass,field) offsetof(klass,field)

// Inlining support
// MSVC has '__declspec(noinline)' but according to the official documentation
// it only applies to member functions. There are reports though which pretend
// that it also works for freestanding functions.
#define NOINLINE     __declspec(noinline)
#define ALWAYSINLINE __forceinline

#endif // SHARE_VM_UTILITIES_GLOBALDEFINITIONS_VISCPP_HPP
