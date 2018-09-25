/**
 * \file
 */

#ifndef __MONO_SIGNBIT_H__
#define __MONO_SIGNBIT_H__

#include <math.h>
#include <mono/utils/mono-publib.h>

#ifdef HAVE_SIGNBIT
#define mono_signbit signbit
#else
#define mono_signbit(x) (sizeof (x) == sizeof (float) ? mono_signbit_float (x) : mono_signbit_double (x))

MONO_API int
mono_signbit_double (double x);

MONO_API int
mono_signbit_float (float x);

#endif

// Instead of isfinite, isinf, isnan, etc.,
// use mono_isfininite, mono_isinf, mono_isnan, etc.
// These functions are implemented in C in order to avoid
// a C++ runtime dependency and for more portable binding
// from C++, esp. across Android versions/architectures.
// WebAssembly, and Win32/gcc.
// See https://github.com/mono/mono/pull/10701 for what
// this systematically and more portably cleans up.

#if defined (__cplusplus) || defined (MONO_MATH_DECLARE_ALL)

// These declarations are usually hidden, in order
// to encourage using the overloaded names instead of
// the type-specific names.

G_EXTERN_C    int mono_isfinite_float (float);
G_EXTERN_C    int mono_isfinite_double (double);
G_EXTERN_C    int mono_isinf_float (float);
G_EXTERN_C    int mono_isinf_double (double);
G_EXTERN_C    int mono_isnan_float (float);
G_EXTERN_C    int mono_isnan_double (double);
G_EXTERN_C    int mono_isnormal_float (float);
G_EXTERN_C    int mono_isnormal_double (double);
G_EXTERN_C    int mono_isunordered_float (float, float);
G_EXTERN_C    int mono_isunordered_double (double, double);
G_EXTERN_C  float mono_trunc_float (float);
G_EXTERN_C double mono_trunc_double (double);

#endif

#ifdef __cplusplus

inline    int mono_isfinite (float a)               { return mono_isfinite_float (a); }
inline    int mono_isfinite (double a)              { return mono_isfinite_double (a); }
inline    int mono_isinf (float a)                  { return mono_isinf_float (a); }
inline    int mono_isinf (double a)                 { return mono_isinf_double (a); }
inline    int mono_isnan (float a)                  { return mono_isnan_float (a); }
inline    int mono_isnan (double a)                 { return mono_isnan_double (a); }
inline    int mono_isnormal (float a)               { return mono_isnormal_float (a); }
inline    int mono_isnormal (double a)              { return mono_isnormal_double (a); }
inline    int mono_isunordered (float a, float b)   { return mono_isunordered_float (a, b); }
inline    int mono_isunordered (double a, double b) { return mono_isunordered_double (a, b); }
inline  float mono_trunc (float a)                  { return mono_trunc_float (a); }
inline double mono_trunc (double a)                 { return mono_trunc_double (a); }

#else

// Direct macros for C.
// This will also work for many C++ platforms, i.e. other than Android and WebAssembly and Win32/gcc.
#define mono_isfinite        isfinite
#define mono_isinf           isinf
#define mono_isnan           isnan
#define mono_isnormal        isnormal
#define mono_isunordered     isunordered
#define mono_trunc           trunc

#endif

#endif
