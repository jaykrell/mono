// cs0557-2.cs: Duplicate user-defined conversion in type `C'
// 
class C {
		public static bool operator != (C a, C b) 
		{
			return true;
		}
		
		public static bool operator != (C a, C b) 
		{
			return true;
		}
	
		public static bool operator == (C a, C b)
		{	return false; }		
	}





class X {
	static void Main ()
	{
	}
}
