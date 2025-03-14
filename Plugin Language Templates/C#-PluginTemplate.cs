using System;
using System.Runtime.InteropServices;
using System.Threading;

namespace PluginTemplate
{
    // Define the PluginEntry structure as it will be seen from unmanaged code.
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct PluginEntry
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string name;
        public IntPtr funcPtr;  // Pointer to the exported function.
        public int arity;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10)]
        public string[] paramTypes;
        [MarshalAs(UnmanagedType.LPStr)]
        public string retType;
    }

    public static class Plugin
    {
        // Helper to get a function pointer from a delegate.
        private static IntPtr GetFunctionPointer(Delegate d)
        {
            return Marshal.GetFunctionPointerForDelegate(d);
        }

        // Exported function: adds two numbers.
        [DllExport("addtwonumbers", CallingConvention = CallingConvention.Cdecl)]
        public static int AddTwoNumbers(int a, int b)
        {
            return a + b;
        }

        // Exported function: says hello.
        [DllExport("sayhello", CallingConvention = CallingConvention.Cdecl)]
        public static IntPtr SayHello(IntPtr namePtr)
        {
            string name = Marshal.PtrToStringAnsi(namePtr);
            string greeting = "Hello, " + name;
            // Caller must free the returned string.
            return Marshal.StringToHGlobalAnsi(greeting);
        }

        // Exported function: factorial.
        [DllExport("factorial", CallingConvention = CallingConvention.Cdecl)]
        public static int Factorial(int n)
        {
            if (n <= 1)
                return 1;
            return n * Factorial(n - 1);
        }

        // Exported function: Fibonacci.
        [DllExport("Fibonacci", CallingConvention = CallingConvention.Cdecl)]
        public static int Fibonacci(int n)
        {
            if (n <= 0)
                return 0;
            else if (n == 1)
                return 1;
            return Fibonacci(n - 1) + Fibonacci(n - 2);
        }

        // Exported function: cross-platform Beep.
        [DllExport("XBeep", CallingConvention = CallingConvention.Cdecl)]
        public static bool XBeep(int frequency, int duration)
        {
            try
            {
                // On Windows, use Console.Beep.
                Console.Beep(frequency, duration);
            }
            catch
            {
                // On non-Windows platforms, write the bell character.
                Console.Write("\a");
                Thread.Sleep(duration);
            }
            return true;
        }

        // Exported function: Sleep.
        [DllExport("PluginSleep", CallingConvention = CallingConvention.Cdecl)]
        public static bool PluginSleep(int ms)
        {
            Thread.Sleep(ms);
            return true;
        }

        // Exported function: DoEvents.
        [DllExport("DoEvents", CallingConvention = CallingConvention.Cdecl)]
        public static void DoEvents()
        {
            // In a real UI application you might call Application.DoEvents().
            // Here we simply yield the thread.
            Thread.Yield();
        }

        // Create delegates for each exported function.
        private static readonly Func<int, int, int> delAddTwoNumbers = AddTwoNumbers;
        private static readonly Func<IntPtr, IntPtr> delSayHello = SayHello;
        private static readonly Func<int, int> delFactorial = Factorial;
        private static readonly Func<int, int> delFibonacci = Fibonacci;
        private static readonly Func<int, int, bool> delXBeep = XBeep;
        private static readonly Func<int, bool> delPluginSleep = PluginSleep;
        private static readonly Action delDoEvents = DoEvents;

        // Array of plugin entries.
        private static PluginEntry[] pluginEntries = new PluginEntry[]
        {
            new PluginEntry {
                name = "AddTwoNumbers",
                funcPtr = GetFunctionPointer(delAddTwoNumbers),
                arity = 2,
                paramTypes = new string[10] { "double", "double", null, null, null, null, null, null, null, null },
                retType = "double"
            },
            new PluginEntry {
                name = "SayHello",
                funcPtr = GetFunctionPointer(delSayHello),
                arity = 1,
                paramTypes = new string[10] { "string", null, null, null, null, null, null, null, null, null },
                retType = "string"
            },
            new PluginEntry {
                name = "Factorial",
                funcPtr = GetFunctionPointer(delFactorial),
                arity = 1,
                paramTypes = new string[10] { "integer", null, null, null, null, null, null, null, null, null },
                retType = "integer"
            },
            new PluginEntry {
                name = "Fibonacci",
                funcPtr = GetFunctionPointer(delFibonacci),
                arity = 1,
                paramTypes = new string[10] { "integer", null, null, null, null, null, null, null, null, null },
                retType = "integer"
            },
            new PluginEntry {
                name = "Beep",
                funcPtr = GetFunctionPointer(delXBeep),
                arity = 2,
                paramTypes = new string[10] { "integer", "integer", null, null, null, null, null, null, null, null },
                retType = "boolean"
            },
            new PluginEntry {
                name = "Sleep",
                funcPtr = GetFunctionPointer(delPluginSleep),
                arity = 1,
                paramTypes = new string[10] { "integer", null, null, null, null, null, null, null, null, null },
                retType = "boolean"
            },
            new PluginEntry {
                name = "DoEvents",
                funcPtr = GetFunctionPointer(delDoEvents),
                arity = 0,
                paramTypes = new string[10] { null, null, null, null, null, null, null, null, null, null },
                retType = "boolean"
            }
        };

        // Exported function: returns the plugin entries.
        // The caller is expected to free the returned memory if necessary.
        [DllExport("GetPluginEntries", CallingConvention = CallingConvention.Cdecl)]
        public static IntPtr GetPluginEntries(out int count)
        {
            count = pluginEntries.Length;
            int size = Marshal.SizeOf(typeof(PluginEntry));
            IntPtr buffer = Marshal.AllocHGlobal(size * pluginEntries.Length);
            for (int i = 0; i < pluginEntries.Length; i++)
            {
                Marshal.StructureToPtr(pluginEntries[i], buffer + i * size, false);
            }
            return buffer;
        }
    }
}