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
        private Socket m_sndsocket = null;
        private EndPoint m_remote_rcvep = null;
        private EndPoint m_remote_sndep = null;

        private Thread m_thread_rcv;
        private byte[] m_rcv_buffer;
        private bool m_is_shutdown = false;

        private YHbuffer m_dgram_queue =  new YHbuffer(3);

        private void YHnetSetup(string dst_ip, int dst_rcvport, int dst_sndport, int src_rcvport, int src_sndport)
        {
            // 初始化socket
            m_rcvsocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            // 绑定本地端口
            IPEndPoint rcv_ep = new IPEndPoint(IPAddress.Any, src_rcvport);
            m_rcvsocket.Bind(rcv_ep);

            m_sndsocket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            IPEndPoint snd_ep = new IPEndPoint(IPAddress.Any, src_sndport);
            m_sndsocket.Bind(snd_ep);

            IPEndPoint sender = new IPEndPoint(IPAddress.Parse(dst_ip), dst_rcvport);
            m_remote_sndep = (EndPoint)sender;

            m_thread_rcv = new Thread(ThreadDoRecv);
            m_thread_rcv.IsBackground = true;

            m_rcv_buffer = new Byte[4096];
        }

        public YHnet(string dst_ip)
        {
            YHnetSetup(dst_ip, 50000, 50001, 50000, 50001);
        }

        public YHnet(string dst_ip, int dst_rcvport, int dst_sndport, int src_rcvport, int src_sndport)
        {
            YHnetSetup(dst_ip, dst_rcvport, dst_sndport, src_rcvport, src_sndport);
        }

        public void Start()
        {
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
                m_rcvsocket.ReceiveFrom(m_rcv_buffer, SocketFlags.None, ref m_remote_rcvep);

                m_dgram_queue.Enqueue(m_rcv_buffer);
            }
        }

        public void SendTo(byte[] data, int data_len)
        {
            m_sndsocket.SendTo(data, data_len, SocketFlags.None, m_remote_sndep);
        }
    }
}
