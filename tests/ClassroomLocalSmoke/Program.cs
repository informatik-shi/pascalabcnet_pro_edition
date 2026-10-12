using System;
using System.Collections.Generic;
using System.IO;
using VisualPascalABC.Classroom;

internal static class Program
{
    private static int Main(string[] args)
    {
        if (args.Length == 5 && args[0] == "--network")
        {
            using (var client = new ClassroomClient())
            {
                client.LoginAsync(args[1], int.Parse(args[2]), false, args[3], args[4])
                      .GetAwaiter().GetResult();
                var archive = client.DownloadAssignmentsAsync().GetAwaiter().GetResult();
                if (archive.Length < 100)
                    throw new Exception("The assignment ZIP is empty");
                client.HeartbeatAsync().GetAwaiter().GetResult();
                client.LogoutAsync().GetAwaiter().GetResult();
                Console.WriteLine("Classroom HTTP client: OK");
            }
            return 0;
        }
        if (args.Length != 1)
            return 2;
        var directory = Path.Combine(Path.GetTempPath(), "abc-pro-local-test-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(directory);
        try
        {
            var source = Path.Combine(directory, "sum.pas");
            File.WriteAllText(source, "{Сумма двух чисел}\nvar a,b: integer;\n" +
                                      "begin Readln(a); Readln(b); Writeln(a+b) end.");
            var tests = new List<ClassroomTest> {
                new ClassroomTest { input = "2\n3\n", expected = "5\n" },
                new ClassroomTest { input = "4\n7\n", expected = "11\n" }
            };
            var result = LocalTestRunner.Run(source, tests, args[0]);
            if (result.Passed != 2 || result.Total != 2)
                throw new Exception("Correct tests failed: " + result.Message);
            tests[1].expected = "12\n";
            result = LocalTestRunner.Run(source, tests, args[0]);
            if (result.Passed != 1 || result.Total != 2)
                throw new Exception("Incorrect result was accepted: " + result.Message);
            Console.WriteLine("Classroom local tests: OK");
            return 0;
        }
        finally
        {
            Directory.Delete(directory, true);
        }
    }
}
