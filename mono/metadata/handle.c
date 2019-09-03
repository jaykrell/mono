/**
 * \file
 * Handle to object in native code
 *
 * Authors:
 *  - Ludovic Henry <ludovic@xamarin.com>
 *  - Aleksey Klieger <aleksey.klieger@xamarin.com>
 *  - Rodrigo Kumpera <kumpera@xamarin.com>
 *
 * Copyright 2016 Dot net foundation.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>
#include <glib.h>

#include <mono/metadata/handle.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/gc-internals.h>
#include <mono/utils/atomic.h>
#include <mono/utils/mono-lazy-init.h>
#include <mono/utils/mono-threads.h>
#ifdef HAVE_BACKTRACE_SYMBOLS
#include <execinfo.h>
#endif

/* TODO (missing pieces)

Add counters for:
	number of stack marks
	stack marks per icall
	mix/max/avg size of stack marks
	handle stack wastage

Shrink the handles stack in mono_handle_stack_scan
Add a boehm implementation

TODO (things to explore):

There's no convenient way to wrap the object allocation function.
Right now we do this:
	MonoCultureInfoHandle culture = MONO_HANDLE_NEW (MonoCultureInfo, mono_object_new_checked (domain, klass, error));

Maybe what we need is a round of cleanup around all exposed types in the runtime to unify all helpers under the same hoof.
Combine: MonoDefaults, GENERATE_GET_CLASS_WITH_CACHE, TYPED_HANDLE_DECL and friends.
	This would solve the age old issue of making it clear which types are optional and tell that to the linker.
	We could then generate neat type safe wrappers.
*/

/*
 * NOTE: Async suspend
 * 
 * If we are running with cooperative GC, all the handle stack
 * manipulation will complete before a GC thread scans the handle
 * stack. If we are using async suspend, however, a thread may be
 * trying to allocate a new handle, or unwind the handle stack when
 * the GC stops the world.
 *
 * In particular, we need to ensure that if the mutator thread is
 * suspended while manipulating the handle stack, the stack is in a
 * good enough state to be scanned.  In particular, the size of each
 * chunk should be updated before an object is written into the
 * handle, and chunks to be scanned (between bottom and top) should
 * always be valid.
 */

static MonoHandleValue*
new_handle_stack (void)
{
	return 0;
}

static void
free_handle_stack (MonoHandleValue *stack)
{
	g_free (stack);
}

static HandleChunk*
new_handle_chunk (void)
{
	return g_new (HandleChunk, 1);
}

static void
free_handle_chunk (HandleChunk *chunk)
{
	g_free (chunk);
}

const MonoObjectHandle mono_null_value_handle = { 0 };

#define THIS_IS_AN_OK_NUMBER_OF_HANDLES 100

static HandleChunkElem*
chunk_element (HandleChunk *chunk, int idx)
{
	return &chunk->elems[idx];
}


#ifdef MONO_HANDLE_TRACK_OWNER
#ifdef HAVE_BACKTRACE_SYMBOLS
#define SET_BACKTRACE(btaddrs) do {					\
	backtrace(btaddrs, 7);						\
	} while (0)
#else
#define SET_BACKTRACE(btaddrs) 0
#endif
#define SET_OWNER(chunk,idx) do { (chunk)->elems[(idx)].owner = owner; SET_BACKTRACE (&((chunk)->elems[(idx)].backtrace_ips[0])); } while (0)
#else
#define SET_OWNER(chunk,idx) do { } while (0)
#endif

#ifdef MONO_HANDLE_TRACK_SP
#define SET_SP(handles,chunk,idx) do { (chunk)->elems[(idx)].alloc_sp = handles->stackmark_sp; } while (0)
#else
#define SET_SP(handles,chunk,idx) do { } while (0)
#endif

#ifdef MONO_HANDLE_TRACK_SP
void
mono_handle_chunk_leak_check (MonoHandleValue *handles) {
	if (handles->stackmark_sp) {
		/* walk back from the top to the topmost non-empty chunk */
		HandleChunk *c = handles->top;
		while (c && c->size <= 0 && c != handles->bottom) {
			c = c->prev;
		}
		if (c == NULL || c->size == 0)
			return;
		g_assert (c && c->size > 0);
		HandleChunkElem *e = chunk_element (c, c->size - 1);
		if (e->alloc_sp < handles->stackmark_sp) {
			/* If we get here, the topmost object on the handle stack was
			 * allocated from a function that is deeper in the call stack than
			 * the most recent HANDLE_FUNCTION_ENTER.  That means it was
			 * probably not wrapped in a HANDLE_FUNCTION_ENTER/_RETURN pair
			 * and will never be reclaimed. */
			g_warning ("Handle %p (object = %p) (allocated from \"%s\") is leaking.\n", e, e->o,
#ifdef MONO_HANDLE_TRACK_OWNER
				   e->owner
#else
				   "<unknown owner>"
#endif
				);
		}
	}
}
#endif

// There are deliberately locals and a constant NULL global with this same name.
#ifdef __cplusplus
extern MonoThreadInfo * const mono_thread_info_current_var = NULL;
#else
MonoThreadInfo * const mono_thread_info_current_var = NULL;
#endif

/* Actual handles implementation */
MonoRawHandle
#ifndef MONO_HANDLE_TRACK_OWNER
mono_handle_new (MonoObject *obj, MonoThreadInfo *info)
#else
mono_handle_new (MonoObject *obj, MonoThreadInfo *info, const char *owner)
#endif
{
	info = info ? info : mono_thread_info_current ();

	MonoHandleValue *handles = info->handle_stack;
	HandleChunk *top = handles->top;
#ifdef MONO_HANDLE_TRACK_SP
	mono_handle_chunk_leak_check (handles);
#endif

	// FIXME: Since we scan the handle stack inprecisely, some of the
	// membars could be removed

retry:
	if (G_LIKELY (top->size < OBJECTS_PER_HANDLES_CHUNK)) {
		int idx = top->size;
		gpointer* objslot = &top->elems [idx].o;
		/* can be interrupted anywhere here, so:
		 * 1. make sure the new slot is null
		 * 2. make the new slot scannable (increment size)
		 * 3. put a valid object in there
		 *
		 * (have to do 1 then 3 so that if we're interrupted
		 * between 1 and 2, the object is still live)
		 */
		*objslot = NULL;
		SET_OWNER (top,idx);
		SET_SP (handles, top, idx);
		mono_memory_write_barrier ();
		top->size++;
		mono_memory_write_barrier ();
		*objslot = obj;
		return objslot;
	}
	if (G_LIKELY (top->next)) {
		top->next->size = 0;
		/* make sure size == 0 is visible to a GC thread before it sees the new top */
		mono_memory_write_barrier ();
		top = top->next;
		handles->top = top;
		goto retry;
	}
	HandleChunk *new_chunk = new_handle_chunk ();
	new_chunk->size = 0;
	new_chunk->prev = top;
	new_chunk->next = NULL;
	/* make sure size == 0 before new chunk is visible */
	mono_memory_write_barrier ();
	top->next = new_chunk;
	handles->top = new_chunk;
	goto retry;
}

void
mono_handle_stack_free (MonoHandleValue *stack)
{
	if (!stack)
		return;
	HandleChunk *c = stack->bottom;
	stack->top = stack->bottom = NULL;
	mono_memory_write_barrier ();
	while (c) {
		HandleChunk *next = c->next;
		free_handle_chunk (c);
		c = next;
	}
	free_handle_chunk (c);
	free_handle_stack (stack);
}

void
mono_handle_stack_free_domain (MonoHandleValue *stack, MonoDomain *domain)
{
	/* Called by the GC while clearing out objects of the given domain from the heap. */
	/* If there are no handles-related bugs, there is nothing to do: if a
	 * thread accessed objects from the domain it was aborted, so any
	 * threads left alive cannot have any handles that point into the
	 * unloading domain.  However if there is a handle leak, the handle stack is not */
	if (!stack)
		return;
	/* Root domain only unloaded when mono is shutting down, don't need to check anything */
	if (domain == mono_get_root_domain () || mono_runtime_is_shutting_down ())
		return;

	for (stack ; stack ; stack = stack->previous)
		g_assert (!stack->object || mono_object_domain (stack->object) != domain);
}

static void
check_handle_stack_monotonic (MonoHandleValue *stack)
{
	/* check that every allocated handle in the current handle stack is at no higher in the native stack than its predecessors */
	// FIXME stack direction, i.e. port to HPPA.
	// Other parts of mono depend on a grow-down stack (where GC aligns the stack up)
	for (stack ; stack ; stack = stack->previous)
		g_assert (!stack->previous || stack < stack->previous);
}

void
mono_handle_stack_scan (MonoHandleValue *stack, GcScanFunc func, gpointer gc_data, gboolean precise, gboolean check)
{
	if (check) /* run just once (per handle stack) per GC */
		check_handle_stack_monotonic (stack);

	/*
	 * We're called twice, on the precise pass we do nothing.
	 * On the inprecise pass, we pin the objects pointed to by the handles.
	 * Note that if we're running, we know the world is stopped.
	 */
	if (precise)
		return;

	for (stack ; stack ; stack = stack->previous) {
		if (stack->o)
			func (stack->o, gc_data);
	}
}

void
mono_handle_frame_pop (MonoHandleFrame *frame)
{
	// FIXME? Where are barriers needed here?

	g_assert (frame);

	MonoHandleFrame *previous = frame->previous;
	frame->thread.handles.frame = previous;
	frame->thread.handles.stack = previous ? previous->stack : NULL;

	if (frame->count > THIS_IS_AN_OK_NUMBER_OF_HANDLES)
		g_warning ("%s USED %d handles\n", frame->func_name, size);
}

/*
 * Pop the stack until @frame and make @value the top value.
 *
 * @return the new handle for what @value points to 
 */
MonoRawHandle
mono_stack_mark_pop_value (MonoHandleFrame *frame, MonoRawHandle value)
{
	g_assert (frame);

	MonoHandleFrame *previous = frame->previous;

	g_assert (previous);

	mono_handle_frame_pop (frame);

	// Move handle from current frame to previous frame.o

	previous->return_value = value ? *(MonoObject**)value : NULL;
	return &previous->return_value;
}

/* Temporary place for some of the handle enabled wrapper functions*/

MonoStringHandle
mono_string_new_handle (MonoDomain *domain, const char *data, MonoError *error)
{
	return MONO_HANDLE_NEW (MonoString, mono_string_new_checked (domain, data, error));
}

MonoArrayHandle
mono_array_new_handle (MonoDomain *domain, MonoClass *eclass, uintptr_t n, MonoError *error)
{
	return MONO_HANDLE_NEW (MonoArray, mono_array_new_checked (domain, eclass, n, error));
}

MonoArrayHandle
mono_array_new_full_handle (MonoDomain *domain, MonoClass *array_class, uintptr_t *lengths, intptr_t *lower_bounds, MonoError *error)
{
	return MONO_HANDLE_NEW (MonoArray, mono_array_new_full_checked (domain, array_class, lengths, lower_bounds, error));
}

uint32_t
mono_gchandle_from_handle (MonoObjectHandle handle, mono_bool pinned)
{
	// FIXME This used to check for out of scope handles.
	// Stack-based handles coming from icall wrappers do not
	// work with that. This functionality can be largely restored
	// by introducing a tag bit in handles -- 0 for managed stack-based,
	// 1 for native TLS-based, having MONO_HANDLE_RAW clear it, and only
	// doing the former checking for native TLS-based handles.
	return mono_gchandle_new_internal (MONO_HANDLE_RAW (handle), pinned);
}

MonoObjectHandle
mono_gchandle_get_target_handle (uint32_t gchandle)
{
	return MONO_HANDLE_NEW (MonoObject, mono_gchandle_get_target_internal (gchandle));
}

gpointer
mono_array_handle_pin_with_size (MonoArrayHandle handle, int size, uintptr_t idx, uint32_t *gchandle)
{
	g_assert (gchandle != NULL);
	*gchandle = mono_gchandle_from_handle (MONO_HANDLE_CAST(MonoObject,handle), TRUE);
	MonoArray *raw = MONO_HANDLE_RAW (handle);
	return mono_array_addr_with_size_internal (raw, size, idx);
}

gunichar2*
mono_string_handle_pin_chars (MonoStringHandle handle, uint32_t *gchandle)
{
	g_assert (gchandle != NULL);
	*gchandle = mono_gchandle_from_handle (MONO_HANDLE_CAST (MonoObject, handle), TRUE);
	MonoString *raw = MONO_HANDLE_RAW (handle);
	return mono_string_chars_internal (raw);
}

gpointer
mono_object_handle_pin_unbox (MonoObjectHandle obj, uint32_t *gchandle)
{
	g_assert (!MONO_HANDLE_IS_NULL (obj));
	MonoClass *klass = mono_handle_class (obj);
	g_assert (m_class_is_valuetype (klass));
	*gchandle = mono_gchandle_from_handle (obj, TRUE);
	return mono_object_unbox_internal (MONO_HANDLE_RAW (obj));
}

//FIXME inline
void
mono_array_handle_memcpy_refs (MonoArrayHandle dest, uintptr_t dest_idx, MonoArrayHandle src, uintptr_t src_idx, uintptr_t len)
{
	mono_array_memcpy_refs_internal (MONO_HANDLE_RAW (dest), dest_idx, MONO_HANDLE_RAW (src), src_idx, len);
}

gboolean
mono_handle_stack_is_empty (MonoHandleValue *stack)
{
	return stack->top == stack->bottom && stack->top->size == 0;
}

//FIXME inline
gboolean
mono_gchandle_target_equal (uint32_t gchandle, MonoObjectHandle equal)
{
	// This function serves to reduce coop handle creation.
	MONO_HANDLE_SUPPRESS_SCOPE (1);
	return mono_gchandle_get_target_internal (gchandle) == MONO_HANDLE_RAW (equal);
}

//FIXME inline
void
mono_gchandle_target_is_null_or_equal (uint32_t gchandle, MonoObjectHandle equal, gboolean *is_null,
	gboolean *is_equal)
{
	// This function serves to reduce coop handle creation.
	MONO_HANDLE_SUPPRESS_SCOPE (1);
	MonoObject *target = mono_gchandle_get_target_internal (gchandle);
	*is_null = target == NULL;
	*is_equal = target == MONO_HANDLE_RAW (equal);
}

//FIXME inline
void
mono_gchandle_set_target_handle (guint32 gchandle, MonoObjectHandle obj)
{
	mono_gchandle_set_target (gchandle, MONO_HANDLE_RAW (obj));
}

//FIXME inline
guint32
mono_gchandle_new_weakref_from_handle (MonoObjectHandle handle)
{
	return mono_gchandle_new_weakref_internal (MONO_HANDLE_SUPPRESS (MONO_HANDLE_RAW (handle)), FALSE);
}

//FIXME inline
int
mono_handle_hash (MonoObjectHandle object)
{
	return mono_object_hash_internal (MONO_HANDLE_SUPPRESS (MONO_HANDLE_RAW (object)));
}

//FIXME inline
guint32
mono_gchandle_new_weakref_from_handle_track_resurrection (MonoObjectHandle handle)
{
	return mono_gchandle_new_weakref_internal (MONO_HANDLE_SUPPRESS (MONO_HANDLE_RAW (handle)), TRUE);
}

//FIXME inline
void
mono_handle_array_getref (MonoObjectHandleOut dest, MonoArrayHandle array, uintptr_t index)
{
	MONO_HANDLE_SUPPRESS (g_assert (dest.__raw));
	MONO_HANDLE_SUPPRESS (*dest.__raw = (MonoObject*)mono_array_get_internal (MONO_HANDLE_RAW (array), gpointer, index));
}
