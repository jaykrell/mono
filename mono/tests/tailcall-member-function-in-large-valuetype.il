/*
// To fix this, use a range check in the call and dynamically
// decide tailcall or not.

using System;
using System.Runtime.CompilerServices;
using static System.Runtime.CompilerServices.MethodImplOptions;

unsafe public
// change to class and it works
struct
LargeValueType
{
	public int a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z;

	[MethodImpl (NoInlining)]
	static void check (long stack1, long stack2)
	{
// NOTE: This is wierd in order to be later hand edited (removed) in the IL.
		if (stack1 != 0)
			return;
		if (stack1 == stack2)
			return;
		Console.WriteLine ("tailcall failure {0} {1}", stack1, stack2);
		Environment.Exit (1);
	}

	[MethodImpl (NoInlining)]
	public void Method1 (long depth = 100, long stack = 0)
	{
		int local;
		if (depth > 0) {
			Method2 (depth - 1, (long)&local);
			return;
		}
		check ((long)&local, stack);
	}

	[MethodImpl (NoInlining)]
	void Method2 (long depth = 100, long stack = 0)
	{
		int local;
		if (depth > 0) {
			Method1 (depth - 1, (long)&local);
			return;
		}
		check ((long)&local, stack);
	}
}

class B
{
	[MethodImpl (NoInlining)]
	public static void Main (string[] args)
	{
		new LargeValueType ().Method1 (999999);
	}
}
*/

.assembly extern mscorlib { }

.assembly 'tailcall-member-function-in-large-valuetype' { }

.class public sequential ansi sealed beforefieldinit LargeValueType
extends [mscorlib]System.ValueType
{
.field public int32 a
.field public int32 b
.field public int32 c
.field public int32 d
.field public int32 e
.field public int32 f
.field public int32 g
.field public int32 h
.field public int32 i
.field public int32 j
.field public int32 k
.field public int32 l
.field public int32 m
.field public int32 n
.field public int32 o
.field public int32 p
.field public int32 q
.field public int32 r
.field public int32 s
.field public int32 t
.field public int32 u
.field public int32 v
.field public int32 w
.field public int32 x
.field public int32 y
.field public int32 z

.method private static hidebysig default void check (int64 stack1, int64 stack2) noinlining
{
/*
ldarg.0
brfalse.s IL_0004
ret
*/

IL_0004:
ldarg.0
ldarg.1
bne.un.s IL_0009
ret

IL_0009:
ldstr "tailcall failure {0} {1}"
ldarg.0
box [mscorlib]System.Int64
ldarg.1
box [mscorlib]System.Int64
call void class [mscorlib]System.Console::WriteLine(string, object, object)
ldc.i4.1
tail.
call void class [mscorlib]System.Environment::Exit(int32)
ret
}

.method public hidebysig instance default void Method1 (int64 depth, int64 stack) noinlining
{
.locals init ( int32 V_0)

ldarg.1
ldc.i4.0
conv.i8
ble.s IL_0014

ldarg.0
ldarg.1
ldc.i4.1
conv.i8
sub
ldloca.s 0
conv.u
conv.u8
tail.
call instance void valuetype LargeValueType::Method2(int64, int64)
ret

IL_0014:
ldloca.s 0
conv.u
conv.u8
ldarg.2
tail.
call void valuetype LargeValueType::check(int64, int64)
ret
}

.method private hidebysig instance default void Method2 (int64 depth, int64 stack) noinlining
{
.locals init ( int32 V_0)

ldarg.1
ldc.i4.0
conv.i8
ble.s IL_0014

ldarg.0
ldarg.1
ldc.i4.1
conv.i8
sub
ldloca.s 0
conv.u
conv.u8
tail.
call instance void valuetype LargeValueType::Method1(int64, int64)
ret

IL_0014:
ldloca.s 0
conv.u
conv.u8
ldarg.2
tail.
call void valuetype LargeValueType::check(int64, int64)
ret
}
}
.class private auto ansi beforefieldinit B
extends [mscorlib]System.Object
{

.method public static hidebysig default void Main (string[] args) noinlining
{
.entrypoint
.locals init ( valuetype LargeValueType V_0)

ldloca.s 0
initobj LargeValueType
ldloc.0
stloc.0
ldloca.s 0
ldc.i4 999999
conv.i8
ldc.i4.0
conv.i8
tail.
call instance void valuetype LargeValueType::Method1(int64, int64)
ret
}

.method public hidebysig specialname rtspecialname instance default void '.ctor' ()
{
ldarg.0
tail.
call instance void object::'.ctor'()
ret
}
}
