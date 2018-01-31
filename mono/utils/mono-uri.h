/**
 * \file
 */

#ifndef __MONO_URI_H
#define __MONO_URI_H
#include <glib.h>
#include <mono/utils/mono-publib.h>

#ifdef __cplusplus
extern "C" {
#endif

MONO_API gchar * mono_escape_uri_string (const gchar *string);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __MONO_URI_H */
