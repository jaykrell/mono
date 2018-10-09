using Mono.Cecil;

public class A
{
       public static void Main (string [] args)
       {
		foreach (var arg in args)
		{
			ModuleDefinition module = ModuleDefinition.ReadModule (arg);
			foreach (var type in module.Types)
			{
				foreach (var m in type.Methods)
				{
					if (!m.IsInternalCall () || !m.IsRuntime ())
						continue;
					System.Console.WriteLine ($"{arg} {type.FullName}.{m.Name} Attributes:{m.Attributes} ImplAttributes:{m.ImplAttributes}");
				}
			}
		}
	}
}
