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
using YHServer.YHLib;

namespace YHServer
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {

        private YHnet m_yhnet = null;
        public MainWindow()
        {
            InitializeComponent();
            ListenSocketInit();
        }

        private void ListenSocketInit()
        {
            m_yhnet = new YHnet("192.168.3.60");

            m_yhnet.Start();
        }

        private void BtnConnect_Click(object sender, RoutedEventArgs e)
        {
            byte[] data = new byte[1024];
            string cmd = "request";
            data = ASCIIEncoding.ASCII.GetBytes(cmd);
            m_yhnet.SendTo(data, data.Length);            
        }
    }
}
