/**
 * \file
 */

#ifndef __UTILS_MONO_UTIL_H__
#define __UTILS_MONO_UTIL_H__

/*
In C this annotates a cast as casting away const.
In C++ this is enforced -- casts are deemed kind of bad
	when there are usually alternatives and merit annotation and restriction.

For example:
	const char* foo = g_strdup*("foo");
	...
	g_free(MONO_CONST_CAST(char*)(foo));
*/
#ifdef __cplusplus
#define MONO_CONST_CAST(t) const_cast<t>
#else
#define MONO_CONST_CAST(t) (t)
#endif

#endif /* __UTILS_MONO_UTIL_H__ */
