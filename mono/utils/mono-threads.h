/**
 * \file
 * Low-level threading
 *
 * Author:
 *	Rodrigo Kumpera (kumpera@gmail.com)
 *
 * (C) 2011 Novell, Inc
 */

#ifndef __MONO_THREADS_H__
#define __MONO_THREADS_H__

#include <mono/utils/mono-os-semaphore.h>
#include <mono/utils/mono-stack-unwinding.h>
#include <mono/utils/mono-linked-list-set.h>
#include <mono/utils/lock-free-alloc.h>
#include <mono/utils/lock-free-queue.h>
#include <mono/utils/mono-tls.h>
#include <mono/utils/mono-coop-semaphore.h>
#include <mono/utils/os-event.h>
#include <mono/utils/refcount.h>

#include <glib.h>
#include <config.h>
#ifdef HOST_WIN32

#include <windows.h>

typedef DWORD MonoNativeThreadId;
typedef HANDLE MonoNativeThreadHandle; /* unused */

typedef DWORD mono_native_thread_return_t;
typedef DWORD mono_thread_start_return_t;

#define MONO_NATIVE_THREAD_ID_TO_UINT(tid) (tid)
#define MONO_UINT_TO_NATIVE_THREAD_ID(tid) ((MonoNativeThreadId)(tid))

typedef LPTHREAD_START_ROUTINE MonoThreadStart;

#else

#include <pthread.h>

#if defined(__MACH__)
#include <mono/utils/mach-support.h>

typedef thread_port_t MonoNativeThreadHandle;

#else

#include <unistd.h>

typedef pid_t MonoNativeThreadHandle;

#endif /* defined(__MACH__) */

typedef pthread_t MonoNativeThreadId;

typedef void* mono_native_thread_return_t;
typedef gsize mono_thread_start_return_t;

#define MONO_NATIVE_THREAD_ID_TO_UINT(tid) (gsize)(tid)
#define MONO_UINT_TO_NATIVE_THREAD_ID(tid) (MonoNativeThreadId)(gsize)(tid)

typedef gsize (*MonoThreadStart)(gpointer);

#if !defined(__HAIKU__)
#define MONO_THREADS_PLATFORM_HAS_ATTR_SETSCHED
#endif /* !defined(__HAIKU__) */

#endif /* #ifdef HOST_WIN32 */

#ifndef MONO_INFINITE_WAIT
#define MONO_INFINITE_WAIT ((guint32) 0xFFFFFFFF)
#endif

typedef struct {
	MonoRefCount ref;
	MonoOSEvent event;
} MonoThreadHandle;

/*
THREAD_INFO_TYPE is a way to make the mono-threads module parametric - or sort of.
The GC using mono-threads might extend the MonoThreadInfo struct to add its own
data, this avoid a pointer indirection on what is on a lot of hot paths.

But extending MonoThreadInfo has the disavantage that all functions here return type
would require a cast, something like the following:

typedef struct {
	MonoThreadInfo info;
	int stuff;
}  MyThreadInfo;

...
((MyThreadInfo*)mono_thread_info_current ())->stuff = 1;

While porting sgen to use mono-threads, the number of casts required was too much and
code ended up looking horrible. So we use this cute little hack. The idea is that
whomever is including this header can set the expected type to be used by functions here
and reduce the number of casts drastically.

Older version of this ran afoul of C++ typesafe linkage.
The declarations did not match the implementation.
That is fixed by having the "real" declarations and implementations
always use MonoThreadInfo* and then having wrapper macros cast.

One could also use wrapper static inlines, but that would require
a rename of the functions and for example, breakpoints would no longer work.

With C++ other workarounds will be viable.
  - The wrappers can be same-named overloads -- still maybe breaking breakpoints.
  - Inputs can be just derived classes.
*/
/*
This technique breaks with C++ name mangling.

Another technique that is compatible with that is e.g.

1. inputs

mono-threads.h:

static inline void* mono_thread_info_typecheck (THREAD_INFO_TYPE *info) { return info; }

void mono_threads_in (void *info);

// Macros do not recurse.
#define mono_threads_in(info) \
	(mono_threads_in (mono_thread_info_typecheck (info)))

mono-threads.c:

void (mono_threads_in) (void *void_info) // parens avoid macro
{
	THREAD_INFO_TYPE *info = (THREAD_INFO_TYPE*)void_info;
	...
}

2. outputs

void*
mono_threads_out (void);

// Macros do not recurse.
#define mono_threads_out() \
	((THREAD_INFO_TYPE*)mono_threads_out ())

For now we use extern "C" instead.
*/
#ifndef THREAD_INFO_TYPE
#define THREAD_INFO_TYPE MonoThreadInfo
#else
#define MONO_THREADS_NEED_THREAD_INFO_TYPE_WRAPPERS
#endif

/* Mono Threads internal configuration knows*/

/* If this is defined, use the signals backed on Mach. Debug only as signals can't be made usable on OSX. */
// #define USE_SIGNALS_ON_MACH

#ifdef HOST_WASM
#define USE_WASM_BACKEND
#elif defined (_POSIX_VERSION)
#if defined (__MACH__) && !defined (USE_SIGNALS_ON_MACH)
#define USE_MACH_BACKEND
#else
#define USE_POSIX_BACKEND
#endif
#elif HOST_WIN32
#define USE_WINDOWS_BACKEND
#else
#error "no backend support for current platform"
#endif /* defined (_POSIX_VERSION) */

enum {
	STATE_STARTING				= 0x00,
	STATE_DETACHED				= 0x01,

	STATE_RUNNING				= 0x02,
	STATE_ASYNC_SUSPENDED			= 0x03,
	STATE_SELF_SUSPENDED			= 0x04,
	STATE_ASYNC_SUSPEND_REQUESTED		= 0x05,

	STATE_BLOCKING				= 0x06,
	STATE_BLOCKING_ASYNC_SUSPENDED 		= 0x07,
	/* FIXME: All the transitions from STATE_SELF_SUSPENDED and
	 * STATE_BLOCKING_SELF_SUSPENDED are the same - they should be the same
	 * state. */
	STATE_BLOCKING_SELF_SUSPENDED		= 0x08,
	STATE_BLOCKING_SUSPEND_REQUESTED	= 0x09,

	STATE_MAX				= 0x09,

	THREAD_STATE_MASK			= 0x00FF,
	THREAD_SUSPEND_COUNT_MASK	= 0xFF00,
	THREAD_SUSPEND_COUNT_SHIFT	= 8,
	THREAD_SUSPEND_COUNT_MAX	= 0xFF,

	SELF_SUSPEND_STATE_INDEX = 0,
	ASYNC_SUSPEND_STATE_INDEX = 1,
};

/*
 * These flags control how the rest of the runtime will see and interact with
 * a thread.
 */
typedef enum {
	/*
	 * No flags means it's a normal thread that takes part in all runtime
	 * functionality.
	 */
	MONO_THREAD_INFO_FLAGS_NONE = 0,
	/*
	 * The thread will not be suspended by the STW machinery. The thread is not
	 * allowed to allocate or access managed memory at all, nor execute managed
	 * code.
	 */
	MONO_THREAD_INFO_FLAGS_NO_GC = 1,
	/*
	 * The thread will not be subject to profiler sampling signals.
	 */
	MONO_THREAD_INFO_FLAGS_NO_SAMPLE = 2,
} MonoThreadInfoFlags;

G_ENUM_FUNCTIONS (MonoThreadInfoFlags)

struct HandleStack;
struct MonoJitTlsData;
typedef struct _MonoThreadInfoInterruptToken MonoThreadInfoInterruptToken;

typedef struct _MonoThreadInfo {
	MonoLinkedListSetNode node;
	guint32 small_id; /*Used by hazard pointers */
	MonoNativeThreadHandle native_handle; /* Valid on mach, android and Windows */
	int thread_state;

	/*
	 * Must not be changed directly, and especially not by other threads. Use
	 * mono_thread_info_get/set_flags () to manipulate this.
	 */
	volatile gint32 flags;

	/*Tells if this thread was created by the runtime or not.*/
	gboolean runtime_thread;

	/* Max stack bounds, all valid addresses must be between [stack_start_limit, stack_end[ */
	void *stack_start_limit, *stack_end;

	/* suspend machinery, fields protected by suspend_semaphore */
	MonoSemType suspend_semaphore;
	int suspend_count;

	MonoSemType resume_semaphore;

	/* only needed by the posix backend */
#if defined(USE_POSIX_BACKEND)
	MonoSemType finish_resume_semaphore;
	gboolean syscall_break_signal;
	int signal;
#endif

	gboolean suspend_can_continue;

	/* This memory pool is used by coop GC to save stack data roots between GC unsafe regions */
	GByteArray *stackdata;

	/*In theory, only the posix backend needs this, but having it on mach/win32 simplifies things a lot.*/
	MonoThreadUnwindState thread_saved_state [2]; //0 is self suspend, 1 is async suspend.

	/*async call machinery, thread MUST be suspended before accessing those fields*/
	void (*async_target)(void*);
	void *user_data;

	/*
	If true, this thread is running a critical region of code and cannot be suspended.
	A critical session is implicitly started when you call mono_thread_info_safe_suspend_sync
	and is ended when you call either mono_thread_info_resume or mono_thread_info_finish_suspend.
	*/
	gboolean inside_critical_region;

	/*
	 * If TRUE, the thread is in async context. Code can use this information to avoid async-unsafe
	 * operations like locking without having to pass an 'async' parameter around.
	 */
	gboolean is_async_context;

	/*
	 * Values of TLS variables for this thread.
	 * This can be used to obtain the values of TLS variable for threads
	 * other than the current one.
	 */
	gpointer tls [TLS_KEY_NUM];

	/* IO layer handle for this thread */
	/* Set when the thread is started, or in _wapi_thread_duplicate () */
	MonoThreadHandle *handle;

	struct MonoJitTlsData *jit_data;

	MonoThreadInfoInterruptToken *interrupt_token;

	/* HandleStack for coop handles */
	struct HandleStack *handle_stack;

	/* Stack mark for targets that explicitly require one */
	gpointer stack_mark;

	/* GCHandle to MonoInternalThread */
	guint32 internal_thread_gchandle;

	/*
	 * Used by the sampling code in mini-posix.c to ensure that a thread has
	 * handled a sampling signal before sending another one.
	 */
	gint32 profiler_signal_ack;

#ifdef USE_WINDOWS_BACKEND
	gint32 thread_wait_info;
#endif

	/*
	 * This is where we store tools tls data so it follows our lifecycle and doesn't depends on posix tls cleanup ordering
	 *
	 * TODO support multiple values by multiple tools
	 */
	void *tools_data;
} MonoThreadInfo;

typedef struct {
	void* (*thread_attach)(THREAD_INFO_TYPE *info);
	/*
	This callback is called right before thread_detach_with_lock. This is called
	without any locks held so it's the place for complicated cleanup.

	The thread must remain operational between this call and thread_detach_with_lock.
	It must be possible to successfully suspend it after thread_detach completes.
	*/
	void (*thread_detach)(THREAD_INFO_TYPE *info);
	/*
	This callback is called with @info still on the thread list.
	This call is made while holding the suspend lock, so don't do callbacks.
	SMR remains functional as its small_id has not been reclaimed.
	*/
	void (*thread_detach_with_lock)(THREAD_INFO_TYPE *info);
	gboolean (*ip_in_critical_region) (MonoDomain *domain, gpointer ip);
	gboolean (*thread_in_critical_region) (THREAD_INFO_TYPE *info);

	// Called on the affected thread.
	void (*thread_flags_changing) (MonoThreadInfoFlags old, MonoThreadInfoFlags new_);
	void (*thread_flags_changed) (MonoThreadInfoFlags old, MonoThreadInfoFlags new_);
} MonoThreadInfoCallbacks;

typedef struct {
	void (*setup_async_callback) (MonoContext *ctx, void (*async_cb)(void *fun), gpointer user_data);
	gboolean (*thread_state_init_from_sigctx) (MonoThreadUnwindState *state, void *sigctx);
	gboolean (*thread_state_init_from_handle) (MonoThreadUnwindState *tctx, MonoThreadInfo *info, /*optional*/ void *sigctx);
	void (*thread_state_init) (MonoThreadUnwindState *tctx);
} MonoThreadInfoRuntimeCallbacks;

//Not using 0 and 1 to ensure callbacks are not returning bad data
typedef enum {
	MonoResumeThread = 0x1234,
	KeepSuspended = 0x4321,
} SuspendThreadResult;

typedef SuspendThreadResult (*MonoSuspendThreadCallback) (THREAD_INFO_TYPE *info, gpointer user_data);

MONO_API MonoThreadInfoFlags
mono_thread_info_get_flags (MonoThreadInfo *info);

/*
 * Sets the thread info flags for the current thread. This function may invoke
 * callbacks containing arbitrary code (e.g. locks) so it must be assumed to be
 * async unsafe.
 */
MONO_API void
mono_thread_info_set_flags (MonoThreadInfoFlags flags);

static inline gboolean
mono_threads_filter_exclude_flags (MonoThreadInfo *info, MonoThreadInfoFlags flags)
{
	return !(mono_thread_info_get_flags (info) & flags);
}

/* Normal iteration; requires the world to be stopped. */

#define FOREACH_THREAD_ALL(thread) \
	MONO_LLS_FOREACH_FILTERED (mono_thread_info_list_head (), THREAD_INFO_TYPE, thread, mono_lls_filter_accept_all, NULL)

#define FOREACH_THREAD_EXCLUDE(thread, not_flags) \
	MONO_LLS_FOREACH_FILTERED (mono_thread_info_list_head (), THREAD_INFO_TYPE, thread, mono_threads_filter_exclude_flags, not_flags)

#define FOREACH_THREAD_END \
	MONO_LLS_FOREACH_END

/* Snapshot iteration; can be done anytime but is slower. */

#define FOREACH_THREAD_SAFE_ALL(thread) \
	MONO_LLS_FOREACH_FILTERED_SAFE (mono_thread_info_list_head (), THREAD_INFO_TYPE, thread, mono_lls_filter_accept_all, NULL)

#define FOREACH_THREAD_SAFE_EXCLUDE(thread, not_flags) \
	MONO_LLS_FOREACH_FILTERED_SAFE (mono_thread_info_list_head (), THREAD_INFO_TYPE, thread, mono_threads_filter_exclude_flags, not_flags)

#define FOREACH_THREAD_SAFE_END \
	MONO_LLS_FOREACH_SAFE_END

static inline MonoNativeThreadId
mono_thread_info_get_tid (THREAD_INFO_TYPE *info)
{
	return MONO_UINT_TO_NATIVE_THREAD_ID (((MonoThreadInfo*) info)->node.key);
}

static inline void
mono_thread_info_set_tid (THREAD_INFO_TYPE *info, MonoNativeThreadId tid)
{
	((MonoThreadInfo*) info)->node.key = (uintptr_t) MONO_NATIVE_THREAD_ID_TO_UINT (tid);
}

/*
 * @thread_info_size is sizeof (GcThreadInfo), a struct the GC defines to make it possible to have
 * a single block with info from both camps. 
 */
void
mono_thread_info_init (size_t thread_info_size);

/*
 * Wait for the above mono_thread_info_init to be called
 */
void
mono_thread_info_wait_inited (void);

void
mono_thread_info_callbacks_init (MonoThreadInfoCallbacks *callbacks);

void
mono_thread_info_signals_init (void);

void
mono_thread_info_runtime_init (MonoThreadInfoRuntimeCallbacks *callbacks);

MonoThreadInfoRuntimeCallbacks *
mono_threads_get_runtime_callbacks (void);

MONO_API int
mono_thread_info_register_small_id (void);

MONO_API MonoThreadInfo *
mono_thread_info_attach (void);

MONO_API void
mono_thread_info_detach (void);

gboolean
mono_thread_info_try_get_internal_thread_gchandle (MonoThreadInfo *info, guint32 *gchandle);

void
mono_thread_info_set_internal_thread_gchandle (MonoThreadInfo *info, guint32 gchandle);

void
mono_thread_info_unset_internal_thread_gchandle (MonoThreadInfo *info);

gboolean
mono_thread_info_is_exiting (void);

MonoThreadInfo *
mono_thread_info_current (void);

MONO_API gboolean
mono_thread_info_set_tools_data (void *data);

MONO_API void*
mono_thread_info_get_tools_data (void);

MonoThreadInfo*
mono_thread_info_current_unchecked (void);

MONO_API int
mono_thread_info_get_small_id (void);

MonoLinkedListSet*
mono_thread_info_list_head (void);

MonoThreadInfo*
mono_thread_info_lookup (MonoNativeThreadId id);

gboolean
mono_thread_info_resume (MonoNativeThreadId tid);

void
mono_thread_info_safe_suspend_and_run (MonoNativeThreadId id, gboolean interrupt_kernel, MonoSuspendThreadCallback callback, gpointer user_data);

void
mono_thread_info_setup_async_call (MonoThreadInfo *info, void (*target_func)(void*), void *user_data);

void
mono_thread_info_suspend_lock (void);

void
mono_thread_info_suspend_unlock (void);

void
mono_thread_info_abort_socket_syscall_for_close (MonoNativeThreadId tid);

void
mono_thread_info_set_is_async_context (gboolean async_context);

gboolean
mono_thread_info_is_async_context (void);

void
mono_thread_info_get_stack_bounds (guint8 **staddr, size_t *stsize);

MONO_API gboolean
mono_thread_info_yield (void);

gint
mono_thread_info_sleep (guint32 ms, gboolean *alerted);

gint
mono_thread_info_usleep (guint64 us);

gpointer
mono_thread_info_tls_get (MonoThreadInfo *info, MonoTlsKey key);

void
mono_thread_info_tls_set (MonoThreadInfo *info, MonoTlsKey key, gpointer value);

void
mono_thread_info_exit (gsize exit_code);

MONO_PAL_API void
mono_thread_info_install_interrupt (void (*callback) (gpointer data), gpointer data, gboolean *interrupted);

MONO_PAL_API void
mono_thread_info_uninstall_interrupt (gboolean *interrupted);

MonoThreadInfoInterruptToken*
mono_thread_info_prepare_interrupt (MonoThreadInfo *info);

void
mono_thread_info_finish_interrupt (MonoThreadInfoInterruptToken *token);

void
mono_thread_info_self_interrupt (void);

void
mono_thread_info_clear_self_interrupt (void);

gboolean
mono_thread_info_is_interrupt_state (MonoThreadInfo *info);

void
mono_thread_info_describe_interrupt_token (MonoThreadInfo *info, GString *text);

gboolean
mono_thread_info_is_live (MonoThreadInfo *info);

int
mono_thread_info_get_system_max_stack_size (void);

MonoThreadHandle*
mono_threads_open_thread_handle (MonoThreadHandle *handle);

void
mono_threads_close_thread_handle (MonoThreadHandle *handle);

#if !defined(HOST_WIN32)

/*Use this instead of pthread_kill */
int
mono_threads_pthread_kill (MonoThreadInfo *info, int signum);

#endif /* !defined(HOST_WIN32) */

/* Internal API between mono-threads and its backends. */

/* Backend functions - a backend must implement all of the following */
/*
This is called very early in the runtime, it cannot access any runtime facilities.

*/
void mono_threads_suspend_init (void); //ok

void mono_threads_suspend_init_signals (void);

void mono_threads_coop_init (void);

/*
This begins async suspend. This function must do the following:

-Ensure the target will EINTR any syscalls if @interrupt_kernel is true
-Call mono_threads_transition_finish_async_suspend as part of its async suspend.
-Register the thread for pending suspend with mono_threads_add_to_pending_operation_set if needed.

If begin suspend fails the thread must be left uninterrupted and resumed.
*/
gboolean mono_threads_suspend_begin_async_suspend (MonoThreadInfo *info, gboolean interrupt_kernel);

/*
This verifies the outcome of an async suspend operation.

Some targets, such as posix, verify suspend results assynchronously. Suspend results must be
available (in a non blocking way) after mono_threads_wait_pending_operations completes.
*/
gboolean mono_threads_suspend_check_suspend_result (MonoThreadInfo *info);

/*
This begins async resume. This function must do the following:

- Install an async target if one was requested.
- Notify the target to resume.
- Register the thread for pending ack with mono_threads_add_to_pending_operation_set if needed.
*/
gboolean mono_threads_suspend_begin_async_resume (MonoThreadInfo *info);

void mono_threads_suspend_register (MonoThreadInfo *info); //ok

void mono_threads_suspend_free (MonoThreadInfo *info);

void mono_threads_suspend_abort_syscall (MonoThreadInfo *info);

gint mono_threads_suspend_search_alternative_signal (void);
gint mono_threads_suspend_get_suspend_signal (void);
gint mono_threads_suspend_get_restart_signal (void);
gint mono_threads_suspend_get_abort_signal (void);

gboolean
mono_thread_platform_create_thread (MonoThreadStart thread_fn, gpointer thread_data,
	gsize* const stack_size, MonoNativeThreadId *tid);

void mono_threads_platform_get_stack_bounds (guint8 **staddr, size_t *stsize);
void mono_threads_platform_init (void);
gboolean mono_threads_platform_in_critical_region (MonoNativeThreadId tid);
gboolean mono_threads_platform_yield (void);
void mono_threads_platform_exit (gsize exit_code);

void mono_threads_coop_begin_global_suspend (void);
void mono_threads_coop_end_global_suspend (void);

MONO_API MonoNativeThreadId
mono_native_thread_id_get (void);

MONO_API gboolean
mono_native_thread_id_equals (MonoNativeThreadId id1, MonoNativeThreadId id2);

//FIXMEcxx typedef mono_native_thread_return_t (MONO_STDCALL * MonoNativeThreadStart)(void*);

MONO_API gboolean
mono_native_thread_create (MonoNativeThreadId *tid, gpointer /*FIXMEcxx MonoNativeThreadStart*/ func, gpointer arg);

#ifdef __cplusplus
// Macros do not recurse.
#define mono_native_thread_create(tid, func, arg) (mono_native_thread_create ((tid), (gpointer)(func), (arg)))
#endif

MONO_API void
mono_native_thread_set_name (MonoNativeThreadId tid, const char *name);

MONO_API gboolean
mono_native_thread_join (MonoNativeThreadId tid);

/*Mach specific internals */
void mono_threads_init_dead_letter (void);
void mono_threads_install_dead_letter (void);

/* mono-threads internal API used by the backends. */
/*
This tells the suspend initiator that we completed suspend and will now be waiting for resume.
*/
void mono_threads_notify_initiator_of_suspend (MonoThreadInfo* info);

/*
This tells the resume initiator that we completed resume duties and will return to runnable state.
*/
void mono_threads_notify_initiator_of_resume (MonoThreadInfo* info);

/*
This tells the resume initiator that we completed abort duties and will return to previous state.
*/
void mono_threads_notify_initiator_of_abort (MonoThreadInfo* info);

/* Thread state machine functions */

typedef enum {
	ResumeError,
	ResumeOk,
	ResumeInitSelfResume,
	ResumeInitAsyncResume,
	ResumeInitBlockingResume,
} MonoResumeResult;

typedef enum {
	SelfSuspendResumed,
	SelfSuspendNotifyAndWait,
} MonoSelfSupendResult;

typedef enum {
	ReqSuspendAlreadySuspended,
	ReqSuspendAlreadySuspendedBlocking,
	ReqSuspendInitSuspendRunning,
	ReqSuspendInitSuspendBlocking,
} MonoRequestSuspendResult;

typedef enum {
	DoBlockingContinue, //in blocking mode, continue
	DoBlockingPollAndRetry, //async suspend raced blocking and won, pool and retry
} MonoDoBlockingResult;

typedef enum {
	DoneBlockingOk, //exited blocking fine
	DoneBlockingWait, //thread should end suspended and wait for resume
} MonoDoneBlockingResult;


typedef enum {
	AbortBlockingIgnore, //Ignore
	AbortBlockingIgnoreAndPoll, //Ignore and poll
	AbortBlockingOk, //Abort worked
	AbortBlockingWait, //Abort worked, but should wait for resume
} MonoAbortBlockingResult;

void mono_threads_transition_attach (MonoThreadInfo* info);

gboolean mono_threads_transition_detach (MonoThreadInfo *info);

MonoRequestSuspendResult mono_threads_transition_request_suspension (MonoThreadInfo *info);

MonoSelfSupendResult mono_threads_transition_state_poll (MonoThreadInfo *info);

MonoResumeResult mono_threads_transition_request_resume (MonoThreadInfo* info);

gboolean mono_threads_transition_finish_async_suspend (MonoThreadInfo* info);

MonoDoBlockingResult mono_threads_transition_do_blocking (MonoThreadInfo* info, const char* func);

MonoDoneBlockingResult mono_threads_transition_done_blocking (MonoThreadInfo* info, const char* func);

MonoAbortBlockingResult mono_threads_transition_abort_blocking (MonoThreadInfo* info, const char* func);

gboolean mono_threads_transition_peek_blocking_suspend_requested (MonoThreadInfo* info);

MonoThreadUnwindState* mono_thread_info_get_suspend_state (MonoThreadInfo *info);

gpointer
mono_threads_enter_gc_unsafe_region_cookie (void);

void mono_thread_info_wait_for_resume (MonoThreadInfo *info);

/* Advanced suspend API, used for suspending multiple threads as once. */
gboolean mono_thread_info_is_running (MonoThreadInfo *info);

gboolean mono_thread_info_is_live (MonoThreadInfo *info);

int mono_thread_info_suspend_count (MonoThreadInfo *info);

int mono_thread_info_current_state (MonoThreadInfo *info);

const char* mono_thread_state_name (int state);
gboolean mono_thread_is_gc_unsafe_mode (void);

/* Suspend phases:
 *
 * In a full coop or full preemptive suspend, there is only a single phase.  In
 * the initial phase, all threads are either cooperatively or preemptively
 * suspended, respectively.
 *
 * In hybrid suspend, there may be two phases.  In the initial phase, threads
 * are invited to cooperatively suspend.  Running threads are expected to
 * finish cooperatively suspending (the phase waits for them), but blocking
 * threads need not.
 *
 * If any blocking thread was encountered in the initial phase, a second
 * "mop-up" phase runs which checks whether the blocking threads self-suspended
 * (in which case nothing more needs to be done) or if they're still in the
 * BLOCKING_SUSPEND_REQUESTED state, in which case they are preemptively
 * suspended.
 */
#define MONO_THREAD_SUSPEND_PHASE_INITIAL (0)
#define MONO_THREAD_SUSPEND_PHASE_MOPUP (1)
// number of phases
#define MONO_THREAD_SUSPEND_PHASE_COUNT (2)
typedef int MonoThreadSuspendPhase;

typedef enum {
	MONO_THREAD_BEGIN_SUSPEND_SKIP = 0,
	MONO_THREAD_BEGIN_SUSPEND_SUSPENDED = 1,
	MONO_THREAD_BEGIN_SUSPEND_NEXT_PHASE = 2,
} MonoThreadBeginSuspendResult;

gboolean mono_thread_info_in_critical_location (MonoThreadInfo *info);

MonoThreadBeginSuspendResult mono_thread_info_begin_suspend (MonoThreadInfo *info, MonoThreadSuspendPhase phase);

gboolean mono_thread_info_begin_resume (MonoThreadInfo *info);

void mono_threads_add_to_pending_operation_set (MonoThreadInfo* info); //XXX rename to something to reflect the fact that this is used for both suspend and resume

gboolean mono_threads_wait_pending_operations (void);
void mono_threads_begin_global_suspend (void);
void mono_threads_end_global_suspend (void);

gboolean
mono_thread_info_is_current (MonoThreadInfo *info);

typedef enum {
	MONO_THREAD_INFO_WAIT_RET_SUCCESS_0   =  0,
	MONO_THREAD_INFO_WAIT_RET_ALERTED     = -1,
	MONO_THREAD_INFO_WAIT_RET_TIMEOUT     = -2,
	MONO_THREAD_INFO_WAIT_RET_FAILED      = -3,
} MonoThreadInfoWaitRet;

MonoThreadInfoWaitRet
mono_thread_info_wait_one_handle (MonoThreadHandle *handle, guint32 timeout, gboolean alertable);

MonoThreadInfoWaitRet
mono_thread_info_wait_multiple_handle (MonoThreadHandle **thread_handles, gsize nhandles, MonoOSEvent *background_change_event, gboolean waitall, guint32 timeout, gboolean alertable);

void mono_threads_join_lock (void);
void mono_threads_join_unlock (void);


#ifdef HOST_WASM
typedef void (*background_job_cb)(void);
void mono_threads_schedule_background_job (background_job_cb cb);
#endif

#ifdef MONO_THREADS_NEED_THREAD_INFO_TYPE_WRAPPERS
#undef MONO_THREADS_NEED_THREAD_INFO_TYPE_WRAPPERS

// This scheme for dealing with THREAD_INFO_TYPE differs
// from the old scheme, in that it has function prototypes match function implementations.
// It does break taking address of functions ("can't take address of macros").
// FIXME In C++ future, can replace this hack with derivation from MonoThreadInfo.
// That will require changing all the explicit accesses to the "named base".
// SgenThreadInfo * info;
// info->base.foo => info-foo, and can be only one foo between base and derived.
// This at least solves the input casts, which is most of the problem.

// Output casts.
#define mono_thread_info_attach(info)		((THREAD_INFO_TYPE*)mono_thread_info_attach ())
#define mono_thread_info_current()		((THREAD_INFO_TYPE*)mono_thread_info_current ())
#define mono_thread_info_current_unchecked()	((THREAD_INFO_TYPE*)mono_thread_info_current_unchecked ())
#define mono_thread_info_lookup(id) 		((THREAD_INFO_TYPE*)mono_thread_info_lookup (id))

// Input casts, only accepting the one type (not void*).

static inline MonoThreadInfo*
mono_thread_info (THREAD_INFO_TYPE *info)
{
	return (MonoThreadInfo*)info;
}

#define mono_thread_info_get_flags(info)				  (mono_thread_info_get_flags (mono_thread_info (info)))
#define mono_thread_info_try_get_internal_thread_gchandle(info, gchandle) (mono_thread_info_try_get_internal_thread_gchandle (mono_thread_info (info), (gchandle)))
#define mono_thread_info_set_internal_thread_gchandle(info, gchandle)	  (mono_thread_info_set_internal_thread_gchandle (mono_thread_info (info), (gchandle)))
#define mono_thread_info_unset_internal_thread_gchandle(info)		  (mono_thread_info_unset_internal_thread_gchandle (mono_thread_info (info)))

#define mono_thread_info_setup_async_call(info, target_func, user_data)   (mono_thread_info_setup_async_call (mono_thread_info (info), (target_func), (user_data)))
#define mono_thread_info_tls_set(info, key, value)			  (mono_thread_info_tls_set (mono_thread_info (info), (key), (value)))
#define mono_thread_info_prepare_interrupt(info)			  (mono_thread_info_prepare_interrupt (mono_thread_info (info)))
#define mono_thread_info_is_interrupt_state(info) 			  (mono_thread_info_is_interrupt_state (mono_thread_info (info)))

#define mono_thread_info_describe_interrupt_token(info, text)		  (mono_thread_info_describe_interrupt_token (mono_thread_info (info), (text)))
#define mono_thread_info_is_live(info)					  (mono_thread_info_is_live (mono_thread_info (info)))
#define mono_threads_pthread_kill(info, signum)				  (mono_threads_pthread_kill (mono_thread_info (info), (signum)))
#define mono_threads_suspend_begin_async_suspend(info, interrupt_kernel)  (mono_threads_suspend_begin_async_suspend (mono_thread_info (info), (interrupt_kernel)))

#define mono_threads_suspend_check_suspend_result(info)			  (mono_threads_suspend_check_suspend_result (mono_thread_info (info)))
#define mono_threads_suspend_begin_async_resume(info)			  (mono_threads_suspend_begin_async_resume (mono_thread_info (info)))
#define mono_threads_suspend_register(info)				  (mono_threads_suspend_register (mono_thread_info (info)))
#define mono_threads_suspend_free(info)					  (mono_threads_suspend_free (mono_thread_info (info)))

#define mono_threads_suspend_abort_syscall(info)			  (mono_threads_suspend_abort_syscall (mono_thread_info (info)))
#define mono_threads_notify_initiator_of_suspend(info)			  (mono_threads_notify_initiator_of_suspend (mono_thread_info (info)))
#define mono_threads_notify_initiator_of_resume(info)			  (mono_threads_notify_initiator_of_resume (mono_thread_info (info)))
#define mono_threads_notify_initiator_of_abort(info)			  (mono_threads_notify_initiator_of_abort (mono_thread_info (info)))

#define mono_threads_transition_attach(info)				  (mono_threads_transition_attach (mono_thread_info (info)))
#define mono_threads_transition_detach(info)				  (mono_threads_transition_detach (mono_thread_info (info)))
#define mono_threads_transition_request_suspension(info)		  (mono_threads_transition_request_suspension (mono_thread_info (info)))
#define mono_threads_transition_state_poll(info)			  (mono_threads_transition_state_poll (mono_thread_info (info)))

#define mono_threads_transition_request_resume(info)			  (mono_threads_transition_request_resume (mono_thread_info (info)))
#define mono_threads_transition_finish_async_suspend(info)		  (mono_threads_transition_finish_async_suspend (mono_thread_info (info)))
#define mono_threads_transition_do_blocking(info)			  (mono_threads_transition_do_blocking (mono_thread_info (info), (func)))
#define mono_threads_transition_done_blocking(info)			  (mono_threads_transition_done_blocking (mono_thread_info (info), (func)))

#define mono_threads_transition_abort_blocking(info)			  (mono_threads_transition_abort_blocking (mono_thread_info (info), (func)))
#define mono_threads_transition_peek_blocking_suspend_requested(info)	  (mono_threads_transition_peek_blocking_suspend_requested (mono_thread_info (info)))
#define mono_thread_info_get_suspend_state(info) 			  (mono_thread_info_get_suspend_state (mono_thread_info (info)))
#define mono_thread_info_wait_for_resume(info) 				  (mono_thread_info_wait_for_resume (mono_thread_info (info)))

#define mono_thread_info_is_running(info) 				  (mono_thread_info_is_running (mono_thread_info (info)))
#define mono_thread_info_is_live(info) 					  (mono_thread_info_is_live (mono_thread_info (info)))
#define mono_thread_info_suspend_count(info) 				  (mono_thread_info_suspend_count (mono_thread_info (info)))
#define mono_thread_info_current_state(info) 				  (mono_thread_info_current_state (mono_thread_info (info)))

#define mono_thread_info_in_critical_location(info) 			  (mono_thread_info_in_critical_location (mono_thread_info (info)))
#define mono_thread_info_begin_suspend(info, phase) 			  (mono_thread_info_begin_suspend (mono_thread_info (info), (phase)))
#define mono_thread_info_begin_resume(info) 				  (mono_thread_info_begin_resume (mono_thread_info (info)))
#define mono_thread_info_is_current(info) 				  (mono_thread_info_is_current (mono_thread_info (info)))

#define mono_threads_filter_exclude_flags(info, flags)			  (mono_threads_filter_exclude_flags (mono_thread_info (info), (flags)))

#else

#define mono_thread_info_tls_get(info, key) (g_cast (mono_thread_info_tls_get ((info), (key))))

#endif

#endif /* __MONO_THREADS_H__ */
