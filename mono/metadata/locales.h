/**
 * \file
 * Culture-sensitive handling
 *
 * Authors:
 *	Dick Porter (dick@ximian.com)
 *
 * (C) 2003 Ximian, Inc.
 */

#ifndef _MONO_METADATA_LOCALES_H_
#define _MONO_METADATA_LOCALES_H_

#include <glib.h>

#include <mono/metadata/object-internals.h>
#include <mono/metadata/icalls.h>

/* This is a copy of System.Globalization.CompareOptions */
typedef enum {
	CompareOptions_None=0x00,
	CompareOptions_IgnoreCase=0x01,
	CompareOptions_IgnoreNonSpace=0x02,
	CompareOptions_IgnoreSymbols=0x04,
	CompareOptions_IgnoreKanaType=0x08,
	CompareOptions_IgnoreWidth=0x10,
	CompareOptions_StringSort=0x20000000,
	CompareOptions_Ordinal=0x40000000
} MonoCompareOptions;

#define mono_thread_current_lcid() ((int)0x007F/* Invariant */)

ICALL_EXPORT
void ves_icall_System_Text_Normalization_load_normalization_resource (guint8 **argProps, guint8** argMappedChars, guint8** argCharMapIndex, guint8** argHelperIndex, guint8** argMapIdxToComposite, guint8** argCombiningClass, MonoError *error);

#endif /* _MONO_METADATA_FILEIO_H_ */
