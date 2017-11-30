public static class MonoExt
{ 
	// rol on x86, amd64
	// TBD otherwise
	public static uint RotateLeft(uint a, int b)
	{
		return (a << b ) | (a >> (32 - b));
	}

	// ror on x86, amd64
	// TBD otherwise
	public static uint RotateRight(uint a, int b)
	{
		return (a >> b) | (a << (32 - b));
	} 

	// rol on amd64
	// more instructions on x86
	// TBD otherwise
	public static ulong RotateLeft(ulong a, int b)
	{
		return (a << b) | (a >> (64 - b));
	}

	// ror on amd64
	// more instructions on x86
	// TBD otherwise
	public static ulong RotateRight(ulong a, int b)
	{
		return (a >> b) | (a << (64 - b));
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
