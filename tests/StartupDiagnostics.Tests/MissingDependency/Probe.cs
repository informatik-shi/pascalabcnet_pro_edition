namespace MissingDependency
{
    public static class Probe
    {
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
        public static string GetValue() { return "loader fixture"; }
    }
}
