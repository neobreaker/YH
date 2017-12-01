using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace YHServer.YHLib
{
    class YHbuffer
    {
        private int m_blocks;
        private Queue<byte[]> m_data_queue;

        public YHbuffer(int blocks)
        {
            m_blocks = blocks;

            m_data_queue = new Queue<byte[]>(m_blocks);

        }

        public void Enqueue(byte[] data)
        {
            if (m_data_queue.Count == m_blocks)
                m_data_queue.Dequeue();
            m_data_queue.Enqueue(data);
        }

        public byte[] Dequeue()
        {
            return m_data_queue.Dequeue();
        }
    }
}
