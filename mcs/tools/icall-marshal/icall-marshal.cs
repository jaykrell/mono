using Mono.Cecil;

public class A
{
	public static void Main (string [] args)
	{
		ModuleDefinition module = ModuleDefinition.ReadModule (args[0]);
		foreach (var type in module.Types) {
			foreach (var m in type.methods) {
//				System.Console.WriteLine (type.FullName + "." + m.Name);
			}
		}
	}
}
