/**
 * \file
 * machine definitions
 *
 * Authors:
 *	Rodrigo Kumpera (kumpera@gmail.com)
 *
 * Copyright (c) 2011 Novell, Inc (http://www.novell.com)
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#ifndef __MONO_MONO_MACHINE_H__
#define __MONO_MONO_MACHINE_H__

/* C type matching the size of a machine register. Not always the same as 'int' */
/* Note that member 'p' of MonoInst must be the same type, as OP_PCONST is defined
 * as one of the OP_ICONST types, so inst_c0 must be the same as inst_p0
 */

#include "config.h"
#include <glib.h>

#if defined (__x86_64__) || defined (__arm64__) || defined (__aarch64__)
// Note: This is independent of __ILP32__ and __LP64__ and is not
// the same thing as HOST_AMD64 and HOST_ARM64. It is more like
// defined (HOST_X32) || defined (HOST_ARM6432).
typedef gint64 host_mgreg_t;
#else
typedef gssize host_mgreg_t;
#endif

// SIZEOF_REGISTER is target. mgreg_t is usually target, sometimes host.
// There is a mismatch in cross (AOT) compilers, and the
// never executed JIT and runtime truncate pointers.
// When casting to/from pointers, use gsize or gssize instead of mgreg_t.
// When dealing with register context, use host_mgreg_t instead of mgreg_t.
// Over conversion to host_mgreg_t causes cross compilers to generate incorrect code,
// i.e. with offsets/sizes in MonoContext.
// Under conversion to host_mgreg_t causes compiler errors/warnings about truncating pointers
// in dead code.
// Therefore under/no conversion is present -- rather, casts in dead code.
#if SIZEOF_REGISTER == 4
typedef gint32 mgreg_t;
#elif SIZEOF_REGISTER == 8
typedef gint64 mgreg_t;
#endif

#if TARGET_SIZEOF_VOID_P == 4
typedef gint32 target_mgreg_t;
#else
typedef gint64 target_mgreg_t;
#endif

/* Alignment for MonoArray.vector */
#if defined(_AIX)
/*
 * HACK: doubles in structs always align to 4 on AIX... even on 64-bit,
 * which is bad for aligned usage like what System.Array.FastCopy does
 */
typedef guint64 mono_64bitaligned_t;
#else
typedef double mono_64bitaligned_t;
#endif

#endif /* __MONO_MONO_MACHINE_H__ */
