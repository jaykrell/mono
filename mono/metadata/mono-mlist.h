/**
 * \file
 */

#ifndef __MONO_METADATA_MONO_MLIST_H__
#define __MONO_METADATA_MONO_MLIST_H__

/*
 * mono-mlist.h: Managed object list implementation
 */

#include <mono/metadata/object-internals.h>
#include <mono/metadata/handle-decl.h>

typedef struct _MonoMList MonoMList;

TYPED_HANDLE_DECL (MonoMList);

MONO_API MONO_RT_EXTERNAL_ONLY
MonoMList*  mono_mlist_alloc       (MonoObject *data);
MONO_API MonoObject* mono_mlist_get_data    (MonoMList* list);
MONO_API void        mono_mlist_set_data    (MonoMList* list, MonoObject *data);
MONO_API MonoMList*  mono_mlist_set_next    (MonoMList* list, MonoMList *next);
MONO_API int         mono_mlist_length      (MonoMList* list);
MONO_API MonoMList*  mono_mlist_next        (MonoMList* list);
MONO_API MonoMList*  mono_mlist_last        (MonoMList* list);
MONO_API MONO_RT_EXTERNAL_ONLY
MonoMList*  mono_mlist_prepend     (MonoMList* list, MonoObject *data);
MONO_API MONO_RT_EXTERNAL_ONLY
MonoMList*  mono_mlist_append      (MonoMList* list, MonoObject *data);

MonoMList*  mono_mlist_prepend_checked      (MonoMList* list, MonoObject *data, MonoError *error);
MonoMList*  mono_mlist_append_checked       (MonoMList* list, MonoObject *data, MonoError *error);

MONO_API MonoMList*  mono_mlist_remove_item (MonoMList* list, MonoMList *item);

//FIXME not all implemented.
MonoObjectHandle mono_mlist_get_data_handle (MonoMListHandle list);
void             mono_mlist_set_data_handle (MonoMListHandle list, MonoObjectHandle data);
MonoMListHandle  mono_mlist_set_next_handle (MonoMListHandle list, MonoMListHandle next);
int              mono_mlist_length_handle   (MonoMListHandle list);
MonoMListHandle  mono_mlist_next_handle     (MonoMListHandle list);
MonoMListHandle  mono_mlist_last_handle     (MonoMListHandle list);
MonoMListHandle  mono_mlist_prepend_handle  (MonoMListHandle list, MonoObjectHandle data, MonoError *error);
MonoMListHandle  mono_mlist_append_handle   (MonoMListHandle list, MonoObjectHandle data, MonoError *error);
MonoMListHandle  mono_mlist_remove_item_handle (MonoMListHandle list, MonoMListHandle item);

#endif /* __MONO_METADATA_MONO_MLIST_H__ */
