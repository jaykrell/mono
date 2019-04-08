/**
 * \file
 * Copyright 2019 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>
#include <glib.h>
#include "mono/metadata/exception-internals.h"
#include "register-icall-def.h"

// FIXME move to jit-icalls.c
MonoJitICallInfos mono_jit_icall_info = { 0 };

gconstpointer
mono_temporary_translate_jit_icall_info_name (gconstpointer data)
{
	if ((char*)data >= (char*)&mono_jit_icall_info && data < (char*)(1 + &mono_jit_icall_info))
		data = ((MonoJitICallInfo*)data)->name;
	return data;
}

gconstpointer
mono_temporary_translate_jit_icall_info_func (gconstpointer data)
{
	if ((char*)data >= (char*)&mono_jit_icall_info && data < (char*)(1 + &mono_jit_icall_info))
		data = ((MonoJitICallInfo*)data)->func;
	return data;
}


gconstpointer
mono_temporary_translate_jit_icall_info (gconstpointer data)
{
	return mono_temporary_translate_jit_icall_info_name (data);
}
