using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace YHServer
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        private Socket m_rcvsocket = null;
        private EndPoint m_remote_ep = null;

        private Thread m_thread_rcv;
      
        private byte[] m_rcv_buffer;

        public MainWindow()
        {
            InitializeComponent();
            ListenSocketInit();
        }

        private void ListenSocketInit()
        {
            // 初始化socket
            m_rcvsocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            // 绑定本地端口
            IPEndPoint local_ep = new IPEndPoint(IPAddress.Any, 50000);
            m_rcvsocket.Bind(local_ep);

            IPEndPoint sender = new IPEndPoint(IPAddress.Parse("192.168.3.60"), 50000);
            m_remote_ep = (EndPoint)sender;

            m_rcv_buffer = new Byte[4096];

            m_thread_rcv = new Thread(ThreadDoRecv);
            m_thread_rcv.IsBackground = true;
            m_thread_rcv.Start();
        }

        private void ThreadDoRecv()
        {
            int cnt = 0;
            while (true)
            {
                m_rcvsocket.ReceiveFrom(m_rcv_buffer, SocketFlags.None, ref m_remote_ep);

                cnt++;
            }
        }

        private void BtnConnect_Click(object sender, RoutedEventArgs e)
        {
            byte[] data = new byte[1024];
            string cmd = "request";
            data = ASCIIEncoding.ASCII.GetBytes(cmd);

            m_rcvsocket.SendTo(data, data.Length, SocketFlags.None, m_remote_ep);
        }
    }
}
