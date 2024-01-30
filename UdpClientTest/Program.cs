using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace UdpClientTest
{
    internal class Program
    {
        static void Main(string[] args)
        {
            // Create a UdpClient object and bind it to a port
            UdpClient udpClient = new UdpClient(40610);

            // Start a task to listen to the port in the background
            Task listenTask = Task.Run(() => Listen(udpClient));

            // Wait for any key to be pressed
            Console.WriteLine("Press any key to stop listening...");
            Console.ReadKey();

            // Close the UdpClient object and cancel the listening task
            udpClient.Close();
            listenTask.Wait();
        }

        static void Listen(UdpClient udpClient)
        {
            // Create an IPEndPoint object to store the sender's information
            IPEndPoint remoteEP = new IPEndPoint(IPAddress.Any, 0);

            // Loop until the UdpClient object is closed
            while (udpClient.Client != null)
            {
                try
                {
                    // Receive data from the port and print it to the console
                    byte[] data = udpClient.Receive(ref remoteEP);
                    string message = Encoding.ASCII.GetString(data);
                    Console.WriteLine($"Received {message} from {remoteEP}");
                }
                catch (SocketException ex)
                {
                    // Handle the exception
                    Console.WriteLine(ex.Message);
                }
            }
        }
    }
}