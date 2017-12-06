using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.ComponentModel;

using NAudio.Wave;


namespace YHServer.YHLib
{
    class YHnet
    {
        private Socket m_rcvsocket = null;
        private Socket m_sndsocket = null;
        private EndPoint m_remote_rcvep = null;
        private EndPoint m_remote_sndep = null;

        private bool m_is_line_connected = false;

        private Thread m_thread_rcv;
        private byte[] m_rcv_buffer;

        private Thread m_thread_play;

        // file
        private FileStream m_fs = null;
        private bool m_is_save = false;

        private IWavePlayer m_wavePlayer = null;
        private BufferedWaveProvider m_wave_provider = null;

        public  YHbuffer m_dgram_queue =  new YHbuffer(30);

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

            IPEndPoint rcv = new IPEndPoint(IPAddress.Any, dst_sndport);
            m_remote_rcvep = (EndPoint)rcv;

            m_rcv_buffer = new Byte[4096];

            
            WaveFormat wf = new WaveFormat(8000, 16, 1);
            m_wavePlayer = new WaveOut();
            m_wave_provider = new BufferedWaveProvider(wf);
            m_wavePlayer.Init(m_wave_provider);
            m_wavePlayer.Play();

            m_thread_rcv = new Thread(ThreadDoRecv);
            m_thread_rcv.IsBackground = true;

            m_thread_rcv.Start();
        
        }

        public YHnet(string dst_ip)
        {
            YHnetSetup(dst_ip, 50000, 50001, 50000, 50001);
        }

        public YHnet(string dst_ip, int dst_rcvport, int dst_sndport, int src_rcvport, int src_sndport)
        {
            YHnetSetup(dst_ip, dst_rcvport, dst_sndport, src_rcvport, src_sndport);
        }

        public void YHSave(bool is_save)
        {
            this.m_is_save = is_save;
        }

        public void Start()
        {
            DateTime dt = DateTime.Now;
            string path = "E:\\";
            string filename = string.Format("{0:yyMMdd HHmmss}", dt) +".wav";

            if (m_is_save)
            {
                m_fs = new FileStream(path + filename, FileMode.OpenOrCreate);
            }

            m_dgram_queue.Clear();

            LineEstablish();

            m_is_line_connected = true;
            m_thread_play = new Thread(ThreadDoPlay);
            m_thread_play.IsBackground = true;
            m_thread_play.Start();

            
        }

        public void ShutDown()
        {
            if (m_is_save)
            {
                m_fs.Flush();
                m_fs.Close();
                
            }
            m_fs = null;

            m_is_line_connected = false;

            LineShutDown();
        }

        private void ThreadDoRecv()
        {
            int rcv_len = 0;
            while (true)
            {
                rcv_len = m_rcvsocket.ReceiveFrom(m_rcv_buffer, SocketFlags.None, ref m_remote_rcvep);

                m_dgram_queue.Enqueue(m_rcv_buffer, rcv_len);

                
            }
        }

        private void ThreadDoPlay()
        {
            YHElement e ;
            while (m_is_line_connected)
            {
                e = m_dgram_queue.Dequeue();
                if(e.m_len > 0)
                {
                    if (m_fs != null)
                    {
                        m_fs.Write(e.m_data, 0, e.m_len);
                    }
                    m_wave_provider.AddSamples(e.m_data, 0, e.m_len);
                }
            }
        }

        private void SendTo(byte[] data, int data_len)
        {
            m_sndsocket.SendTo(data, data_len, SocketFlags.None, m_remote_sndep);
        }

        public void LineEstablish()
        {
            byte[] data = new byte[1024];
            string cmd = "AT1";
            data = ASCIIEncoding.ASCII.GetBytes(cmd);
            SendTo(data, data.Length);
        }

        public void LineShutDown()
        {
            byte[] data = new byte[1024];
            string cmd = "AT2";
            data = ASCIIEncoding.ASCII.GetBytes(cmd);
            SendTo(data, data.Length);
        }
    }
}
