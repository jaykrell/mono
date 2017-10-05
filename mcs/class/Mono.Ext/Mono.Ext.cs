public static class MonoExt
{ 
	// rol on x86, amd64
	// TBD otherwise
	public static uint RotateLeft(uint value, int count)
	{
		return (value << count ) | (value >> (32 - count));
	}

	// ror on x86, amd64
	// TBD otherwise
	public static uint RotateRight(uint value, int count)
	{
		return (value >> count) | (value << (32 - count));
	} 

	// rol on amd64
	// more instructions on x86
	// TBD otherwise
	public static ulong RotateLeft(ulong value, int count)
	{
		return (value << count) | (value >> (64 - count));
	}

	// ror on amd64
	// more instructions on x86
	// TBD otherwise
	public static ulong RotateRight(ulong value, int count)
	{
		return (value >> count) | (value << (64 - count));
	}

	// rdtsc on x86, amd64
	// TBD otherwise
	public static ulong ReadTimeStampCounter()
	{
		return 0;
	}

	// int3 on x86, amd64
	// TBD otherwise
	public static void DebugBreak()
	{
	}
}
