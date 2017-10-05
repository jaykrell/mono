import test1data;
public class test1
{
   static uint RotateLeft(uint value, int count)
   {
        return (value << count) | (value >> (32 - count));
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

    void Assert(bool value, string name)
    {
        if (value) return;
        System.Console.WriteLine("assertion failure:{0}", name);
        MonoExt.DebugBreak();
    }

    static void test1(ulong value, int count)
    {
        uint c = 0;
        uint d = 0;
        ulong e = 0;
        ulong f = 0;
        uint g = 0;
        uint h = 0;
        ulong i = 0;
        ulong j = 0;

        MonoExt.DebugBreak();
        double t1 = (double)MonoExt.ReadTimeStampCounter();

        int ii;
        int iterations = 1000 * 10;

        for (ii = 0; ii < iterations; ++ii)
        {
            c = RotateLeft((uint)value, count);
            d = RotateRight((uint)value, count);
            e = RotateLeft((ulong)value, count);
            f = RotateRight((ulong)value, count);
        }

        double t2 = (double)MonoExt.ReadTimeStampCounter();

        for (ii = 0; ii < iterations; ++ii)
        {
            g = MonoExt.RotateLeft((uint)value, count);
            h = MonoExt.RotateRight((uint)value, count);
            i = MonoExt.RotateLeft((ulong)value, count);
            j = MonoExt.RotateRight((ulong)value, count);
        }

        ulong t3 = MonoExt.ReadTimeStampCounter();

        double slow = t2 - t1;
        double fast = t3 - t2;

        System.Console.WriteLine("{0} {1} {2}", slow, fast, (slow - fast) / slow);

        System.Console.WriteLine("{0:X} rol32 {1:X} = {2:X}", value, count, c);
        System.Console.WriteLine("{0:X} ror32 {1:X} = {2:X}", value, count, d);
        System.Console.WriteLine("{0:X} rol64 {1:X} = {2:X}", value, count, e);
        System.Console.WriteLine("{0:X} ror64 {1:X} = {2:X}", value, count, f);

        System.Console.WriteLine("{0:X} rol32 {1:X} = {2:X}", value, count, g);
        System.Console.WriteLine("{0:X} ror32 {1:X} = {2:X}", value, count, h);
        System.Console.WriteLine("{0:X} rol64 {1:X} = {2:X}", value, count, i);
        System.Console.WriteLine("{0:X} ror64 {1:X} = {2:X}", value, count, j);
    }


    static void Main(string[] args)
    {
        System.Console.WriteLine("Hello World!");
 
        if (args.Length > 1)
        {
            ulong value = ulong.Parse(args[0]);
            int count = int.Parse(args[1]);
            test(value, count);
            return;
        }
        for (ulong value = 1; value <= 4; ++value)
        {
            for (int count = 1; count <= 4; ++count)
            {
                test(value, count);
            }
        }
        for (long value = -1; value <= 16; ++value)
        {
            for (int count = 0; count <= 16; ++count)
            {
            //  test(value, count);
            }
        }
    }
}
