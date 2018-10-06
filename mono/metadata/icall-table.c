/**
 * \file
 *
 * Authors:
 *   Dietmar Maurer (dietmar@ximian.com)
 *   Paolo Molaro (lupus@ximian.com)
 *	 Patrik Torstensson (patrik.torstensson@labs2.com)
 *   Marek Safar (marek.safar@gmail.com)
 *   Aleksey Kliger (aleksey@xamarin.com)
 *
 * Copyright 2001-2003 Ximian, Inc (http://www.ximian.com)
 * Copyright 2004-2009 Novell, Inc (http://www.novell.com)
 * Copyright 2011-2015 Xamarin Inc (http://www.xamarin.com).
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#include <config.h>
#include <glib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined (HAVE_WCHAR_H)
#include <wchar.h>
#endif

#include <mono/utils/mono-publib.h>
#include <mono/utils/bsearch.h>
#include <mono/metadata/icalls.h>
#include "handle-decl.h"
#include <mono/metadata/icall-table.h>
#include "environment.h"
#include "locales.h"
#include "w32file.h"
#include "marshal.h"
#include "security.h"
#include "w32socket.h"
#include "security-manager.h"
#include "security-core-clr.h"
#include "string-icalls.h"
#include "monitor.h"
#include "mono-perfcounters.h"
#include "w32semaphore.h"
#include "w32process.h"
#include "w32mutex.h"
#include "w32event.h"
#include "utils/mono-digest.h"
#include "mono-route.h"
#include "threadpool-io.h"
#include "filewatcher.h"

ICALL_EXPORT
double
ves_icall_System_Math_Floor (double x);

ICALL_EXPORT
double
ves_icall_System_Math_Round (double x);

ICALL_EXPORT
double
ves_icall_System_Math_FMod (double x, double y);

ICALL_EXPORT
double
ves_icall_System_Math_ModF (double x, double *d);

ICALL_EXPORT
double 
ves_icall_System_Math_Sin (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Cos (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Cbrt (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Tan (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Sinh (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Cosh (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Tanh (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Acos (double x);

ICALL_EXPORT
double
ves_icall_System_Math_Acosh (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Asin (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Asinh (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Atan (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Atan2 (double y, double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Atanh (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Exp (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Log (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Log10 (double x);

ICALL_EXPORT
double 
ves_icall_System_Math_Pow (double x, double y);

ICALL_EXPORT
double 
ves_icall_System_Math_Sqrt (double x);

ICALL_EXPORT
double
ves_icall_System_Math_Abs_double (double v);

ICALL_EXPORT
float
ves_icall_System_Math_Abs_single (float v);

ICALL_EXPORT
double
ves_icall_System_Math_Ceiling (double v);

ICALL_EXPORT
float
ves_icall_System_MathF_Acos (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Acosh (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Asin (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Asinh  (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Atan  (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Atan2 (float x, float y);

ICALL_EXPORT
float
ves_icall_System_MathF_Atanh (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Cbrt (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Ceiling (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Cos (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Cosh (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Exp (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Floor (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Log (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Log10 (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Pow (float x, float y);

ICALL_EXPORT
float
ves_icall_System_MathF_Sin (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Sinh (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Sqrt (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Tan (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_Tanh (float x);

ICALL_EXPORT
float
ves_icall_System_MathF_FMod (float x, float y);

ICALL_EXPORT
float
ves_icall_System_MathF_ModF (float x, float *d);

typedef union _MonoError MonoError;

ICALL_EXPORT
MonoReflectionAssemblyHandle
ves_icall_System_Reflection_Assembly_GetCallingAssembly (MonoError *error);

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Array_GetValueImpl (MonoArrayHandle array, guint32 pos, MonoError *error);

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Array_GetValue (MonoArrayHandle arr, MonoArrayHandle indices, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Array_SetValueImpl (MonoArrayHandle arr, MonoObjectHandle value, guint32 pos, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Array_SetValue (MonoArrayHandle arr, MonoObjectHandle value,
				 MonoArrayHandle idxs, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_System_Array_CreateInstanceImpl (MonoReflectionTypeHandle type, MonoArrayHandle lengths, MonoArrayHandle bounds, MonoError *error);

ICALL_EXPORT gint32
ves_icall_System_Array_GetRank (MonoObjectHandle arr, MonoError *error);


ICALL_EXPORT gint32
ves_icall_System_Array_GetLength (MonoArrayHandle arr, gint32 dimension, MonoError *error);

ICALL_EXPORT gint64
ves_icall_System_Array_GetLongLength (MonoArrayHandle arr, gint32 dimension, MonoError *error);

ICALL_EXPORT gint32
ves_icall_System_Array_GetLowerBound (MonoArrayHandle arr, gint32 dimension, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Array_ClearInternal (MonoArrayHandle arr, int idx, int length, MonoError *error);

ICALL_EXPORT gboolean
ves_icall_System_Array_FastCopy (MonoArray *source, int source_idx, MonoArray* dest, int dest_idx, int length);

ICALL_EXPORT void
ves_icall_System_Array_GetGenericValueImpl (MonoArray *arr, guint32 pos, gpointer value);

ICALL_EXPORT void
ves_icall_System_Array_SetGenericValueImpl (MonoArray *arr, guint32 pos, gpointer value);

ICALL_EXPORT void
ves_icall_System_Runtime_RuntimeImports_Memmove (guint8 *destination, guint8 *source, guint byte_count);

ICALL_EXPORT void
ves_icall_System_Runtime_RuntimeImports_Memmove_wbarrier (guint8 *destination, guint8 *source, guint len, MonoType *type);

ICALL_EXPORT void
ves_icall_System_Runtime_RuntimeImports_ZeroMemory (guint8 *p, guint byte_length);

ICALL_EXPORT void
ves_icall_System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray (MonoArrayHandle array, MonoClassField *field_handle, MonoError *error);

ICALL_EXPORT gint
ves_icall_System_Runtime_CompilerServices_RuntimeHelpers_GetOffsetToStringData (void);

ICALL_EXPORT MonoObject *
ves_icall_System_Runtime_CompilerServices_RuntimeHelpers_GetObjectValue (MonoObject *obj);

ICALL_EXPORT void
ves_icall_System_Runtime_CompilerServices_RuntimeHelpers_RunClassConstructor (MonoType *handle);

ICALL_EXPORT void
ves_icall_System_Runtime_CompilerServices_RuntimeHelpers_RunModuleConstructor (MonoImage *image);

ICALL_EXPORT MonoBoolean
ves_icall_System_Runtime_CompilerServices_RuntimeHelpers_SufficientExecutionStack (void);

ICALL_EXPORT MonoObject *
ves_icall_System_Object_MemberwiseClone (MonoObject *this_obj);

ICALL_EXPORT gint32
ves_icall_System_ValueType_InternalGetHashCode (MonoObject *this_obj, MonoArray **fields);

ICALL_EXPORT MonoBoolean
ves_icall_System_ValueType_Equals (MonoObject *this_obj, MonoObject *that, MonoArray **fields);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_System_Object_GetType (MonoObjectHandle obj, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_System_Type_internal_from_name (MonoStringHandle name,
					  MonoBoolean throwOnError,
					  MonoBoolean ignoreCase,
					  MonoError *error);
ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_System_Type_internal_from_handle (MonoType *handle, MonoError *error);

ICALL_EXPORT MonoType*
ves_icall_Mono_RuntimeClassHandle_GetTypeFromClass (MonoClass *klass, MonoError *error);

ICALL_EXPORT void
ves_icall_Mono_RuntimeGPtrArrayHandle_GPtrArrayFree (GPtrArray *ptr_array, MonoError *error);

ICALL_EXPORT void
ves_icall_Mono_SafeStringMarshal_GFree (void *c_str, MonoError *error);

ICALL_EXPORT char*
ves_icall_Mono_SafeStringMarshal_StringToUtf8 (MonoStringHandle s, MonoError *error);

ICALL_EXPORT guint32
ves_icall_type_GetTypeCodeInternal (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT guint32
ves_icall_RuntimeTypeHandle_type_is_assignable_from (MonoReflectionTypeHandle ref_type, MonoReflectionTypeHandle ref_c, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_RuntimeTypeHandle_is_subclass_of (MonoType *childType, MonoType *baseType);

ICALL_EXPORT guint32
ves_icall_RuntimeTypeHandle_IsInstanceOfType (MonoReflectionTypeHandle ref_type, MonoObjectHandle obj, MonoError *error);

ICALL_EXPORT guint32
ves_icall_RuntimeTypeHandle_GetAttributes (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoReflectionMarshalAsAttributeHandle
ves_icall_System_Reflection_FieldInfo_get_marshal_info (MonoReflectionFieldHandle field_h, MonoError *error);

ICALL_EXPORT MonoReflectionFieldHandle
ves_icall_System_Reflection_FieldInfo_internal_from_handle_type (MonoClassField *handle, MonoType *type, MonoError *error);

ICALL_EXPORT MonoReflectionEventHandle
ves_icall_System_Reflection_EventInfo_internal_from_handle_type (MonoEvent *handle, MonoType *type, MonoError *error);

ICALL_EXPORT MonoReflectionPropertyHandle
ves_icall_System_Reflection_PropertyInfo_internal_from_handle_type (MonoProperty *handle, MonoType *type, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_System_Reflection_FieldInfo_GetTypeModifiers (MonoReflectionFieldHandle field_h, MonoBoolean optional, MonoError *error);

ICALL_EXPORT int
ves_icall_get_method_attributes (MonoMethod *method);

ICALL_EXPORT void
ves_icall_get_method_info (MonoMethod *method, MonoMethodInfo *info, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_System_Reflection_MonoMethodInfo_get_parameter_info (MonoMethod *method, MonoReflectionMethodHandle member, MonoError *error);

ICALL_EXPORT MonoReflectionMarshalAsAttributeHandle
ves_icall_System_MonoMethodInfo_get_retval_marshal (MonoMethod *method, MonoError *error);

ICALL_EXPORT gint32
ves_icall_MonoField_GetFieldOffset (MonoReflectionFieldHandle field, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_MonoField_GetParentType (MonoReflectionFieldHandle field, MonoBoolean declaring, MonoError *error);

ICALL_EXPORT MonoObject *
ves_icall_MonoField_GetValueInternal (MonoReflectionField *field, MonoObject *obj);

ICALL_EXPORT void
ves_icall_MonoField_SetValueInternal (MonoReflectionFieldHandle field, MonoObjectHandle obj, MonoObjectHandle value, MonoError  *error);

void
ves_icall_MonoField_SetValueInternal2 (MonoReflectionFieldHandle field, MonoObjectHandle obj, MonoObjectHandle value, MonoError  *error);

ICALL_EXPORT void
ves_icall_System_RuntimeFieldHandle_SetValueDirect (MonoReflectionField *field, MonoReflectionType *field_type, MonoTypedRef *obj, MonoObject *value, MonoReflectionType *context_type);

ICALL_EXPORT MonoObject *
ves_icall_MonoField_GetRawConstantValue (MonoReflectionField *rfield);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_MonoField_ResolveType (MonoReflectionFieldHandle ref_field, MonoError *error);

ICALL_EXPORT void
ves_icall_MonoPropertyInfo_get_property_info (MonoReflectionPropertyHandle property, MonoPropertyInfo *info, PInfo req_info, MonoError *error);

ICALL_EXPORT void
ves_icall_MonoEventInfo_get_event_info (MonoReflectionMonoEventHandle ref_event, MonoEventInfo *info, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_RuntimeType_GetInterfaces (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT void
ves_icall_RuntimeType_GetInterfaceMapData (MonoReflectionTypeHandle ref_type, MonoReflectionTypeHandle ref_iface, MonoArrayHandleOut targets, MonoArrayHandleOut methods, MonoError *error);

ICALL_EXPORT void
ves_icall_RuntimeType_GetPacking (MonoReflectionTypeHandle ref_type, guint32 *packing, guint32 *size, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_RuntimeTypeHandle_GetElementType (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_RuntimeTypeHandle_GetBaseType (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_RuntimeTypeHandle_IsPointer (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_RuntimeTypeHandle_IsPrimitive (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_RuntimeTypeHandle_HasReferences (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_RuntimeTypeHandle_IsByRef (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_RuntimeTypeHandle_IsComObject (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT guint32
ves_icall_reflection_get_token (MonoObjectHandle obj, MonoError *error);

guint32
ves_icall_reflection_get_token1 (MonoObjectHandle obj, MonoError *error);

guint32
ves_icall_reflection_get_token2 (MonoObjectHandle obj, MonoError *error);

guint32
ves_icall_reflection_get_token3 (MonoObjectHandle obj, MonoError *error);

guint32
ves_icall_reflection_get_token4 (MonoObjectHandle obj, MonoError *error);

guint32
ves_icall_reflection_get_token5 (MonoObjectHandle obj, MonoError *error);

guint32
ves_icall_reflection_get_token6 (MonoObjectHandle obj, MonoError *error);

guint32
ves_icall_reflection_get_token7 (MonoObjectHandle obj, MonoError *error);

guint32
ves_icall_reflection_get_token8 (MonoObjectHandle obj, MonoError *error);

ICALL_EXPORT MonoReflectionModuleHandle
ves_icall_RuntimeTypeHandle_GetModule (MonoReflectionTypeHandle type, MonoError *error);

ICALL_EXPORT MonoReflectionAssemblyHandle
ves_icall_RuntimeTypeHandle_GetAssembly (MonoReflectionTypeHandle type, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_RuntimeType_get_DeclaringType (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_RuntimeType_get_Name (MonoReflectionTypeHandle reftype, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_RuntimeType_get_Namespace (MonoReflectionTypeHandle type, MonoError *error);

ICALL_EXPORT gint32
ves_icall_RuntimeTypeHandle_GetArrayRank (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_RuntimeType_GetGenericArguments (MonoReflectionTypeHandle ref_type, MonoBoolean runtimeTypeArray, MonoError *error);

ICALL_EXPORT gboolean
ves_icall_RuntimeTypeHandle_IsGenericTypeDefinition (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_RuntimeTypeHandle_GetGenericTypeDefinition_impl (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_RuntimeType_MakeGenericType (MonoReflectionTypeHandle reftype, MonoArrayHandle type_array, MonoError *error);

ICALL_EXPORT gboolean
ves_icall_RuntimeTypeHandle_HasInstantiation (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT gint32
ves_icall_RuntimeType_GetGenericParameterPosition (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoGenericParamInfo *
ves_icall_RuntimeTypeHandle_GetGenericParameterInfo (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_RuntimeTypeHandle_IsGenericVariable (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_RuntimeType_GetCorrespondingInflatedMethod (MonoReflectionTypeHandle ref_type,
						      MonoReflectionMethodHandle generic,
						      MonoError *error);
MonoReflectionMethodHandle
ves_icall_RuntimeType_GetCorrespondingInflatedMethod2 (MonoReflectionTypeHandle ref_type,
						      MonoReflectionMethodHandle generic,
						      MonoError *error);
ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_RuntimeType_get_DeclaringMethod (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_RuntimeType_IsTypeExportedToWindowsRuntime (MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_RuntimeType_IsWindowsRuntimeObjectType (MonoError *error);

ICALL_EXPORT void
ves_icall_MonoMethod_GetPInvoke (MonoReflectionMethodHandle ref_method, int* flags, MonoStringHandleOut entry_point, MonoStringHandleOut dll_name, MonoError *error);

ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_MonoMethod_GetGenericMethodDefinition (MonoReflectionMethodHandle ref_method, MonoError *error);

MonoReflectionMethodHandle
ves_icall_MonoMethod_GetGenericMethodDefinition2 (MonoReflectionMethodHandle ref_method, MonoError *error);

ICALL_EXPORT gboolean
ves_icall_MonoMethod_get_IsGenericMethod (MonoReflectionMethodHandle ref_method, MonoError *erro);

ICALL_EXPORT gboolean
ves_icall_MonoMethod_get_IsGenericMethodDefinition (MonoReflectionMethodHandle ref_method, MonoError *Error);

ICALL_EXPORT MonoArrayHandle
ves_icall_MonoMethod_GetGenericArguments (MonoReflectionMethodHandle ref_method, MonoError *error);

ICALL_EXPORT MonoObject *
ves_icall_InternalInvoke (MonoReflectionMethod *method, MonoObject *this_arg, MonoArray *params, MonoException **exc);

#ifndef DISABLE_REMOTING
ICALL_EXPORT MonoObject *
ves_icall_InternalExecute (MonoReflectionMethod *method, MonoObject *this_arg, MonoArray *params, MonoArray **outArgs);
#endif

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Enum_ToObject (MonoReflectionTypeHandle enumType, guint64 value, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Enum_InternalHasFlag (MonoObjectHandle a, MonoObjectHandle b, MonoError *error);

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Enum_get_value (MonoObjectHandle ehandle, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_System_Enum_get_underlying_type (MonoReflectionTypeHandle type, MonoError *error);

ICALL_EXPORT int
ves_icall_System_Enum_compare_value_to (MonoObjectHandle enumHandle, MonoObjectHandle otherHandle, MonoError *error);

ICALL_EXPORT int
ves_icall_System_Enum_get_hashcode (MonoObjectHandle enumHandle, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Enum_GetEnumValuesAndNames (MonoReflectionTypeHandle type, MonoArrayHandleOut values, MonoArrayHandleOut names, MonoError *error);

ICALL_EXPORT GPtrArray*
ves_icall_RuntimeType_GetFields_native (MonoReflectionTypeHandle ref_type, char *utf8_name, guint32 bflags, guint32 mlisttype, MonoError *error);

ICALL_EXPORT GPtrArray*
ves_icall_RuntimeType_GetMethodsByName_native (MonoReflectionTypeHandle ref_type, const char *mname, guint32 bflags, guint32 mlisttype, MonoError *error);

ICALL_EXPORT GPtrArray*
ves_icall_RuntimeType_GetConstructors_native (MonoReflectionTypeHandle ref_type, guint32 bflags, MonoError *error);

ICALL_EXPORT GPtrArray*
ves_icall_RuntimeType_GetPropertiesByName_native (MonoReflectionTypeHandle ref_type, gchar *propname, guint32 bflags, guint32 mlisttype, MonoError *error);

ICALL_EXPORT GPtrArray*
ves_icall_RuntimeType_GetEvents_native (MonoReflectionTypeHandle ref_type, char *utf8_name, guint32 bflags, guint32 mlisttype, MonoError *error);

ICALL_EXPORT GPtrArray *
ves_icall_RuntimeType_GetNestedTypes_native (MonoReflectionTypeHandle ref_type, char *str, guint32 bflags, guint32 mlisttype, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_System_Reflection_Assembly_InternalGetType (MonoReflectionAssemblyHandle assembly_h, MonoReflectionModuleHandle module, MonoStringHandle name, MonoBoolean throwOnError, MonoBoolean ignoreCase, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Reflection_Assembly_get_code_base (MonoReflectionAssemblyHandle assembly, MonoBoolean escaped, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Reflection_Assembly_get_global_assembly_cache (MonoReflectionAssemblyHandle assembly, MonoError *error);

ICALL_EXPORT MonoReflectionAssemblyHandle
ves_icall_System_Reflection_Assembly_load_with_partial_name (MonoStringHandle mname, MonoObjectHandle evidence, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Reflection_Assembly_get_location (MonoReflectionAssemblyHandle refassembly, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Reflection_Assembly_get_ReflectionOnly (MonoReflectionAssemblyHandle assembly_h, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Reflection_Assembly_InternalImageRuntimeVersion (MonoReflectionAssemblyHandle refassembly, MonoError *error);

ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_System_Reflection_Assembly_get_EntryPoint (MonoReflectionAssemblyHandle assembly_h, MonoError *error);

ICALL_EXPORT MonoReflectionModuleHandle
ves_icall_System_Reflection_Assembly_GetManifestModuleInternal (MonoReflectionAssemblyHandle assembly, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_System_Reflection_Assembly_GetManifestResourceNames (MonoReflectionAssemblyHandle assembly_h, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Reflection_Assembly_GetAotId (MonoError *error);

ICALL_EXPORT GPtrArray*
ves_icall_System_Reflection_Assembly_InternalGetReferencedAssemblies (MonoReflectionAssemblyHandle assembly, MonoError *error);

ICALL_EXPORT void *
ves_icall_System_Reflection_Assembly_GetManifestResourceInternal (MonoReflectionAssemblyHandle assembly_h, MonoStringHandle name, gint32 *size, MonoReflectionModuleHandleOut ref_module, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Reflection_Assembly_GetManifestResourceInfoInternal (MonoReflectionAssemblyHandle assembly_h, MonoStringHandle name, MonoManifestResourceInfoHandle info_h, MonoError *error);

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Reflection_Assembly_GetFilesInternal (MonoReflectionAssemblyHandle assembly_h, MonoStringHandle name, MonoBoolean resource_modules, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_System_Reflection_Assembly_GetModulesInternal (MonoReflectionAssemblyHandle assembly_h, MonoError *error);

ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_GetCurrentMethod (MonoError *error);

ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_System_Reflection_MethodBase_GetMethodFromHandleInternalType_native (MonoMethod *method, MonoType *type, MonoBoolean generic_check, MonoError *error);

ICALL_EXPORT MonoReflectionMethodBodyHandle
ves_icall_System_Reflection_MethodBase_GetMethodBodyInternal (MonoMethod *method, MonoError *error);

ICALL_EXPORT MonoReflectionAssemblyHandle
ves_icall_System_Reflection_Assembly_GetExecutingAssembly (MonoError *error);

ICALL_EXPORT MonoReflectionAssemblyHandle
ves_icall_System_Reflection_Assembly_GetEntryAssembly (MonoError *error);

ICALL_EXPORT MonoReflectionAssemblyHandle
ves_icall_System_Reflection_Assembly_GetCallingAssembly (MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_RuntimeType_getFullName (MonoReflectionTypeHandle object, gboolean full_name,
										  gboolean assembly_qualified, MonoError *error);

ICALL_EXPORT int
ves_icall_RuntimeType_get_core_clr_security_level (MonoReflectionTypeHandle rfield, MonoError *error);

ICALL_EXPORT int
ves_icall_MonoField_get_core_clr_security_level (MonoReflectionField *rfield);

ICALL_EXPORT int
ves_icall_MonoMethod_get_core_clr_security_level (MonoReflectionMethodHandle rfield, MonoError *error);

int
ves_icall_MonoMethod_get_core_clr_security_level2 (MonoReflectionMethodHandle rfield, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Reflection_Assembly_get_fullName (MonoReflectionAssemblyHandle assembly, MonoError *error);

ICALL_EXPORT MonoAssemblyName *
ves_icall_System_Reflection_AssemblyName_GetNativeName (MonoAssembly *mass);

ICALL_EXPORT void
ves_icall_System_Reflection_Assembly_InternalGetAssemblyName (MonoStringHandle fname, MonoAssemblyName *name, MonoStringHandleOut normalized_codebase, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Reflection_Assembly_LoadPermissions (MonoReflectionAssemblyHandle assembly_h,
						      char **minimum, guint32 *minLength, char **optional, guint32 *optLength, char **refused, guint32 *refLength, MonoError *error);
ICALL_EXPORT MonoArrayHandle
ves_icall_System_Reflection_Assembly_GetTypes (MonoReflectionAssemblyHandle assembly_handle, MonoBoolean exportedOnly, MonoError *error);

ICALL_EXPORT void
ves_icall_Mono_RuntimeMarshal_FreeAssemblyName (MonoAssemblyName *aname, MonoBoolean free_struct, MonoError *error);

ICALL_EXPORT void
ves_icall_Mono_Runtime_DisableMicrosoftTelemetry (MonoError *error);

ICALL_EXPORT void
ves_icall_Mono_Runtime_EnableMicrosoftTelemetry (char *appBundleID, char *appSignature, char *appVersion, char *merpGUIPath, char *eventType, char *appPath, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_Mono_Runtime_ExceptionToState (MonoExceptionHandle exc_handle, guint64 *portable_hash_out, guint64 *unportable_hash_out, MonoError *error);

ICALL_EXPORT void
ves_icall_Mono_Runtime_SendMicrosoftTelemetry (char *payload, guint64 portable_hash, guint64 unportable_hash, MonoError *error);

ICALL_EXPORT void
ves_icall_Mono_Runtime_DumpTelemetry (char *payload, guint64 portable_hash, guint64 unportable_hash, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Reflection_AssemblyName_ParseAssemblyName (const char *name, MonoAssemblyName *aname, MonoBoolean *is_version_defined_arg, MonoBoolean *is_token_defined_arg);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_System_Reflection_Module_GetGlobalType (MonoReflectionModuleHandle module, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Reflection_Module_Close (MonoReflectionModuleHandle module, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Reflection_Module_GetGuidInternal (MonoReflectionModuleHandle refmodule, MonoError *error);

ICALL_EXPORT gpointer
ves_icall_System_Reflection_Module_GetHINSTANCE (MonoReflectionModuleHandle module, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Reflection_Module_GetPEKind (MonoImage *image, gint32 *pe_kind, gint32 *machine, MonoError *error);

ICALL_EXPORT gint32
ves_icall_System_Reflection_Module_GetMDStreamVersion (MonoImage *image, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_System_Reflection_Module_InternalGetTypes (MonoReflectionModuleHandle module, MonoError *error);

ICALL_EXPORT MonoType*
ves_icall_System_Reflection_Module_ResolveTypeToken (MonoImage *image, guint32 token, MonoArrayHandle type_args, MonoArrayHandle method_args, MonoResolveTokenError *resolve_error, MonoError *error);

ICALL_EXPORT MonoMethod*
ves_icall_System_Reflection_Module_ResolveMethodToken (MonoImage *image, guint32 token, MonoArrayHandle type_args, MonoArrayHandle method_args, MonoResolveTokenError *resolve_error, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Reflection_Module_ResolveStringToken (MonoImage *image, guint32 token, MonoResolveTokenError *resolve_error, MonoError *error);

ICALL_EXPORT MonoClassField*
ves_icall_System_Reflection_Module_ResolveFieldToken (MonoImage *image, guint32 token, MonoArrayHandle type_args, MonoArrayHandle method_args, MonoResolveTokenError *resolve_error, MonoError *error);

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Reflection_Module_ResolveMemberToken (MonoImage *image, guint32 token, MonoArrayHandle type_args, MonoArrayHandle method_args, MonoResolveTokenError *error, MonoError *merror);

ICALL_EXPORT MonoArrayHandle
ves_icall_System_Reflection_Module_ResolveSignature (MonoImage *image, guint32 token, MonoResolveTokenError *resolve_error, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_RuntimeTypeHandle_IsArray (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_RuntimeType_make_array_type (MonoReflectionTypeHandle ref_type, int rank, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_RuntimeType_make_byref_type (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoReflectionTypeHandle
ves_icall_RuntimeType_MakePointerType (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Delegate_CreateDelegate_internal (MonoReflectionTypeHandle ref_type, MonoObjectHandle target,
						   MonoReflectionMethodHandle info, MonoBoolean throwOnBindFailure, MonoError *error);

ICALL_EXPORT MonoMulticastDelegateHandle
ves_icall_System_Delegate_AllocDelegateLike_internal (MonoDelegateHandle delegate, MonoError *error);

ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_System_Delegate_GetVirtualMethod_internal (MonoDelegateHandle delegate, MonoError *error);

ICALL_EXPORT gint32
ves_icall_System_Buffer_ByteLengthInternal (MonoArray *array);

ICALL_EXPORT gint8
ves_icall_System_Buffer_GetByteInternal (MonoArray *array, gint32 idx);

ICALL_EXPORT void 
ves_icall_System_Buffer_SetByteInternal (MonoArray *array, gint32 idx, gint8 value);

ICALL_EXPORT MonoBoolean
ves_icall_System_Buffer_BlockCopyInternal (MonoArray *src, gint32 src_offset, MonoArray *dest, gint32 dest_offset, gint32 count);

#ifndef DISABLE_REMOTING
ICALL_EXPORT MonoObjectHandle
ves_icall_Remoting_RealProxy_GetTransparentProxy (MonoObjectHandle this_obj, MonoStringHandle class_name, MonoError *error);

ICALL_EXPORT MonoReflectionType *
ves_icall_Remoting_RealProxy_InternalGetProxyType (MonoTransparentProxy *tp);
#endif

MonoStringHandle
ves_icall_System_Environment_get_UserName (MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Environment_get_MachineName (MonoError *error);

ICALL_EXPORT int
ves_icall_System_Environment_get_Platform (void);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Environment_get_NewLine (MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Environment_GetIs64BitOperatingSystem (void);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Environment_GetEnvironmentVariable_native (const gchar *utf8_name, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_System_Environment_GetCommandLineArgs (MonoError *error);

ICALL_EXPORT MonoArray *
ves_icall_System_Environment_GetEnvironmentVariableNames (void);

ICALL_EXPORT void
ves_icall_System_Environment_InternalSetEnvironmentVariable (MonoString *name, MonoString *value);

ICALL_EXPORT void
ves_icall_System_Environment_Exit (int result);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Environment_GetGacPath (MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Environment_GetWindowsFolderPath (int folder, MonoError *error);

ICALL_EXPORT MonoArray *
ves_icall_System_Environment_GetLogicalDrives (void);

ICALL_EXPORT MonoString *
ves_icall_System_IO_DriveInfo_GetDriveFormat (MonoString *path);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Environment_InternalGetHome (MonoError *error);;

ICALL_EXPORT MonoStringHandle
ves_icall_System_Text_EncodingHelper_InternalCodePage (gint32 *int_code_page, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Environment_get_HasShutdownStarted (void);

ICALL_EXPORT void
ves_icall_System_Environment_BroadcastSettingChange (MonoError *error);

ICALL_EXPORT
gint32
ves_icall_System_Environment_get_TickCount (void);

ICALL_EXPORT gint32
ves_icall_System_Runtime_Versioning_VersioningHelper_GetRuntimeId (MonoError *error);

#ifndef DISABLE_REMOTING
ICALL_EXPORT MonoBoolean
ves_icall_IsTransparentProxy (MonoObjectHandle proxy, MonoError *error);

ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_Remoting_RemotingServices_GetVirtualMethod (
	MonoReflectionTypeHandle rtype, MonoReflectionMethodHandle rmethod, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Runtime_Activation_ActivationServices_EnableProxyActivation (MonoReflectionTypeHandle type, MonoBoolean enable, MonoError *error);

#else /* DISABLE_REMOTING */

ICALL_EXPORT void
ves_icall_System_Runtime_Activation_ActivationServices_EnableProxyActivation (MonoReflectionTypeHandle type, MonoBoolean enable, MonoError *error);

#endif

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Runtime_Activation_ActivationServices_AllocateUninitializedClassInstance (MonoReflectionTypeHandle type, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_IO_get_temp_path (MonoError *error);

#ifndef PLATFORM_NO_DRIVEINFO
ICALL_EXPORT MonoBoolean
ves_icall_System_IO_DriveInfo_GetDiskFreeSpace (MonoString *path_name, guint64 *free_bytes_avail,
						guint64 *total_number_of_bytes, guint64 *total_number_of_free_bytes,
						gint32 *error);

ICALL_EXPORT guint32
ves_icall_System_IO_DriveInfo_GetDriveType (MonoString *root_path_name);

#endif /* PLATFORM_NO_DRIVEINFO */

ICALL_EXPORT gpointer
ves_icall_RuntimeMethodHandle_GetFunctionPointer (MonoMethod *method, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Configuration_DefaultConfig_get_machine_config_path (MonoError *error);

MonoStringHandle
ves_icall_System_Environment_GetMachineConfigPath (MonoError *error);

MonoStringHandle
ves_icall_System_Web_Util_ICalls_GetMachineConfigPath (MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Configuration_InternalConfigurationHost_get_bundled_app_config (MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Environment_get_bundled_machine_config (MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Configuration_DefaultConfig_get_bundled_machine_config (MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Configuration_InternalConfigurationHost_get_bundled_machine_config (MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Web_Util_ICalls_get_machine_install_dir (MonoError *error);

ICALL_EXPORT gboolean
ves_icall_get_resources_ptr (MonoReflectionAssemblyHandle assembly, gpointer *result, gint32 *size, MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Diagnostics_Debugger_IsAttached_internal (MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Diagnostics_Debugger_IsLogging (MonoError *error);

ICALL_EXPORT void
ves_icall_System_Diagnostics_Debugger_Log (int level, MonoStringHandle category, MonoStringHandle message, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Diagnostics_DefaultTraceListener_WriteWindowsDebugString (const gunichar2 *message, MonoError *error);

/* Only used for value types */
ICALL_EXPORT MonoObjectHandle
ves_icall_System_Activator_CreateInstanceInternal (MonoReflectionTypeHandle ref_type, MonoError *error);

ICALL_EXPORT MonoReflectionMethodHandle
ves_icall_MonoMethod_get_base_method (MonoReflectionMethodHandle m, gboolean definition, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_MonoMethod_get_name (MonoReflectionMethodHandle m, MonoError *error);

ICALL_EXPORT void
mono_ArgIterator_Setup (MonoArgIterator *iter, char* argsp, char* start);

ICALL_EXPORT MonoTypedRef
mono_ArgIterator_IntGetNextArg (MonoArgIterator *iter);

ICALL_EXPORT MonoTypedRef
mono_ArgIterator_IntGetNextArgT (MonoArgIterator *iter, MonoType *type);

ICALL_EXPORT MonoType*
mono_ArgIterator_IntGetNextArgType (MonoArgIterator *iter);

ICALL_EXPORT MonoObjectHandle
mono_TypedReference_ToObject (MonoTypedRef* tref, MonoError *error);

ICALL_EXPORT MonoTypedRef
mono_TypedReference_MakeTypedReferenceInternal (MonoObject *target, MonoArray *fields);

ICALL_EXPORT void
ves_icall_System_Runtime_InteropServices_Marshal_Prelink (MonoReflectionMethodHandle method, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Runtime_InteropServices_Marshal_PrelinkAll (MonoReflectionTypeHandle type, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_System_Runtime_InteropServices_RuntimeInformation_get_RuntimeArchitecture (MonoError *error);

ICALL_EXPORT int
ves_icall_Interop_Sys_DoubleToString(double value, char *format, char *buffer, int bufferLength);

ICALL_EXPORT void
ves_icall_System_Runtime_RuntimeImports_ecvt_s(char *buffer, size_t sizeInBytes, double value, int count, int* dec, int* sign);

/* These parameters are "readonly" in corlib/System/NumberFormatter.cs */
ICALL_EXPORT void
ves_icall_System_NumberFormatter_GetFormatterTables (guint64 const **mantissas,
					    gint32 const **exponents,
					    gunichar2 const **digitLowerTable,
					    gunichar2 const **digitUpperTable,
					    gint64 const **tenPowersList,
					    gint32 const **decHexDigits);
ICALL_EXPORT MonoArrayHandle
ves_icall_ParameterInfo_GetTypeModifiers (MonoReflectionParameterHandle param, MonoBoolean optional, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_MonoPropertyInfo_GetTypeModifiers (MonoReflectionPropertyHandle property, MonoBoolean optional, MonoError *error);

ICALL_EXPORT MonoObject*
ves_icall_property_info_get_default_value (MonoReflectionProperty *property);

ICALL_EXPORT MonoBoolean
ves_icall_MonoCustomAttrs_IsDefinedInternal (MonoObjectHandle obj, MonoReflectionTypeHandle attr_type, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_MonoCustomAttrs_GetCustomAttributesInternal (MonoObjectHandle obj, MonoReflectionTypeHandle attr_type, MonoBoolean pseudoattrs, MonoError *error);

ICALL_EXPORT MonoArrayHandle
ves_icall_MonoCustomAttrs_GetCustomAttributesDataInternal (MonoObjectHandle obj, MonoError *error);

ICALL_EXPORT MonoStringHandle
ves_icall_Mono_Runtime_GetDisplayName (MonoError *error);

ICALL_EXPORT gint32
ves_icall_Microsoft_Win32_NativeMethods_WaitForInputIdle (gpointer handle, gint32 milliseconds, MonoError *error);

ICALL_EXPORT gint32
ves_icall_Microsoft_Win32_NativeMethods_GetCurrentProcessId (MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_Mono_TlsProviderFactory_IsBtlsSupported (MonoError *error);

#ifndef DISABLE_COM

ICALL_EXPORT int
ves_icall_System_Runtime_InteropServices_Marshal_GetHRForException_WinRT(MonoExceptionHandle ex, MonoError *error);

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Runtime_InteropServices_Marshal_GetNativeActivationFactory(MonoObjectHandle type, MonoError *error);

ICALL_EXPORT void*
ves_icall_System_Runtime_InteropServices_Marshal_GetRawIUnknownForComObjectNoAddRef(MonoObjectHandle obj, MonoError *error);

ICALL_EXPORT MonoObjectHandle
ves_icall_System_Runtime_InteropServices_WindowsRuntime_UnsafeNativeMethods_GetRestrictedErrorInfo(MonoError *error);

ICALL_EXPORT MonoBoolean
ves_icall_System_Runtime_InteropServices_WindowsRuntime_UnsafeNativeMethods_RoOriginateLanguageException (int ierr, MonoStringHandle message, void* languageException, MonoError *error);

ICALL_EXPORT void
ves_icall_System_Runtime_InteropServices_WindowsRuntime_UnsafeNativeMethods_RoReportUnhandledError (MonoObjectHandle oerr, MonoError *error);

ICALL_EXPORT int
ves_icall_System_Runtime_InteropServices_WindowsRuntime_UnsafeNativeMethods_WindowsCreateString(MonoStringHandle sourceString, int length, void** hstring, MonoError *error);

ICALL_EXPORT int
ves_icall_System_Runtime_InteropServices_WindowsRuntime_UnsafeNativeMethods_WindowsDeleteString(void* hstring, MonoError *error);

ICALL_EXPORT mono_unichar2*
ves_icall_System_Runtime_InteropServices_WindowsRuntime_UnsafeNativeMethods_WindowsGetStringRawBuffer(void* hstring, unsigned* length, MonoError *error);

#endif

ICALL_EXPORT void
ves_icall_System_IO_LogcatTextWriter_Log (const char *appname, gint32 level, const char *message);

ICALL_EXPORT int
ves_icall_System_GC_GetCollectionCount (int generation, MonoError *error);

ICALL_EXPORT int
ves_icall_System_GC_GetGeneration (MonoObjectHandle object, MonoError *error);

ICALL_EXPORT int
ves_icall_System_GC_GetMaxGeneration (MonoError *error);

ICALL_EXPORT void
ves_icall_System_GC_RecordPressure (gint64 value, MonoError *error);

ICALL_EXPORT gint64
ves_icall_System_Diagnostics_Stopwatch_GetTimestamp (void);

ICALL_EXPORT gint64
ves_icall_System_Threading_Timer_GetTimeMonotonic (void);

ICALL_EXPORT gint64
ves_icall_System_DateTime_GetSystemTimeAsFileTime (void);

ICALL_EXPORT int
ves_icall_System_Threading_Thread_SystemMaxStackSize (void);

ICALL_EXPORT gboolean
ves_icall_System_Threading_Thread_YieldInternal (void);

void*
mono_perfcounter_get_impl (MonoString* category, MonoString* counter, MonoString* instance, int *type, MonoBoolean *custom);

MonoBoolean
mono_perfcounter_get_sample (void *impl, MonoBoolean only_value, MonoCounterSample *sample);

gint64
mono_perfcounter_update_value (void *impl, MonoBoolean do_incr, gint64 value);

void
mono_perfcounter_free_data (void *impl);

/* Category icalls */
MonoBoolean
mono_perfcounter_category_del (MonoString *name);

MonoString*
mono_perfcounter_category_help (MonoString *category);

MonoBoolean
mono_perfcounter_category_exists (MonoString *counter, MonoString *category);

MonoBoolean
mono_perfcounter_create (MonoString *category, MonoString *help, int type, MonoArray *items);

MonoBoolean
mono_perfcounter_instance_exists (MonoString *instance, MonoString *category);

MonoArray*
mono_perfcounter_category_names (void);

MonoArray*
mono_perfcounter_counter_names (MonoString *category);

MonoArray*
mono_perfcounter_instance_names (MonoString *category);

MONO_API int mono_cpu_count (void);

#define NOHANDLES(inner) inner
#define HANDLES(...) MONO_HANDLE_DECLARE_RAW (__VA_ARGS__);
#define HANDLES_MAYBE(cond, ...) MONO_HANDLE_DECLARE_RAW (__VA_ARGS__);
typedef union _MonoError MonoError;

/*
 * icall.c defines a lot of icalls as static, to avoid having to add prototypes for
 * them, just don't include any mono headers and emit dummy prototypes.
 */
// Generate prototypes.
#define ICALL_TYPE(id,name,first) /* nothing */
#define ICALL(id,name,func) /* nothing */
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL

#undef HANDLES
#undef HANDLES_MAYBE

#define HANDLES(id, name, func, ...) 	 	 ICALL (id, name, func ## _raw)
#define HANDLES_MAYBE(cond, id, name, func, ...) ICALL (id, name, func ## _raw)

// Generate Icall_ constants
enum {
#define ICALL_TYPE(id,name,first)
#define ICALL(id,name,func) Icall_ ## id,
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
	Icall_last
};

enum {
#define ICALL_TYPE(id,name,first) Icall_type_ ## id,
#define ICALL(id,name,func)
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
	Icall_type_num
};

typedef struct {
	guint16 first_icall;
} IcallTypeDesc;

static const IcallTypeDesc icall_type_descs [] = {
#define ICALL_TYPE(id,name,firstic) {(Icall_ ## firstic)},
#define ICALL(id,name,func)
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
	{Icall_last}
};

#define icall_desc_num_icalls(desc) ((desc) [1].first_icall - (desc) [0].first_icall)

// This, instead of an array of pointers, to optimize away a pointer and a relocation per string.

#define MSGSTRFIELD(line) MSGSTRFIELD1(line)
#define MSGSTRFIELD1(line) str##line

static const struct msgstrtn_t {
#define ICALL_TYPE(id,name,first) char MSGSTRFIELD(__LINE__) [sizeof (name)];
#define ICALL(id,name,func)
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
} icall_type_names_str = {
#define ICALL_TYPE(id,name,first) (name),
#define ICALL(id,name,func)
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
};

static const guint16 icall_type_names_idx [] = {
#define ICALL_TYPE(id,name,first) (offsetof (struct msgstrtn_t, MSGSTRFIELD(__LINE__))),
#define ICALL(id,name,func)
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
};

#define icall_type_name_get(id) ((const char*)&icall_type_names_str + icall_type_names_idx [(id)])

static const struct msgstr_t {
#define ICALL_TYPE(id,name,first)
#define ICALL(id,name,func) char MSGSTRFIELD(__LINE__) [sizeof (name)];
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
} icall_names_str = {
#define ICALL_TYPE(id,name,first)
#define ICALL(id,name,func) (name),
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
};

static const guint16 icall_names_idx [] = {
#define ICALL_TYPE(id,name,first)
#define ICALL(id,name,func) (offsetof (struct msgstr_t, MSGSTRFIELD(__LINE__))),
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
};

#define icall_name_get(id) ((const char*)&icall_names_str + icall_names_idx [(id)])

static const gconstpointer icall_functions [] = {
#define ICALL_TYPE(id,name,first)
#define ICALL(id,name,func) ((gpointer)(func)),
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
	NULL
};

#ifdef ENABLE_ICALL_SYMBOL_MAP

static const gconstpointer icall_symbols [] = {
#define ICALL_TYPE(id,name,first)
#define ICALL(id,name,func) #func,
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
	NULL
};

#endif // ENABLE_ICALL_SYMBOL_MAP

#undef HANDLES
#undef HANDLES_MAYBE
#undef NOHANDLES

static const guchar icall_uses_handles [] = {
#define ICALL_TYPE(id,name,first)
#define ICALL(id,name,func) 0,
#define HANDLES(...) 1,
#define HANDLES_MAYBE(...) 1,
#define NOHANDLES(inner) 0,
#include "metadata/icall-def.h"
#undef ICALL_TYPE
#undef ICALL
};

#undef HANDLES
#undef HANDLES_MAYBE
#undef NOHANDLES

static int
compare_method_imap (const void *key, const void *elem)
{
	const char* method_name = (const char*)&icall_names_str + (*(guint16*)elem);
	return strcmp ((const char*)key, method_name);
}

static gsize
find_slot_icall (const IcallTypeDesc *imap, const char *name)
{
	const guint16 *nameslot = (const guint16 *)mono_binary_search (name, icall_names_idx + imap->first_icall, icall_desc_num_icalls (imap), sizeof (icall_names_idx [0]), compare_method_imap);
	if (!nameslot)
		return -1;
	return (nameslot - &icall_names_idx [0]);
}

static gboolean
find_uses_handles_icall (const IcallTypeDesc *imap, const char *name)
{
	gsize slotnum = find_slot_icall (imap, name);
	if (slotnum == -1)
		return FALSE;
	return (gboolean)icall_uses_handles [slotnum];
}

static gpointer
find_method_icall (const IcallTypeDesc *imap, const char *name)
{
	gsize slotnum = find_slot_icall (imap, name);
	if (slotnum == -1)
		return NULL;
	return (gpointer)icall_functions [slotnum];
}

static int
compare_class_imap (const void *key, const void *elem)
{
	const char* class_name = (const char*)&icall_type_names_str + (*(guint16*)elem);
	return strcmp ((const char*)key, class_name);
}

static const IcallTypeDesc*
find_class_icalls (const char *name)
{
	const guint16 *nameslot = (const guint16 *)mono_binary_search (name, icall_type_names_idx, Icall_type_num, sizeof (icall_type_names_idx [0]), compare_class_imap);
	if (!nameslot)
		return NULL;
	return &icall_type_descs [nameslot - &icall_type_names_idx [0]];
}

static gpointer
icall_table_lookup (char *classname, char *methodname, char *sigstart, gboolean *uses_handles)
{
	const IcallTypeDesc *imap = NULL;
	gpointer res;

	imap = find_class_icalls (classname);

	/* it wasn't found in the static call tables */
	if (!imap) {
		if (uses_handles)
			*uses_handles = FALSE;
		return NULL;
	}
	res = find_method_icall (imap, methodname);
	if (res) {
		if (uses_handles)
			*uses_handles = find_uses_handles_icall (imap, methodname);
		return res;
	}
	/* try _with_ signature */
	*sigstart = '(';
	res = find_method_icall (imap, methodname);
	if (res) {
		if (uses_handles)
			*uses_handles = find_uses_handles_icall (imap, methodname);
		return res;
	}
	return NULL;
}

#ifdef ENABLE_ICALL_SYMBOL_MAP
static int
func_cmp (gconstpointer key, gconstpointer p)
{
	return (gsize)key - (gsize)*(gsize*)p;
}
#endif

static const char*
lookup_icall_symbol (gpointer func)
{
#ifdef ENABLE_ICALL_SYMBOL_MAP
	int i;
	gpointer slot;
	static gconstpointer *functions_sorted;
	static const char**symbols_sorted;
	static gboolean inited;

	if (!inited) {
		gboolean changed;

		functions_sorted = g_malloc (G_N_ELEMENTS (icall_functions) * sizeof (gpointer));
		memcpy (functions_sorted, icall_functions, G_N_ELEMENTS (icall_functions) * sizeof (gpointer));
		symbols_sorted = g_malloc (G_N_ELEMENTS (icall_functions) * sizeof (gpointer));
		memcpy (symbols_sorted, icall_symbols, G_N_ELEMENTS (icall_functions) * sizeof (gpointer));
		/* Bubble sort the two arrays */
		changed = TRUE;
		while (changed) {
			changed = FALSE;
			for (i = 0; i < G_N_ELEMENTS (icall_functions) - 1; ++i) {
				if (functions_sorted [i] > functions_sorted [i + 1]) {
					gconstpointer tmp;

					tmp = functions_sorted [i];
					functions_sorted [i] = functions_sorted [i + 1];
					functions_sorted [i + 1] = tmp;
					tmp = symbols_sorted [i];
					symbols_sorted [i] = symbols_sorted [i + 1];
					symbols_sorted [i + 1] = (const char*)tmp;
					changed = TRUE;
				}
			}
		}
		inited = TRUE;
	}

	slot = mono_binary_search (func, functions_sorted, G_N_ELEMENTS (icall_functions), sizeof (gpointer), func_cmp);
	if (!slot)
		return NULL;
	g_assert (slot);
	return symbols_sorted [(gpointer*)slot - (gpointer*)functions_sorted];
#else
	fprintf (stderr, "icall symbol maps not enabled, pass --enable-icall-symbol-map to configure.\n");
	g_assert_not_reached ();
	return NULL;
#endif
}

void
mono_icall_table_init (void)
{
	int i = 0;

	/* check that tables are sorted: disable in release */
	if (TRUE) {
		int j;
		const char *prev_class = NULL;
		const char *prev_method;
		
		for (i = 0; i < Icall_type_num; ++i) {
			const IcallTypeDesc *desc;
			int num_icalls;
			prev_method = NULL;
			if (prev_class && strcmp (prev_class, icall_type_name_get (i)) >= 0)
				g_print ("class %s should come before class %s\n", icall_type_name_get (i), prev_class);
			prev_class = icall_type_name_get (i);
			desc = &icall_type_descs [i];
			num_icalls = icall_desc_num_icalls (desc);
			/*g_print ("class %s has %d icalls starting at %d\n", prev_class, num_icalls, desc->first_icall);*/
			for (j = 0; j < num_icalls; ++j) {
				const char *methodn = icall_name_get (desc->first_icall + j);
				if (prev_method && strcmp (prev_method, methodn) >= 0)
					g_print ("method %s should come before method %s\n", methodn, prev_method);
				prev_method = methodn;
			}
		}
	}

	MonoIcallTableCallbacks cb;
	memset (&cb, 0, sizeof (MonoIcallTableCallbacks));
	cb.version = MONO_ICALL_TABLE_CALLBACKS_VERSION;
	cb.lookup = icall_table_lookup;
	cb.lookup_icall_symbol = lookup_icall_symbol;

	mono_install_icall_table_callbacks (&cb);
}
