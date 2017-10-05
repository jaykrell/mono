using System;

public class MonoExtTests
{
    const int k = 6; // number of array elements per conceptual struct

    // These serve to take away constant-ness, but are usually zero.
    // They also serve as to where to store results, but should be restored to zero.
    public static ulong not_constant_ul;
    public static volatile int not_constant_i;
    public static volatile uint not_constant_ui;

    static int check(int first_error, string op, int index, ulong value, int count, ulong expected, ulong actual)
    {
        if (actual == expected)
            return first_error;
        System.Console.WriteLine("error {0} {1} {2:X} {3:X} expected:{4:X} actual:{5:X}", op, index / k, value, count, expected, actual);
        //MonoExt.DebugBreak();
        return index;
    }

    // duplicates of Mono.Ext for JIT pattern matching
    static uint RotateLeft(uint value, int count)
    {
        return (value << count ) | (value >> (32 - count));
    }

    static uint RotateRight(uint value, int count)
    {
        return (value >> count) | (value << (32 - count));
    } 

    static ulong RotateLeft(ulong value, int count)
    {
        return (value << count) | (value >> (64 - count));
    }

    static ulong RotateRight(ulong value, int count)
    {
        return (value >> count) | (value << (64 - count));
    }

    // TODO JIT pattern match
    // duplicate of test_0_check_intrinsics but with local functions instead of MonoExt.
    //
    // The underlying feature is not yet present.

    public static int test_0_check_jit_optimization()
    {
        //System.Console.WriteLine("test_check_jit_optimization");
        int error = 0;

        //MonoExt.DebugBreak();

        // Check some with constants -- one, the other, both.
        // Generated tests are >=1, so some manual tests are <0.

        error |= check(error, "rol32i", -101 * k, 1, 2, 4, RotateLeft(1 + not_constant_ui, 2));
        error |= check(error, "rol32r", -102 * k, 3, 4, (uint)0xC0000000 >> 2, RotateRight(3 + not_constant_ui, 4));
        error |= check(error, "rol64i", -103 * k, 5, 6, 5 << 6, RotateLeft(5UL + not_constant_ui, 6));
        error |= check(error, "ror64i", -104 * k, 7, 8, 0x700000000000000, RotateRight(7UL + not_constant_ui, 8));

        error |= check(error, "rol32i", -105 * k, 1, 2, 4, RotateLeft(1, 2));
        error |= check(error, "rol32r", -106 * k, 3, 4, 0xC0000000 >> 2, RotateRight(3, 4));
        error |= check(error, "rol64i", -107 * k, 5, 6, 5 << 6, RotateLeft(5UL, 6));
        error |= check(error, "ror64i", -108 * k, 7, 8, 0x700000000000000, RotateRight(7UL, 8));

        error |= check(error, "rol32i", -109 * k, 1, 2, 4, RotateLeft(1, 2 + not_constant_i));
        error |= check(error, "rol32r", -110 * k, 3, 4, 0xC0000000 >> 2, RotateRight(3, 4 + not_constant_i));
        error |= check(error, "rol64i", -111 * k, 5, 6, 5 << 6, RotateLeft(5UL, 6 + not_constant_i));
        error |= check(error, "ror64i", -112 * k, 7, 8, 0x700000000000000, RotateRight(7UL, 8 + not_constant_i));

        var data = MonoExtTestData.data;
        if (k != MonoExtTestData.k)
        {
            System.Console.WriteLine("test vs. testdata schema mismatch {0} {1}", k, MonoExtTestData.k);
            return -1;
        }
        for (int i = k; i < data.Length; i += k) // skip the first so 0 is success
        {
            var value = (ulong)data[i];
            var count = (int)data[i + 1];
            error |= check(error, "rol32", i, (uint)value, count, (ulong)(uint)data[i + 2], RotateLeft((uint)value, count));
            error |= check(error, "ror32", i, (uint)value, count, (ulong)(uint)data[i + 3], RotateRight((uint)value, count));
            error |= check(error, "rol64", i, value, count, (ulong)data[i + 4], RotateLeft(value, count));
            error |= check(error, "ror64", i, value, count, (ulong)data[i + 5], RotateRight(value, count));
        }
        return error / k;
    }

    public static int test_0_check_intrinsics()
    {
        //System.Console.WriteLine("test_check_intrinsics");
        int error = 0;

        //MonoExt.DebugBreak();

        // Check some with constants -- one, the other, both.
        // Generated tests are >=1, so some manual tests are <0.

        error |= check(error, "rol32i", -201 * k, 1, 2, 4, MonoExt.RotateLeft(1 + not_constant_ui, 2));
        error |= check(error, "rol32r", -202 * k, 3, 4, 0xC0000000 >> 2, MonoExt.RotateRight(3 + not_constant_ui, 4));
        error |= check(error, "rol64i", -203 * k, 5, 6, 5 << 6, MonoExt.RotateLeft(5UL + not_constant_ui, 6));
        error |= check(error, "ror64i", -204 * k, 7, 8, 0x700000000000000, MonoExt.RotateRight(7UL + not_constant_ui, 8));

        error |= check(error, "rol32i", -205 * k, 1, 2, 4, MonoExt.RotateLeft(1, 2));
        error |= check(error, "rol32r", -206 * k, 3, 4, 0xC0000000 >> 2, MonoExt.RotateRight(3, 4));
        error |= check(error, "rol64i", -207 * k, 5, 6, 5 << 6, MonoExt.RotateLeft(5UL, 6));
        error |= check(error, "ror64i", -208 * k, 7, 8, 0x700000000000000, MonoExt.RotateRight(7UL, 8));

        error |= check(error, "rol32i", -209 * k, 1, 2, 4, MonoExt.RotateLeft(1, 2 + not_constant_i));
        error |= check(error, "rol32r", -210 * k, 3, 4, 0xC0000000 >> 2, MonoExt.RotateRight(3, 4 + not_constant_i));
        error |= check(error, "rol64i", -211 * k, 5, 6, 5 << 6, MonoExt.RotateLeft(5UL, 6 + not_constant_i));
        error |= check(error, "ror64i", -212 * k, 7, 8, 0x700000000000000, MonoExt.RotateRight(7UL, 8 + not_constant_i));

        var data = MonoExtTestData.data;
        if (k != MonoExtTestData.k)
        {
            System.Console.WriteLine("test vs. testdata schema mismatch {0} {1}", k, MonoExtTestData.k);
            return -1;
        }
        for (int i = k; i < data.Length; i += k) // skip the first so 0 is success
        {
            var value = (ulong)data[i];
            var count = (int)data[i + 1];
            error |= check(error, "rol32", i, (uint)value, count, (ulong)(uint)data[i + 2], RotateLeft((uint)value, count));
            error |= check(error, "ror32", i, (uint)value, count, (ulong)(uint)data[i + 3], RotateRight((uint)value, count));
            error |= check(error, "rol64", i, value, count, (ulong)data[i + 4], RotateLeft(value, count));
            error |= check(error, "ror64", i, value, count, (ulong)data[i + 5], RotateRight(value, count));
        }
        return error / k;
    }

    public static int test_0_disassemble_all_shift_by_constant()
    // This test always succeeds, but one should look at its disassembly
    // to determine that things are working.
    {
        //System.Console.WriteLine("test_disassemble_all_shift_by_constant");
        //MonoExt.DebugBreak();

        // Make sure constants are propagated (disassemble to verify).

        not_constant_ui = MonoExt.RotateLeft(not_constant_ui, 2);
        not_constant_ui = MonoExt.RotateRight(not_constant_ui, 4);
        not_constant_ul = MonoExt.RotateLeft(not_constant_ul, 6);
        not_constant_ul = MonoExt.RotateRight(not_constant_ul, 8);
        not_constant_ui = 0;
        not_constant_ul = 0;
        return 0;
    }

    public static int test_0_disassemble_all_constant()
    // This test always succeeds, but one should look at its disassembly
    // to determine that things are working.
    {
        //System.Console.WriteLine("test_disassemble_all_constant");
        //MonoExt.DebugBreak();
        not_constant_ul = MonoExt.ReadTimeStampCounter();

        // Make sure constants are propagated (disassemble to verify).

        not_constant_ui = MonoExt.RotateLeft(1, 2);   // 3
        not_constant_ui = MonoExt.RotateRight(3, 4);  // 0x30000000
        not_constant_ul = MonoExt.RotateLeft(5L, 6);
        not_constant_ul = MonoExt.RotateRight(7L, 8);
        not_constant_ui = 0;
        not_constant_ul = 0;
        return 0;
    }

    static int OldMain(string[] args)
    {
        int err = test_0_disassemble_all_constant();
        err = (err != 0) ? err : test_0_disassemble_all_shift_by_constant();
        err = (err != 0) ? err : test_0_check_intrinsics();
        err = (err != 0) ? err : test_0_check_jit_optimization();
        if (err != 0)
            System.Console.WriteLine("err {0}", err);
        return err;
    }

    public static int Main (String[] args) {
        return TestDriver.RunTests (typeof (MonoExtTests), args);
    }
}
