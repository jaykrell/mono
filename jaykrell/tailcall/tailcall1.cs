/*
Unlike the CoreCLR tests:

 - portable
 - runnable
 - self-checking
   Do not crash or run forever on success or failure.
 - Catching stack overflow is an option, e.g. for negative tests, not yet used.
 - will see about portable p/invoke later for increased coverage
*/
using System;
using System.Runtime.CompilerServices;
using static System.Runtime.CompilerServices.MethodImplOptions;

unsafe class A
{
	[MethodImpl (NoInlining)]
	public static int tail1 ()
	{
		byte local;
		return new A().tail2 (&local, 0, 1);
	}

	[MethodImpl (NoInlining)]
	int tail2 (byte* root_stack, long diff_stack, int counter)
	// Every invocation should have the same address of local.
	// We cannot pass the address of local, as that defeats tailcall,
	// but we can pass its difference from an initial position.
	// The self-call here to tail2 should/must be hand edited to be tail.
	// Well, there are two cases, one requires it, one does not, we are more
	// interested in the one that requires it.
	{
		byte local;
		if (counter > 0)
			return tail2 (root_stack, &local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "tail1");
	}

	[MethodImpl (NoInlining)]
	static int check (byte* root_stack, byte* local, long diff_stack, string name)
	{
		if (local - root_stack != diff_stack)
		{
			System.Console.WriteLine ($"{name} failure {root_stack - local}");
			return 1;
		}
		System.Console.WriteLine ($"{name} success");
		return 0;
	}

	[MethodImpl (NoInlining)]
	static int check (long root_stack, byte* local, long diff_stack, string name)
	{
		return check ((byte*)root_stack, local, diff_stack, name);
	}

	// Mono does not like the pointers but is ok with integers, so make that variation.
	// The actual prohibition I believe is supposed to be against managed pointers.
	// We'll see later.

	[MethodImpl (NoInlining)]
	public static int itail1 ()
	{
		byte local;
		return new A().itail2 ((long)&local, 0, 2);
	}

	[MethodImpl (NoInlining)]
	int itail2 (long root_stack, long diff_stack, int counter)
	{
		byte local;
		if (counter > 0)
			return itail2 (root_stack, (long)&local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "itail1");
	}

	// static with stack pointer
	// s is for static
	// stack pointer is implied

	[MethodImpl (NoInlining)]
	public static int stail1 ()
	{
		byte local;
		return stail2 (&local, 0, 3);
	}

	[MethodImpl (NoInlining)]
	static int stail2 (byte* root_stack, long diff_stack, int counter)
	{
		byte local;
		if (counter > 0)
			return stail2 (root_stack, &local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "stail1");
	}

	// static-instead-of-instance
	// integer-instead-of-stackpointer (working around initial mono problem)

	[MethodImpl (NoInlining)]
	public static int sitail1 ()
	{
		byte local;
		return sitail2 ((long)&local, 0, 4);
	}

	[MethodImpl (NoInlining)]
	static int sitail2 (long root_stack, long diff_stack, int counter)
	{
		byte local;
		if (counter > 0)
			return sitail2 (root_stack, (long)&local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "sitail1");
	}

	// same things but virtual

	[MethodImpl (NoInlining)]
	public static int vtail1 ()
	{
		byte local;
		return new A().vtail2(&local, 0, 5);
	}

	[MethodImpl (NoInlining)]
	public virtual int vtail2 (byte* root_stack, long diff_stack, int counter)
	{
		byte local;
		if (counter > 0)
			return vtail2 (root_stack, &local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "vtail1");
	}

	// virtual and integer

	[MethodImpl (NoInlining)]
	public static int ivtail1 ()
	{
		byte local;
		return new A().ivtail2 ((long)&local, 0, 6);
	}

	[MethodImpl (NoInlining)]
	public virtual int ivtail2 (long root_stack, long diff_stack, int counter)
	{
		byte local;
		if (counter > 0)
			return ivtail2 (root_stack, (long)&local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "ivtail1");
	}

	// static variations -- should make no difference

	// now generics, static, member, virtual, primitive, reference, struct etc.
	// g is for generic
	// p is for primitive (int, float, etc.)
	// r is for reference (object, etc.)
	// vt is for valuetype or struct
	// v is for virtual
	// s is for static

	[MethodImplAttribute (NoInlining)]
	public static int gptail1 ()
	{
		byte local;
		return new A().gptail2<int> (&local, 0, 7);
	}

	[MethodImplAttribute (NoInlining)]
	int gptail2<T> (byte* root_stack, long diff_stack, int counter)
	{
		byte local;
		if (counter > 0)
			return gptail2<int> (root_stack, &local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "gptail1");
	}

	// taili changed to calli by perl (non-virtual non-static only for now)
	// Note that delegates are not a variation, they just use Delegate.Invoke.

	[MethodImplAttribute (NoInlining)]
	public static int taili1 ()
	{
		byte local;
		return taili2 (&local, 0, 8);
	}

	[MethodImplAttribute (NoInlining)]
	static int taili2 (byte* root_stack, long diff_stack, int counter)
	{
		byte local;
		if (counter > 0)
			return taili2 (root_stack, &local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "taili1");
	}

	[MethodImplAttribute (NoInlining)]
	public static int itaili1 ()
	{
		byte local;
		return itaili2 ((long)&local, 0, 9);
	}

	[MethodImplAttribute (NoInlining)]
	static int itaili2 (long root_stack, long diff_stack, int counter)
	{
		byte local;
		if (counter > 0)
			return itaili2 (root_stack, (long)&local - root_stack, counter - 1);
		return check (root_stack, &local, diff_stack, "itaili1");
	}

	// r is for passing a managed reference
	// To current frame is not allowed, but to other frame it is.

	[MethodImplAttribute (NoInlining)]
	public static int srtail1 ()
	{
		byte local;
		object o = new object ();
		return srtail2 (&local, 0, 10, ref o);
	}

	[MethodImplAttribute (NoInlining)]
	static int srtail2 (byte* root_stack, long diff_stack, int counter, ref object o)
	{
		byte local;
		if (counter > 0)
			return srtail2 (root_stack, &local - root_stack, counter - 1, ref o);
		return check (root_stack, &local, diff_stack, "rtail1");
	}

	[MethodImplAttribute (NoInlining)]
	public static int sirtail1 ()
	{
		byte local;
		object o = new object ();
		return sirtail2 ((long)&local, 0, 11, ref o);
	}

	[MethodImplAttribute (NoInlining)]
	static int sirtail2 (long root_stack, long diff_stack, int counter, ref object o)
	{
		byte local;
		if (counter > 0)
			return sirtail2 (root_stack, (long)&local - root_stack, counter - 1, ref o);
		return check (root_stack, &local, diff_stack, "irtail1");
	}

	[MethodImplAttribute (NoInlining)]
	public static void Main (string[] args)
	{
		Environment.Exit(tail1 () // non-virtual non-static stack-pointer
				+ itail1 () // non-virtual non-static integer-instead-of-stack-pointer
				+ stail1 ()    // static stack-pointer
				+ sitail1 ()    // stack integer-stack-pointer
				+ vtail1 ()  // virtual stack-pointer
				+ ivtail1 () // virtual integer-for-stack-pointer
				+ gptail1 () // generic-of-primitive non-static non-virtual stack-pointer
				+ taili1 () // calli
				+ itaili1 () // calli with integer stack pointer
				+ srtail1 ()  // with managed reference, but not to current frame
				+ sirtail1 () // with managed reference, but not to current frame
				// FIXME more variations -- but mono only passes one of these and desktop all so is a good start
				);
	}
}
