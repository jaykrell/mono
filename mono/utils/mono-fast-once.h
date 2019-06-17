// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2660.htm
//
// but with cosmetic changes (names, comments, important typos).
// Same algorithm.
//
// Copyright (c) 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met.
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of Google Inc. nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MONO_FAST_ONCE_H
#define MONO_FAST_ONCE_H

// Int should suffice here, for atomiticy and range.

#ifdef HOST_WIN32

#include <stddef.h>
#include <limits.h>

typedef volatile size_t     mono_fast_once_t;
#define MONO_FAST_ONCE_INIT SIZE_MAX

#else

#include <signal.h>
#include <stdint.h>

typedef volatile sig_atomic_t	mono_fast_once_t;
#define MONO_FAST_ONCE_INIT	SIG_ATOMIC_MAX

#endif

extern MONO_KEYWORD_THREAD mono_fast_once_t mono_fast_once_thread;

void
mono_fast_exactly_once_not_inline (mono_fast_once_t *once, void (*func)(void));

// This is for initialization that must occur at most once.
// It is tempting to allow a void* context parameter to func, however
// this compromises the limited range of mono_fast_once_t.
static inline void
mono_fast_exactly_once (mono_fast_once_t *once, void (*func)(void))
{
	if (*once > mono_fast_once_thread) {
		// We have not seen an object with this high
		// of an initialization order yet or it
		// is uninitialized or initializing.
		//
		// Lock and double check.
		mono_fast_exactly_once_not_inline (once, func);
	}
}

// This is for initialization that can occur more than once, concurrently,
// and should produce hte same result each time, for which should
// run a minimal number of times for efficiency.
//
// There are many examples.
//
// This is weaker than exactly_once, in that once
// an object enters the "initializing" state, other
// threads need not wait for it to become initialized. Instead
// they cna proceed and do the same initialization.
void
mono_fast_at_least_once_not_inline (mono_fast_once_t *once, void (*func)(void));

// This is for initialization that must occur at most once.
// It is tempting to allow a void* context parameter to func, however
// this compromises the limited range of mono_fast_once_t.
static inline void
mono_fast_at_least_once (mono_fast_once_t *once, void (*func)(void))
{
	if (*once > mono_fast_once_thread) {
		// We have not seen an object with this high
		// of an initialization order yet or it
		// is uninitialized or initializing.
		//
		// Lock and double check.
		mono_fast_at_least_once_not_inline (once, func);
	}
}

#endif MONO_FAST_ONCE_H
