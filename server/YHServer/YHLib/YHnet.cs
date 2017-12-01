using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace YHServer.YHLib
{
    class YHnet
    {
        private Socket m_rcvsocket = null;
        private EndPoint m_remote_ep = null;
        private Thread m_thread_rcv;
        private byte[] m_rcv_buffer;
        private bool m_is_shutdown = false;

        private YHbuffer m_dgram_queue =  new YHbuffer(3);

        public YHnet(string dst_ip, int dst_port, int src_port)
        {
            // 初始化socket
            m_rcvsocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            // 绑定本地端口
            IPEndPoint local_ep = new IPEndPoint(IPAddress.Any, src_port);
            m_rcvsocket.Bind(local_ep);

            IPEndPoint sender = new IPEndPoint(IPAddress.Parse(dst_ip), dst_port);
            m_remote_ep = (EndPoint)sender;

            m_rcv_buffer = new Byte[4096];

            m_thread_rcv = new Thread(ThreadDoRecv);
            m_thread_rcv.IsBackground = true;
            m_thread_rcv.Start();
        }

        public void ShutDown()
        {
            m_is_shutdown = true;
        }

        private void ThreadDoRecv()
        {
            while (!m_is_shutdown)
            {
                m_rcvsocket.ReceiveFrom(m_rcv_buffer, SocketFlags.None, ref m_remote_ep);

                m_dgram_queue.Enqueue(m_rcv_buffer);
            }
        }

        public void SendTo(byte[] data, int data_len)
        {
            m_rcvsocket.SendTo(data, data_len, SocketFlags.None, m_remote_ep);
        }
    }
}
