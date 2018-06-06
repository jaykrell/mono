/**
 * \file
 */

#include <config.h>
#include "mono/utils/mono-coop-semaphore.h"
#include "mono/utils/mono-threads.h"

#define ENABLE_MONO_LOG 1
#include <mono/utils/mono-log.h>

static void
mono_coop_sem_interrupt (gpointer sem)
{
	MONO_LOG ();
	mono_os_sem_post (&((MonoCoopSem*)sem)->s);
}

static const char*
timed_wait_string (int a)
{
	switch (a)
	{
	case MONO_SEM_TIMEDWAIT_RET_SUCCESS: return "MONO_SEM_TIMEDWAIT_RET_SUCCESS";
	case MONO_SEM_TIMEDWAIT_RET_ALERTED: return "MONO_SEM_TIMEDWAIT_RET_ALERTED";
	case MONO_SEM_TIMEDWAIT_RET_TIMEDOUT: return "MONO_SEM_TIMEDWAIT_RET_TIMEDOUT";
	}
	return "";
}

MonoSemTimedwaitRet
mono_coop_sem_timedwait (MonoCoopSem *sem, guint timeout_ms, MonoSemFlags flags)
{
	MonoSemTimedwaitRet res;
	gboolean alertable = (flags & MONO_SEM_FLAGS_ALERTABLE) != 0;
	gboolean interrupted = FALSE;
	gboolean implement_alertable = TRUE;
	implement_alertable = FALSE;

	MONO_LOG ("alertable:%d", alertable);

	if (alertable && implement_alertable) {
		mono_thread_info_install_interrupt (mono_coop_sem_interrupt, sem, &interrupted);
		if (interrupted) {
			MONO_LOG ("alertable:%d interrupted1", alertable);
			return MONO_SEM_TIMEDWAIT_RET_ALERTED;
		}
	}

	MONO_ENTER_GC_SAFE;

	res = mono_os_sem_timedwait (&sem->s, timeout_ms, flags);

	MONO_EXIT_GC_SAFE;

	if (alertable && implement_alertable)
		mono_thread_info_uninstall_interrupt (&interrupted);

	if (interrupted) {
		MONO_LOG ("alertable:%d interrupted2", alertable);
		return MONO_SEM_TIMEDWAIT_RET_ALERTED;
	}

	MONO_LOG ("alertable:%d res:%d%s", alertable, res, timed_wait_string (res));

	return res;
}
