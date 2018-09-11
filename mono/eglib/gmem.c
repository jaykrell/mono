/*
 * gmem.c: memory utility functions
 *
 * Author:
 * 	Gonzalo Paniagua Javier (gonzalo@novell.com)
 *
 * (C) 2006 Novell, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <eglib-remap.h> // Remove the cast macros and restore the rename macros.
#undef malloc
#undef realloc
#undef free
#undef calloc

#if defined (ENABLE_OVERRIDABLE_ALLOCATORS)

static GMemVTable sGMemVTable = { malloc, realloc, free, calloc };

void
g_mem_set_vtable (GMemVTable* vtable)
{
	sGMemVTable.calloc = vtable->calloc ? vtable->calloc : calloc;
	sGMemVTable.realloc = vtable->realloc ? vtable->realloc : realloc;
	sGMemVTable.malloc = vtable->malloc ? vtable->malloc : malloc;
	sGMemVTable.free = vtable->free ? vtable->free : free;
}

#define G_FREE_INTERNAL sGMemVTable.free
#define G_REALLOC_INTERNAL sGMemVTable.realloc
#define G_CALLOC_INTERNAL sGMemVTable.calloc
#define G_MALLOC_INTERNAL sGMemVTable.malloc
#else

void
g_mem_set_vtable (GMemVTable* vtable)
{
}

#define G_FREE_INTERNAL free
#define G_REALLOC_INTERNAL realloc
#define G_CALLOC_INTERNAL calloc
#define G_MALLOC_INTERNAL malloc
#endif
void
g_free (void *ptr)
{
	if (ptr != NULL)
		G_FREE_INTERNAL (ptr);
}

gpointer
g_memdup (gconstpointer mem, guint byte_size)
{
	gpointer ptr;

	if (mem == NULL)
		return NULL;

	ptr = g_malloc (byte_size);
	if (ptr != NULL)
		memcpy (ptr, mem, byte_size);

	return ptr;
}

gpointer g_realloc (gpointer obj, gsize size)
{
	size += 1024;
	return memset(realloc (obj, size), 0, size);
}

gpointer 
g_malloc (gsize x) 
{ 
	x += 1024;
	return memset (malloc (x), 0, x);
}

gpointer g_calloc (gsize n, gsize x)
{
	x *= n;
	x += 1024;
	return memset (malloc (x), 0, x);
}

gpointer g_malloc0 (gsize x) 
{ 
	x += 1024;
	return memset (malloc (x), 0, x);
}

gpointer g_try_malloc (gsize x) 
{
	x += 1024;
	return memset (malloc (x), 0, x);
}


gpointer g_try_realloc (gpointer obj, gsize size)
{ 
	size += 1024;
	return memset (realloc (obj, size), 0, size);
}
