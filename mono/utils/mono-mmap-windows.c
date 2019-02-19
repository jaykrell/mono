/**
 * \file
 * Windows support for mapping code into the process address space
 *
 * Author:
 *   Mono Team (mono-list@lists.ximian.com)
 *
 * Copyright 2001-2008 Novell, Inc.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>
#include <glib.h>

#if defined(HOST_WIN32)
#include <windows.h>
#include "mono/utils/mono-mmap-windows-internals.h"
#include <mono/utils/mono-counters.h>
#include <io.h>

static void *malloced_shared_area;

int
mono_pagesize (void)
{
	SYSTEM_INFO info;
	static int saved_pagesize = 0;
	if (saved_pagesize)
		return saved_pagesize;
	GetSystemInfo (&info);
	saved_pagesize = info.dwPageSize;
	return saved_pagesize;
}

int
mono_valloc_granule (void)
{
	SYSTEM_INFO info;
	static int saved_valloc_granule = 0;
	if (saved_valloc_granule)
		return saved_valloc_granule;
	GetSystemInfo (&info);
	saved_valloc_granule = info.dwAllocationGranularity;
	return saved_valloc_granule;
}

int
mono_mmap_win_prot_from_flags (int flags)
{
	int prot = flags & (MONO_MMAP_READ|MONO_MMAP_WRITE|MONO_MMAP_EXEC);
	switch (prot) {
	case 0: prot = PAGE_NOACCESS; break;
	case MONO_MMAP_READ: prot = PAGE_READONLY; break;
	case MONO_MMAP_READ|MONO_MMAP_EXEC: prot = PAGE_EXECUTE_READ; break;
	case MONO_MMAP_READ|MONO_MMAP_WRITE: prot = PAGE_READWRITE; break;
	case MONO_MMAP_READ|MONO_MMAP_WRITE|MONO_MMAP_EXEC: prot = PAGE_EXECUTE_READWRITE; break;
	case MONO_MMAP_WRITE: prot = PAGE_READWRITE; break;
	case MONO_MMAP_WRITE|MONO_MMAP_EXEC: prot = PAGE_EXECUTE_READWRITE; break;
	case MONO_MMAP_EXEC: prot = PAGE_EXECUTE; break;
	default:
		g_assert_not_reached ();
	}
	return prot;
}

void*
mono_valloc (void *addr, size_t length, int flags, MonoMemAccountType type)
{
	if (!mono_valloc_can_alloc (length))
		return NULL;

	void *ptr;
	int mflags = MEM_RESERVE|MEM_COMMIT;
	int prot = mono_mmap_win_prot_from_flags (flags);
	/* translate the flags */

	ptr = VirtualAlloc (addr, length, mflags, prot);

	mono_account_mem (type, (ssize_t)length);

	return ptr;
}

void*
mono_valloc_aligned (size_t length, size_t alignment, int flags, MonoMemAccountType type)
{
	int prot = mono_mmap_win_prot_from_flags (flags);
	char *mem = (char*)VirtualAlloc (NULL, length + alignment, MEM_RESERVE, prot);
	char *aligned;

	if (!mem)
		return NULL;

	if (!mono_valloc_can_alloc (length))
		return NULL;

	aligned = mono_aligned_address (mem, length, alignment);

	aligned = (char*)VirtualAlloc (aligned, length, MEM_COMMIT, prot);
	g_assert (aligned);

	mono_account_mem (type, (ssize_t)length);

	return aligned;
}

int
mono_vfree (void *addr, size_t length, MonoMemAccountType type)
{
	MEMORY_BASIC_INFORMATION mbi;
	SIZE_T query_result = VirtualQuery (addr, &mbi, sizeof (mbi));
	BOOL res;

	g_assert (query_result);

	res = VirtualFree (mbi.AllocationBase, 0, MEM_RELEASE);

	g_assert (res);

	mono_account_mem (type, -(ssize_t)length);

	return 0;
}

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) || G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT)

static void
remove_trailing_whitespace_utf16 (wchar_t *s)
{
	gsize length = wcslen (s);
	gsize const original_length = length;
	while (length > 0 && iswspace (s [length - 1]))
		--length;
	if (length != original_length)
		s [length] = 0;
}

void*
mono_file_map_error (size_t length, int flags, int fd, guint64 offset, void **ret_handle,
	const char *filepath, char **error_message)
{
	void *ptr = NULL;
	HANDLE mapping = NULL;
	int const prot = mono_mmap_win_prot_from_flags (flags);
	/* translate the flags */
	/*if (flags & MONO_MMAP_PRIVATE)
		mflags |= MAP_PRIVATE;
	if (flags & MONO_MMAP_SHARED)
		mflags |= MAP_SHARED;
	if (flags & MONO_MMAP_ANON)
		mflags |= MAP_ANONYMOUS;
	if (flags & MONO_MMAP_FIXED)
		mflags |= MAP_FIXED;
	if (flags & MONO_MMAP_32BIT)
		mflags |= MAP_32BIT;*/
	int const mflags = (flags & MONO_MMAP_WRITE) ? FILE_MAP_COPY : FILE_MAP_READ;
	HANDLE const file = (HANDLE)_get_osfhandle (fd);
	const char *failed_function = NULL;

	// The size of the mapping is the maximum file offset to map.
	// See https://docs.microsoft.com/en-us/windows/desktop/Memory/creating-a-file-mapping-object.
	const guint64 mapping_length = offset + length;

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)

	failed_function = "CreateFileMapping";
	mapping = CreateFileMappingW (file, NULL, prot, (DWORD)(mapping_length >> 32), (DWORD)mapping_length, NULL);
	if (mapping == NULL)
		goto exit;

	failed_function = "MapViewOfFile";
	ptr = MapViewOfFile (mapping, mflags, (DWORD)(offset >> 32), (DWORD)offset, length);
	if (ptr == NULL)
		goto exit;

#elif G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT)

	failed_function = "CreateFileMappingFromApp";
	mapping = CreateFileMappingFromApp (file, NULL, prot, mapping_length, NULL);
	if (mapping == NULL)
		goto exit;

	failed_function = "MapViewOfFileFromApp";
	ptr = MapViewOfFileFromApp (mapping, mflags, offset, length);
	if (ptr == NULL)
		goto exit;

#else
#error unknown Windows variant
#endif

exit:
	if (!ptr && (mapping || error_message)) {
		int const win32_error = GetLastError ();
		if (mapping)
			CloseHandle (mapping);
		if (error_message) {
			WCHAR win32_error_string [100] = { 0 }; // FIXME LocalFree not initially in UWP but it is now.
			FormatMessageW (FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
				0, win32_error, 0, win32_error_string, 99, NULL);
			// Win32 error messages end with an unsightly newline.
			remove_trailing_whitespace_utf16 (win32_error_string);
			*error_message = g_strdup_printf ("%s failed file:%s length:0x%IX offset:0x%I64X function:%s error:%ls(0x%X)\n",
				__func__, filepath ? filepath : "", length, offset, failed_function, win32_error_string, win32_error);
		}
		SetLastError (win32_error);
	}
	*ret_handle = mapping;
	return ptr;
}

void*
mono_file_map (size_t length, int flags, int fd, guint64 offset, void **ret_handle)
{
	return mono_file_map_error (length, flags, fd, offset, ret_handle, NULL, NULL);
}

int
mono_file_unmap (void *addr, void *handle)
{
	UnmapViewOfFile (addr);
	CloseHandle (handle);
	return 0;
}

#endif // G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) || G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT)

int
mono_mprotect (void *addr, size_t length, int flags)
{
	DWORD oldprot;
	int prot = mono_mmap_win_prot_from_flags (flags);

	if (flags & MONO_MMAP_DISCARD) {
		VirtualFree (addr, length, MEM_DECOMMIT);
		VirtualAlloc (addr, length, MEM_COMMIT, prot);
		return 0;
	}
	return VirtualProtect (addr, length, prot, &oldprot) == 0;
}

void*
mono_shared_area (void)
{
	if (!malloced_shared_area)
		malloced_shared_area = mono_malloc_shared_area (0);
	/* get the pid here */
	return malloced_shared_area;
}

void
mono_shared_area_remove (void)
{
	if (malloced_shared_area)
		g_free (malloced_shared_area);
	malloced_shared_area = NULL;
}

void*
mono_shared_area_for_pid (void *pid)
{
	return NULL;
}

void
mono_shared_area_unload (void *area)
{
}

int
mono_shared_area_instances (void **array, int count)
{
	return 0;
}

#endif // HOST_WIN32
