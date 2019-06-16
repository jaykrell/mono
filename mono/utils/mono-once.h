/**
 * \file
 */

#ifndef __MONO_ONCE_H__
#define __MONO_ONCE_H__

#if 0 // nice idea, but it is not used

#include "mono-lazy-init.h"

typedef mono_lazy_init_t mono_once_t;

#define MONO_ONCE_INIT MONO_LAZY_INIT_STATUS_NOT_INITIALIZED

static inline void
mono_once (mono_once_t *once, void (*once_init) (void))
{
	mono_lazy_initialize (once, once_init);
}

#endif

#endif /* __MONO_ONCE_H__ */
