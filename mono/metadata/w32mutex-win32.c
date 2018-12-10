/**
 * \file
 * Runtime support for managed Mutex on Win32
 *
 * Author:
 *	Ludovic Henry (luhenry@microsoft.com)
 *
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include "w32mutex.h"

#include <windows.h>
#include <winbase.h>
#include <mono/metadata/handle.h>
#include <mono/utils/mono-error-internals.h>


void
mono_w32mutex_init (void)
{
}

gpointer
ves_icall_System_Threading_Mutex_CreateMutex_internal (MonoBoolean owned, const gunichar2 *name,
	gint32 name_length, MonoBoolean *created, MonoError *error)
{
	// FIXME check for embedded nuls in name

	HANDLE mutex;

	*created = TRUE;

	/* Need to blow away any old errors here, because code tests
	 * for ERROR_ALREADY_EXISTS on success (!) to see if a mutex
	 * was freshly created */
	SetLastError (ERROR_SUCCESS);

	MONO_ENTER_GC_SAFE;
	mutex = CreateMutexW (NULL, owned, name);
	if (name && GetLastError () == ERROR_ALREADY_EXISTS)
		*created = FALSE;
	MONO_EXIT_GC_SAFE;

	return mutex;
}

MonoBoolean
ves_icall_System_Threading_Mutex_ReleaseMutex_internal (gpointer handle)
{
	return ReleaseMutex (handle);
}

gpointer
ves_icall_System_Threading_Mutex_OpenMutex_internal (MonoStringHandle name, gint32 rights, gint32 *err, MonoError *error)
{
	// FIXME check for embedded nuls in name

	HANDLE ret;

	*err = ERROR_SUCCESS;
	MONO_ENTER_GC_SAFE;
	ret = OpenMutexW (rights, FALSE, name);
	if (!ret)
		*err = GetLastError ();
	MONO_EXIT_GC_SAFE;

	return ret;
}
