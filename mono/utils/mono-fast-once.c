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
//
// For each thing to initialize, there is an integer.
//
// It has three values:
//  - not initialized: max
//  - being initialized: max - 1
//  - the dynamic order in which it ended up initiaized: start at 1 and increasing
//    1 at a time, never skipping any values
//
// For the uninitialized and initializing values to be high
// instead of low is useful, as it helps handle 3 slow paths
// with one conditional branch.
//
// For each thread, there is an integer that indicates
// the highest initialized value it has seen.
// It starts at 0, meaning it has seen nothing.
//
// Besides being less than any initialization order (1), this is also
// less than uninitialized (max) or initializing (max - 1).
//
// Roughly the first time a thread encouters an object, it takes a lock.
// But not quite that bad.
//
// Whenever a thread encouters an object whose initialization order
// is greater than any object it has already seen, it takes a lock.
// This includes uninitialized and initializing.
// So they can go multiple at a time.
//
// Under the lock, it double checks.
//
// The lock + double check handles both the need
// to have a barrier before using a variable that was already initialized,
// and to serialize the decision as to if this thread should to the initialization.
// Therefore subtely removing the barrier from the fast path, because
// "enabling" the fast path requires an earlier barrier to increase
// the thread local.
//
// The relevant data is meant not be shared across dynamic link boundaries,
// lest the number of objects actually approach intmax, which, granted,
// isn't likely anyway.
//

#include "mono-fast-once.h"

static mono_fast_once_t mono_fast_once_global_order;
#define uninitialized (MONO_FAST_ONCE_INIT)
#define initializing  (MONO_FAST_ONCE_INIT - 1)

static
mono_fast_once_t
mono_fast_once_increment (volatile mono_fast_once_t* once)
{
	mono_fast_once_t result;
	mono_memory_barrier (); // probably redundant
	switch (sizeof (mono_fast_once_t)) {
	case 4:
		result = (mono_fast_once_t)mono_atomic_inc_i32 ((volatile gint32*)once);
		break;
	case 8:
		result = (mono_fast_once_t)mono_atomic_inc_i64 ((volatile gint64*)once);
		break;
	default:
		g_assert_not_reached ();
	}
	mono_memory_barrier (); // probably redundant
	return result;
}

static
mono_fast_once_t
mono_fast_once_cas (volatile mono_fast_once_t* once, mono_fast_once_t exch, mono_fast_once_t comp)
{
	mono_fast_once_t result;
	mono_memory_barrier (); // probably redundant
	switch (sizeof (mono_fast_once_t)) {
	case 4:
		result = (mono_fast_once_t)mono_atomic_cas_i32 ((volatile gint32*)once, exch, comp);
		break;
	case 8:
		result = (mono_fast_once_t)mono_atomic_cas_i64 ((volatile gint64*)once, exch, comp);
		break;
	default:
		g_assert_not_reached ();
	}
	mono_memory_barrier (); // probably redundant
	return result;
}

// For initialization that must not occur multiple times.
void
mono_fast_exactly_once_not_inline (mono_fast_once_t *object, void (*initialize)((void))
{
	if (*object > thread)
	{
		// We have not seen an object with this high
		// of an initialization order yet or it
		// is uninitialized or initializing.
		//
		// Lock and double check.
		//
		// This lock serves also as a barrier, in
		// case the object is already initialized or is being initialized.

		lock (&mu);

		if (*object == uninitialized)  // max
		{
			*object = initializing; // max - 1
			unlock(&mu);

			// Run initialization outside of lock.
			initialize ();

			lock(&mu);

			// This is atomic to synchronize with mono_fast_at_least_once_not_inline.
			*object = mono_fast_once_increment (&global_order); // starts at 1, never hits max - 1

			broadcast (&cv);
		}
		else
		{
			// It is already initialized or initializing.
			while (*object == initializing)
				wait (&cv, &mu);
		}
		// lock still held
		// thread can see all objects up to this order without additional lock or barrier.
		//
		// This read is why the locking cannot
		// devolve to just atomic operations.
		thread = global_order;
		//thread = *object;
		unlock (&mu); // also serves as a barrier
	}
	// Either it was long since initialized
	// or we initialized it or we waited for another
	// thread to finish. In the later 2 cases, we had a lock/barrier.
	//
	// "long since" is defined as, we had to take a lock/barrier
	// to update our understanding of what is already initialized.
}

// For initialization that can occur multiple times, concurrently.
// This is not in the literature but is derived from previous.
void
mono_fast_at_least_once_not_inline (mono_fast_once_t *object, void (*initialize)((void))
{
	if (*object > thread)
	{
		// We have not seen an object with this high
		// of an initialization order yet or it
		// is uninitialized or initializing.
		//
		// Double check.

		if (mono_fast_once_cas (object, initializing, uninitialized) == uninitialized)
		{
			// This is the first thread to see it uninitialized.
			// This thread will mark it initialized.
			// Others will initialize but not mark it as initialized.

			initialize ();

			*object = mono_fast_once_increment (&global_order); // starts at 1, never hits max - 1
		}
		else
		{
			// It is already initialized or initializing.
			if (*object == initializing)
			{
				// Proceed to initialize it on this thread, but
				// do not record an order for it.

				initialize ();
			}
			else
			{
				// The object is initialized.
				// thread can see all objects up to this order without additional lock or barrier.
				thread = global_order;
				//thread = *object;
				mono_memory_barrier ();
			}
		}
	}
	// Either it was long since initialized or we initialized it.
	//
	// "long since" is defined as, we had to take a lock/barrier
	// to update our understanding of what is already initialized.
}
