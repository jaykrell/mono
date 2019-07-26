// mono-windows-thread-name.c
//
// There several ways to set thread name on Windows.
//
// Historically: Raise an exception with an ASCII string.
// This is visible only in a debugger. Only if a debugger
// is running when the exception is raised. There is
// no way to get the thread name.
//
// Xbox360: XSetThreadName?
//
// XboxOne: SetThreadName(thread handle, unicode)
// This works with or without a debugger and can be retrieved with GetThreadName.
//
// Windows 10 1607 or newer:
//  SetThreadDescription(thread handle, unicode)
//  This is like XboxOne -- works with or without debugger, can be retrieved
//  with GetThreadDescription, and appears in ETW traces.
// See https://randomascii.wordpress.com/2015/10/26/thread-naming-in-windows-time-for-something-better/
//
// UWP: Also SetThreadDescription, but running prior to 1607
// would require LoadLibrary / GetProcAddress, which are not allowed.
//
// Author:
//  Jay Krell (jaykrell@microsoft.com)
//
// Copyright 2019 Microsoft
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
#include "config.h"
#include "mono-threads.h"

#ifdef HOST_WIN32

#if defined(_MSC_VER)
const DWORD MS_VC_EXCEPTION=0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
#endif

void
mono_native_thread_set_name (MonoNativeThreadId tid, const char *name)
{
#if defined(_MSC_VER)
	/* http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx */
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = tid;
	info.dwFlags = 0;

	__try {
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR),       (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}
#endif
}

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)

typedef
HRESULT
(__stdcall * SetThreadDescription_t) (HANDLE thread_handle, PCWSTR thread_name);

static
HRESULT
__stdcall
mono_windows_set_thread_name_fallback_nop (HANDLE thread_handle, PCWSTR thread_name)
{
	// This function is called on older systems, when LoadLibrary / GetProcAddress fail.
}

static
HRESULT
__stdcall
mono_windows_set_thread_name_init (HANDLE thread_handle, PCWSTR thread_name);

static SetThreadDescription_t set_thread_description = mono_windows_set_thread_name_init;

static
HRESULT
__stdcall
mono_windows_set_thread_name_init (HANDLE thread_handle, PCWSTR thread_name)
{
	// This function is called the first time mono_windows_thread_set_name is called
	// to LoadLibrary / GetProcAccess.
	//
	// Do not write NULL to global, that is racy.
	SetThreadDescription_t local = NULL;
	HMODULE kernel32 = LoadLibraryExW (L"kernel32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (kernel32)
		local = (SetThreadDescription_t)GetProcAddress (kernel32,
			"SetThreadDescription");
	if (!local)
		local = mono_windows_set_thread_name_fallback_nop;
	set_thread_description = local;
	local (thread_handle, thread_name);
}

void
mono_windows_thread_set_name (HANDLE thread_handle, const gunichar2* thread_name)
{
	set_thread_description (thread_hande, thread_name);
}

#else

void
mono_windows_thread_set_name (HANDLE thread_handle, const gunichar2* thread_name)
{
	// When Windows 10 1607 is the minimum supported, just call it directly.
	// If LoadLibrary / GetProcAddress are made available to UWP, use that.
	// SetThreadDescription (thread_hande, thread_name);
}

#endif
