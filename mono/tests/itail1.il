.assembly extern mscorlib { }
.assembly tailcall1 { }
.class A extends [mscorlib]System.Object {
.method public hidebysig specialname rtspecialname instance void .ctor() cil managed
{
.maxstack 8
ldarg.0
call instance void [mscorlib]System.Object::.ctor()
ret
}
.method public hidebysig static int32 itail1() cil managed noinlining {
.entrypoint
.maxstack 4
.locals init (uint8 V_0, int32 V_1)
nop
newobj instance void A::.ctor()
ldloca.s V_0
conv.u
conv.u8
ldc.i4.0
conv.i8
ldc.i4.2
call instance int32 A::itail2(int64, int64, int32)
stloc.1
br.s IL_0015
IL_0015: ldloc.1
ret
}
.method private hidebysig instance int32 itail2(int64 root_stack, int64 diff_stack, int32 counter) cil managed noinlining {
.maxstack 5
.locals init (uint8 V_0, bool V_1, int32 V_2)
nop
ldarg.3
ldc.i4.0
cgt
stloc.1
ldloc.1
brfalse.s IL_001c
ldarg.0
ldarg.1
ldloca.s V_0
conv.u
conv.u8
ldarg.1
sub
ldarg.3
ldc.i4.1
sub
tail. call instance int32 A::itail2(int64, int64, int32)
ret
stloc.2
br.s IL_002e
IL_001c: ldarg.1
ldloca.s V_0
conv.u
ldarg.2
ldstr "itail1"
call int32 A::check(int64, uint8*, int64, string)
stloc.2
br.s IL_002e
IL_002e: ldloc.2
ret
}
.method private hidebysig static int32 check(uint8* root_stack, uint8* local, int64 diff_stack, string name) cil managed noinlining {
.maxstack 4
.locals init (bool V_0, int32 V_1)
nop
ldarg.1
ldarg.0
sub
ldc.i4.1
div
conv.i8
ldarg.2
ceq
ldc.i4.0
ceq
stloc.0
ldloc.0
brfalse.s IL_0032
nop
ldstr "{0} failure {1}"
ldarg.3
ldarg.0
ldarg.1
sub
ldc.i4.1
div
conv.i8
box [mscorlib]System.Int64
call string [mscorlib]System.String::Format(string, object, object)
call void [mscorlib]System.Console::WriteLine(string)
nop
ldc.i4.1
stloc.1
br.s IL_0047
IL_0032: ldstr "{0} success"
ldarg.3
call string [mscorlib]System.String::Format(string, object)
call void [mscorlib]System.Console::WriteLine(string)
nop
ldc.i4.0
stloc.1
br.s IL_0047
IL_0047: ldloc.1
ret
}
.method private hidebysig static int32 check(int64 root_stack, uint8* local, int64 diff_stack, string name) cil managed noinlining {
.maxstack 4
.locals init (int32 V_0)
nop
ldarg.0
conv.u
ldarg.1
ldarg.2
ldarg.3
call int32 A::check(uint8*, uint8*, int64, string)
stloc.0
br.s IL_000e
IL_000e: ldloc.0
ret
}
}
