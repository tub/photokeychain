using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Win32.SafeHandles;
using System.IO;
using System.Runtime.InteropServices;

namespace FotoFrameTest
{
    class Win32API
    {
        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        internal static extern SafeFileHandle CreateFile(
            string lpFileName,
            [MarshalAs(UnmanagedType.U4)] FileAccess dwDesiredAccess,
            [MarshalAs(UnmanagedType.U4)] FileShare dwShareMode,
            IntPtr lpSecurityAttributes,
            [MarshalAs(UnmanagedType.U4)] FileMode dwCreationDisposition,
            uint dwFlagsAndAttributes,
            SafeFileHandle hTemplateFile);

        [DllImport("kernel32.dll")]
        static extern int QueryDosDevice(
            string lpDeviceName,
            IntPtr lpTargetPath,
            int ucchMax);

        public class Helpers
        {
            public static string[] QueryDosDevice()
            {
                // Allocate some memory to get a list of all system devices.
                // Start with a small size and dynamically give more space until we have enough room.
                int returnSize = 0;
                int maxSize = 100;
                string allDevices = null;
                IntPtr mem;
                string[] retval = null;
                const int ERROR_INSUFFICIENT_BUFFER = 122;

                while (returnSize == 0)
                {
                    mem = Marshal.AllocHGlobal(maxSize);
                    if (mem != IntPtr.Zero)
                    {
                        // mem points to memory that needs freeing
                        try
                        {
                            returnSize = Win32API.QueryDosDevice(null, mem, maxSize);
                            if (returnSize != 0)
                            {
                                allDevices = Marshal.PtrToStringAnsi(mem, returnSize);
                                retval = allDevices.Split('\0');
                                break;    // not really needed, but makes it more clear...
                            }
                            else if (Marshal.GetLastWin32Error() == ERROR_INSUFFICIENT_BUFFER)
                            {
                                maxSize *= 10;
                            }
                            else
                            {
                                Marshal.ThrowExceptionForHR(Marshal.GetLastWin32Error());
                            }
                        }
                        finally
                        {
                            Marshal.FreeHGlobal(mem);
                        }
                    }
                    else
                    {
                        throw new OutOfMemoryException();
                    }
                }
                return retval;
            }
        }
    }

    class Program
    {
        /// <summary>
        /// Application entry point
        /// </summary>
        /// <param name="args">Command line arguments</param>
        static void Main(string[] args)
        {
            Console.SetWindowSize(100, 40);
            Console.SetBufferSize(100, 1000);
            Console.WriteLine("=== FOTOFRAME HACK TEST ===");

            // Enumerate devices to find the right one...
            string deviceName = "";
            foreach (string dev in Win32API.Helpers.QueryDosDevice())
            {
                if (dev.Contains("Ven_SITRONIX"))
                {
                    deviceName = dev;
                    Console.WriteLine("Found device: {0}", dev);
                    break;
                }
            }

            if (deviceName == "")
            {
                Console.WriteLine("Could not find device! Is it connected and in Update mode?");
                // Wait for key...
                Console.WriteLine("Press any enter key...");
                Console.ReadLine();
                return;
            }

            // Open the device
            SafeFileHandle hDevice = new SafeFileHandle(IntPtr.Zero, true);
            string devicePath = @"\\.\" + deviceName;
            hDevice = Win32API.CreateFile(
                devicePath,
                FileAccess.Read,
                FileShare.ReadWrite,
                IntPtr.Zero,
                FileMode.Open,
                0x20000000 | 0x80000000,
                new SafeFileHandle(IntPtr.Zero, true));
            if (hDevice.IsInvalid)
            {
                Console.WriteLine("Could not open device, invalid handle! Error {0}",
                    Marshal.GetLastWin32Error());
            }
            else
            {
                Console.WriteLine("Opened device, handle {0}", hDevice.DangerousGetHandle());

                FileStream stream = new FileStream(hDevice, FileAccess.Read);
                
                // Read 4k of info
                int BUFFER_SIZE = 4 * 1024;
                //stream.Seek(0, SeekOrigin.Begin);
                byte[] buffer = new byte[BUFFER_SIZE];
                stream.Read(buffer, 0, BUFFER_SIZE);
                
                // Should be "SITRONIX CORP."
                Console.WriteLine("Buffer starts with '{0}'...", Encoding.ASCII.GetString(buffer, 0, 14));
                
                foreach (byte b in buffer)
                {
                    Console.ForegroundColor = ConsoleColor.DarkGray;
                    Console.Write("{0:X2}:", b);
                    if (b > 31)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.Write("{0} ", (char)b);
                    }
                    else
                    {
                        Console.ForegroundColor = ConsoleColor.White;
                        Console.Write("? ");
                    }
                }
                Console.ForegroundColor = ConsoleColor.Gray;
                Console.WriteLine();
            }

            // Wait for key...
            Console.WriteLine("Press any enter key...");
            Console.ReadLine();
        }
    }
}
