/**
 * \file
 *
 * Authors:
 *   Dietmar Maurer (dietmar@ximian.com)
 *   Paolo Molaro (lupus@ximian.com)
 *	 Patrik Torstensson (patrik.torstensson@labs2.com)
 *   Marek Safar (marek.safar@gmail.com)
 *   Aleksey Kliger (aleksey@xamarin.com)
 *
 * Copyright 2001-2003 Ximian, Inc (http://www.ximian.com)
 * Copyright 2004-2009 Novell, Inc (http://www.novell.com)
 * Copyright 2011-2015 Xamarin Inc (http://www.xamarin.com).
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>
#include <glib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined (HAVE_WCHAR_H)
#include <wchar.h>
#endif

#include <mono/utils/mono-publib.h>
#include <mono/utils/bsearch.h>
#include <mono/metadata/icalls.h>
#include "handle-decl.h"
#include "icall-decl.h"

// These definitions are used for multiple includes of icall-def.h and eventually undefined.
#define NOHANDLES(inner) inner
#define HANDLES_MAYBE(cond, id, name, func, ret, nargs, argtypes) HANDLES (id, name, func, ret, nargs, argtypes)
#define HANDLES(id, name, func, ...)	ICALL (id, name, func ## _raw)
#define HANDLES_REUSE_WRAPPER		HANDLES

#ifdef ENABLE_ICALL_SYMBOL_MAP
#define IF_ENABLE_ICALL_SYMBOL_MAP(x) x
#else
#define IF_ENABLE_ICALL_SYMBOL_MAP(x) /* nothing */
#endif

#define MONO_ARRAY_AND_COUNT(x) x, G_N_ELEMENTS (x)

typedef struct MonoIcallTableMethod {
	const char *managed_name;
	IF_ENABLE_ICALL_SYMBOL_MAP (const char *native_name;)
	gpointer native_function_pointer;
	gboolean uses_handles;
} MonoIcallTableMethod;

typedef struct MonoIcallTableClass {
	const char *class_name;
	const MonoIcallTableMethod *methods;
	gsize method_count;
} MonoIcallTableClass;

#define ICALL(ignored, managed, native) 	ICALL_COMMON (managed, native, FALSE)
#define HANDLES(ignored, managed, native, ...)	ICALL_COMMON (managed, native ## _raw, TRUE)

#define ICALL_TYPE(cname, class, ignored) 	    }; static const MonoIcallTableMethod mono_icall_table_methods_ ## cname [ ] = {
#define ICALL_COMMON(managed, native, uses_handles) { MONO_CONSTANT_STRING_AND_LENGTH (managed), IF_ENABLE_ICALL_SYMBOL_MAP (#native,) (gpointer)native, uses_handles },
char mono_icall_table_dummy [ ] = {
#include "icall-def.h"
#undef ICALL_TYPE
#undef ICALL_COMMON

#define ICALL_COMMON(...) /* nothing */
static const MonoIcallTableClass mono_icall_table_classes [ ] = {
#define ICALL_TYPE(cname, class, ignored) { class, MONO_ARRAY_AND_COUNT (mono_icall_table_methods_ ## cname) },
#include "icall-def.h"
};
#undef ICALL_TYPE
#undef ICALL_COMMON
#undef HANDLES
#undef HANDLES_MAYBE
#undef HANDLES_REUSE_WRAPPER
#undef NOHANDLES

#ifdef ENABLE_ICALL_SYMBOL_MAP

#define ICALL_TYPE(cname, class, ignored) 	    /* nothing */
#define ICALL_COMMON(managed, native, uses_handles) + 1
static const gsize mono_icall_table_function_count = 0
#include "icall-def.h"
;
#undef ICALL_TYPE
#undef ICALL_COMMON

static MonoIcallTableMethod *mono_icall_functions_sorted [mono_icall_table_function_count];

#endif

static int
mono_icall_table_compare_method_by_name (const void *key, const void *elem)
{
	return strcmp ((const char*)key, ((MonoIcallTableMethod*)elem)->managed_name);
}

static int
mono_icall_table_compare_method_indirect_by_native_functtion (const void *a, const void *b)
{
	char *c = *(char**)&a;
	char *d = *(char**)&b;
	return (c < d) ? -1 : (c > d) ? 1 : 0;
}

static int
mono_icall_table_compare_class_by_name (const void *key, const void *elem)
{
	return strcmp ((const char*)key, ((MonoIcallTableClass*)elem)->class_name);
}

static gpointer
icall_table_lookup (char *classname, char *methodname, char *sigstart, gboolean *uses_handles)
{
	MonoIcallTableMethod *method;
	MonoIcallTableClass *klass;

	if (uses_handles)
		*uses_handles = FALSE;

	klass = (MonoIcallTableClass*)mono_binary_search (classname, mono_icall_table_classes,
		  G_N_ELEMENTS (mono_icall_table_classes), sizeof (mono_icall_table_classes [0]), mono_icall_table_compare_class_by_name);
	if (!klass)
		return NULL;
retry:
	method = (MonoIcallTableMethod*)mono_binary_search (methodname, klass->methods,
		klass->method_count, sizeof (klass->methods [0]), mono_icall_table_compare_method_by_name);
	if (method) {
		if (uses_handles)
			*uses_handles = method->uses_handles;
		return method->native_function_pointer;
	}
	/* try _with_ signature */
	if (sigstart) {
		*sigstart = '(';
		sigstart = NULL;
		goto retry;
	}
	return NULL;
}

#ifdef ENABLE_ICALL_SYMBOL_MAP
static int
func_cmp (gconstpointer key, gconstpointer p)
{
	return (gsize)key - (gsize)*(gsize*)p;
}
#endif

static const char*
lookup_icall_symbol (gpointer func)
{
#ifdef ENABLE_ICALL_SYMBOL_MAP
	int i;
	gpointer slot;
	static int k;

	if (!k) {

		for (int i = 0; i < G_N_ELEMENTS (mono_icall_table_classes); ++i) {
			for (int j = 0; j < mono_icall_table_classes [i].method_count; ++j) {
				mono_icall_functions_sorted [k++] = &mono_icall_table_classes [i].methods [j];
			}
		}

		qsort (mono_icall_functions_sorted, G_N_ELEMENTS (mono_icall_functions_sorted), sizeof (mono_icall_functions_sorted [0], compare);
	}

	slot = mono_binary_search (func, mono_icall_functions_sorted, G_N_ELEMENTS (mono_icall_functions_sorted), sizeof (gpointer), func_cmp);
	if (!slot)
		return NULL;
	g_assert (slot);
	return symbols_sorted [(gpointer*)slot - (gpointer*)functions_sorted];
#else
	fprintf (stderr, "icall symbol maps not enabled, pass --enable-icall-symbol-map to configure.\n");
	g_assert_not_reached ();
	return NULL;
#endif
}

void
mono_icall_table_init (void)
{
	int i = 0;

	/* check that tables are sorted: disable in release */
	if (TRUE) {
		int j;
		const char *prev_class = NULL;
		const char *prev_method;
		
		for (i = 0; i < Icall_type_num; ++i) {
			const IcallTypeDesc *desc;
			int num_icalls;
			prev_method = NULL;
			if (prev_class && strcmp (prev_class, icall_type_name_get (i)) >= 0)
				g_print ("class %s should come before class %s\n", icall_type_name_get (i), prev_class);
			prev_class = icall_type_name_get (i);
			desc = &icall_type_descs [i];
			num_icalls = icall_desc_num_icalls (desc);
			/*g_print ("class %s has %d icalls starting at %d\n", prev_class, num_icalls, desc->first_icall);*/
			for (j = 0; j < num_icalls; ++j) {
				const char *methodn = icall_name_get (desc->first_icall + j);
				if (prev_method && strcmp (prev_method, methodn) >= 0)
					g_print ("method %s should come before method %s\n", methodn, prev_method);
				prev_method = methodn;
			}
		}
	}

	MonoIcallTableCallbacks cb;
	memset (&cb, 0, sizeof (MonoIcallTableCallbacks));
	cb.version = MONO_ICALL_TABLE_CALLBACKS_VERSION;
	cb.lookup = icall_table_lookup;
	cb.lookup_icall_symbol = lookup_icall_symbol;

	mono_install_icall_table_callbacks (&cb);
}
