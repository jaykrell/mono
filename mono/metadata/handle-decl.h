/**
 * \file
 * Handle to object in native code
 *
 * Authors:
 *  - Ludovic Henry <ludovic@xamarin.com>
 *  - Aleksey Klieger <aleksey.klieger@xamarin.com>
 *  - Rodrigo Kumpera <kumpera@xamarin.com>
 *
 * Copyright 2016 Dot net foundation.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

#ifndef __MONO_HANDLE_DECL_H__
#define __MONO_HANDLE_DECL_H__

#include <config.h>
#include <glib.h>
#include <mono/metadata/object-forward.h>
#include <mono/utils/mono-compiler.h>

// Type-safe handles are a struct with a pointer to pointer.
// The only operations allowed on them are the functions/macros in this file, and assignment
// from same handle type to same handle type.
//
// Type-unsafe handles formerly were a pointer to a struct with a pointer.
// The same level of indirection either way.
// Besides the type-safe operations, they could also:
//  1. compared to NULL, instead of only MONO_HANDLE_IS_NULL
//  2. assigned from NULL, instead of only a handle
//  3. MONO_HANDLE_NEW (T) from anything, instead of only a T*
//  4. MONO_HANDLE_CAST from anything, instead of only another handle type
//  5. assigned from any void*, at least in C
//  6. Cast from any handle type to any handle type, without using MONO_HANDLE_CAST.
//  7. Cast from any handle type to any pointer type and vice versa, such as incorrect unboxing.
//  8. mono_object_class (handle), instead of mono_handle_class
//
// None of those operations were likely intended.
//
// marshal-ilgen.c does not know how to marshal type safe handles.
// It passes/accepts type-unsafe handles and generated wrappers in C convert.

/*
Handle macros/functions
*/

#define TYPED_HANDLE_PAYLOAD_NAME(TYPE) TYPE ## HandlePayload
#define TYPED_HANDLE_NAME(TYPE) TYPE ## Handle
#define TYPED_OUT_HANDLE_NAME(TYPE) TYPE ## HandleOut
#define TYPED_IN_OUT_HANDLE_NAME(TYPE) TYPE ## HandleInOut

// For internal purposes. Do not use.
#define TYPED_HANDLE_PAYLOAD_NAME_UNSAFE(TYPE)	TYPE ## HandlePayloadUnsafe
#define TYPED_HANDLE_NAME_UNSAFE(TYPE)  	TYPE ## HandleUnsafe
#define TYPED_OUT_HANDLE_NAME_UNSAFE(TYPE) 	TYPE ## OutHandleUnsafe
#define TYPED_IN_OUT_HANDLE_NAME_UNSAFE(TYPE) 	TYPE ## InOutHandleUnsafe

// internal helpers:
#define MONO_HANDLE_CAST_FOR(type) mono_handle_cast_##type
#define MONO_HANDLE_TYPECHECK_FOR(type) mono_handle_typecheck_##type

/*
 *   Expands to a decl for handles to SomeType and to an internal payload struct.
 *
 * For example, TYPED_HANDLE_DECL(MonoObject) (see below) expands to:
 *
 * typedef struct {
 *   MonoObject **__raw;
 * } MonoObjectHandlePayload,
 *   MonoObjectHandle,
 *   MonoObjectHandleOut;
 *
 * Internal helper functions are also generated.
 */
#ifdef __cplusplus
#define MONO_IF_CPLUSPLUS(x) x
#else
#define MONO_IF_CPLUSPLUS(x) /* nothing */
#endif

#define TYPED_HANDLE_DECL(TYPE)							\
										\
	typedef struct { TYPE *__raw; } TYPED_HANDLE_PAYLOAD_NAME_UNSAFE (TYPE), \
		*TYPED_HANDLE_NAME_UNSAFE (TYPE), 				\
		*TYPED_OUT_HANDLE_NAME_UNSAFE (TYPE), 				\
		*TYPED_IN_OUT_HANDLE_NAME_UNSAFE (TYPE); 			\
										\
	typedef struct {							\
		MONO_IF_CPLUSPLUS (						\
			MONO_ALWAYS_INLINE					\
			TYPE * GetRaw () { return __raw ? *__raw : NULL; }	\
		)								\
		TYPE **__raw;							\
	} TYPED_HANDLE_PAYLOAD_NAME (TYPE),					\
	  TYPED_HANDLE_NAME (TYPE),						\
	  TYPED_OUT_HANDLE_NAME (TYPE),						\
	  TYPED_IN_OUT_HANDLE_NAME (TYPE);					\
/* Do not call these functions directly. Use MONO_HANDLE_NEW and MONO_HANDLE_CAST. */ \
/* Another way to do this involved casting mono_handle_new function to a different type. */ \
static inline MONO_ALWAYS_INLINE TYPED_HANDLE_NAME (TYPE) 	\
MONO_HANDLE_CAST_FOR (TYPE) (gpointer a)			\
{								\
	TYPED_HANDLE_NAME (TYPE) b = { (TYPE**)a };		\
	return b;						\
}								\
static inline MONO_ALWAYS_INLINE MonoObject* 			\
MONO_HANDLE_TYPECHECK_FOR (TYPE) (TYPE *a)			\
{								\
	return (MonoObject*)a;					\
}								\
/* Out/InOut synonyms for icall-def.h HANDLES () */		\
static inline MONO_ALWAYS_INLINE TYPED_HANDLE_NAME (TYPE) 	\
MONO_HANDLE_CAST_FOR (TYPE##Out) (gpointer a)			\
{								\
	return MONO_HANDLE_CAST_FOR (TYPE) (a);			\
}								\
static inline MONO_ALWAYS_INLINE MonoObject* 			\
MONO_HANDLE_TYPECHECK_FOR (TYPE##Out) (TYPE *a)			\
{								\
	return MONO_HANDLE_TYPECHECK_FOR (TYPE) (a);		\
}								\
static inline MONO_ALWAYS_INLINE TYPED_HANDLE_NAME (TYPE) 	\
MONO_HANDLE_CAST_FOR (TYPE##InOut) (gpointer a)			\
{								\
	return MONO_HANDLE_CAST_FOR (TYPE) (a);			\
}								\
static inline MONO_ALWAYS_INLINE MonoObject* 			\
MONO_HANDLE_TYPECHECK_FOR (TYPE##InOut) (TYPE *a)		\
{								\
	return MONO_HANDLE_TYPECHECK_FOR (TYPE) (a);		\
}

/*
 * TYPED_VALUE_HANDLE_DECL(SomeType):
 *   Expands to a decl for handles to SomeType (which is a managed valuetype (likely a struct) of some sort) and to an internal payload struct.
 * It is currently identical to TYPED_HANDLE_DECL (valuetypes vs. referencetypes).
 */
#define TYPED_VALUE_HANDLE_DECL(TYPE) TYPED_HANDLE_DECL(TYPE)

#include "object-forward.h"

/* Baked typed handles we all want */
TYPED_HANDLE_DECL (MonoAppContext);
TYPED_HANDLE_DECL (MonoArray);
TYPED_HANDLE_DECL (MonoException);
TYPED_HANDLE_DECL (MonoObject);
TYPED_HANDLE_DECL (MonoString);

/* Safely access System.Reflection.Module from native code */
TYPED_HANDLE_DECL (MonoReflectionModule);

#endif /* __MONO_HANDLE_DECL_H__ */
