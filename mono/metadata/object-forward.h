/**
 * \file
 *
 * Forward declarations of opaque types, and typedefs thereof.
 *
 */

#ifndef __MONO_OBJECT_FORWARD_H__
#define __MONO_OBJECT_FORWARD_H__

#include <mono/utils/mono-publib.h>

typedef struct _MonoAssemblyName MonoAssemblyName;
typedef struct _MonoClass MonoClass;
typedef struct _MonoClassField MonoClassField;
typedef struct _MonoEvent MonoEvent;
typedef struct  MonoEventInfo MonoEventInfo;
typedef struct  MonoGenericParamInfo MonoGenericParamInfo;
typedef struct _MonoImage MonoImage;
typedef struct _MonoIUnknown MonoIUnknown;
typedef struct _MonoMethod MonoMethod;
typedef struct  MonoMethodInfo MonoMethodInfo;
typedef struct _MonoNativeOverlapped MonoNativeOverlapped;
typedef struct  MonoPropertyInfo MonoPropertyInfo;
typedef struct _MonoThreadInfo MonoThreadInfo;
typedef struct _MonoType MonoType;
typedef struct _MonoTypedRef MonoTypedRef;
typedef struct _MonoProperty MonoProperty;
typedef struct  MonoW32ProcessInfo MonoW32ProcessInfo;

typedef struct _MonoAppContext MONO_RT_MANAGED_ATTR MonoAppContext;
typedef struct _MonoArray MONO_RT_MANAGED_ATTR MonoArray;
typedef struct _MonoObject MONO_RT_MANAGED_ATTR MonoObject;
typedef struct _MonoException MONO_RT_MANAGED_ATTR MonoException;
typedef struct _MonoReflectionAssembly MONO_RT_MANAGED_ATTR MonoReflectionAssembly;
typedef struct _MonoReflectionMethod MONO_RT_MANAGED_ATTR MonoReflectionMethod;
typedef struct _MonoReflectionModule MONO_RT_MANAGED_ATTR MonoReflectionModule;
typedef struct _MonoReflectionModuleBuilder MONO_RT_MANAGED_ATTR MonoReflectionModuleBuilder;
typedef struct  MonoReflectionMonoEvent MONO_RT_MANAGED_ATTR MonoReflectionMonoEvent;
typedef struct  MonoReflectionParameter MONO_RT_MANAGED_ATTR MonoReflectionParameter;
typedef struct _MonoReflectionProperty MONO_RT_MANAGED_ATTR MonoReflectionProperty;
typedef struct _MonoReflectionTypeBuilder MONO_RT_MANAGED_ATTR MonoReflectionTypeBuilder;
typedef struct _MonoString MONO_RT_MANAGED_ATTR MonoString;
typedef struct _MonoThread MONO_RT_MANAGED_ATTR MonoThreadObject;

#endif /* __MONO_OBJECT_FORWARD_H__ */
