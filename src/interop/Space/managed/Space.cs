using System.Runtime.CompilerServices;
[assembly: System.Reflection.AssemblyNativeVersion("1.0.0.0")]
namespace Sinespace
{
    public static class Space
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void HostReport(int value);
    }
}
