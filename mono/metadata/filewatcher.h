/**
 * \file
 * File System Watcher internal calls
 *
 * Authors:
 *	Gonzalo Paniagua Javier (gonzalo@ximian.com)
 *
 * (C) 2004 Novell, Inc. (http://www.novell.com)
 */

#ifndef _MONO_METADATA_FILEWATCHER_H
#define _MONO_METADATA_FILEWATCHER_H

#include <mono/metadata/object.h>
#include "mono/utils/mono-compiler.h"
#include <glib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif



gint ves_icall_System_IO_FSW_SupportsFSW (void);

gboolean ves_icall_System_IO_FAMW_InternalFAMNextEvent (gpointer conn,
							MonoString **filename,
							gint *code,
							gint *reqnum);

int ves_icall_System_IO_InotifyWatcher_GetInotifyInstance (void);
int ves_icall_System_IO_InotifyWatcher_AddWatch (int fd, MonoString *directory, gint32 mask);
int ves_icall_System_IO_InotifyWatcher_RemoveWatch (int fd, gint32 watch_descriptor);

int ves_icall_System_IO_KqueueMonitor_kevent_notimeout (int *kq, gpointer changelist, int nchanges, gpointer eventlist, int nevents);

#ifdef HOST_IOS // This will obsoleted by System.Native as soon as it's ported to iOS
MONO_API char* SystemNative_RealPath(const char* path);
MONO_API void SystemNative_Sync (void);
#endif



#endif

