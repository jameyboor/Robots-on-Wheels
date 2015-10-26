#include "Client.h"
#include "Network.h"
#include "Server.h"

#include <algorithm>

void Client::StartRead()
{
    if (!m_started)
        m_readThread = std::thread(&Client::Read, this);
}

void Client::Read()
{
    m_started = true;
#ifdef __linux__
    while (m_started)
    {
        m_readBuffer.Normalize();
        m_readBuffer.EnsureFreeSpace();
        int read = recv(m_socket, m_readBuffer.GetWritePointer(), m_readBuffer.GetRemainingSpace(), 0);
        m_readBuffer.WriteCompleted(read);
        if (read > 0)
        {
            while (m_readBuffer.GetActiveSize() > 0)
            {
                if (m_headerBuffer.GetRemainingSpace() > 0)
                {
                    std::size_t readHeaderSize = std::min(m_readBuffer.GetActiveSize(), m_headerBuffer.GetRemainingSpace());
                    m_headerBuffer.Write(m_readBuffer.GetReadPointer(), readHeaderSize);
                    m_readBuffer.ReadCompleted(readHeaderSize);

                    if (m_headerBuffer.GetRemainingSpace() > 0)
                        break;

                    // received header
                    HandleHeaderRead();
                }

                if (m_packetBuffer.GetRemainingSpace() > 0)
                {
                    std::size_t readDataSize = std::min(m_readBuffer.GetActiveSize(), m_packetBuffer.GetRemainingSpace());
                    m_packetBuffer.Write(m_readBuffer.GetReadPointer(), readDataSize);
                    m_readBuffer.ReadCompleted(readDataSize);

                    if (m_packetBuffer.GetRemainingSpace() > 0)
                        break;
                }

                HandleDataRead();
                ++m_receivedPackets;

                m_headerBuffer.Reset();
            }
        }
        else
            m_started = false;
    }
#endif
}

void Client::HandleHeaderRead()
{
    MessageHeader* header = reinterpret_cast<MessageHeader*>(m_headerBuffer.GetReadPointer());
    header->opcode = EndianConvert(header->opcode);
    header->size = EndianConvert(header->size);
    Log(LOG_LEVEL_INFO, "Got new header, command %u, size %u", header->opcode, header->size);
    m_packetBuffer.Resize(header->size);
}

void Client::HandleDataRead()
{
    MessageHeader* header = reinterpret_cast<MessageHeader*>(m_headerBuffer.GetReadPointer());
    Packet pkt(header->opcode, std::move(m_packetBuffer));
    Log(LOG_LEVEL_INFO, "Got new packet, opcode %u, from Client %u (%s)", pkt.GetOpcode(), GetId(), GetIp().c_str());
    sNetworkHandler->CallHandler(this, &pkt);
}

void Client::SendPacket(Packet packet)
{
    WriteGuard guard(m);
    MessageBuffer buffer(packet.size() + sizeof(MessageHeader));
    MessageHeader header;
    header.opcode = packet.GetOpcode();
    header.size = packet.size();
    buffer.Write(&header, sizeof(MessageHeader));
    if (!packet.empty())
        buffer.Write(packet.contents(), packet.size());
    Log(LOG_LEVEL_INFO, "Sending packet..size : %u", buffer.GetActiveSize());
    int sent = send(m_socket, buffer.GetReadPointer(), buffer.GetActiveSize(), 0);
    if (sent < 0)
        m_started = false;
    else
        ++m_sentPackets;
}

void Client::WaitForCleanup()
{
    m_readThread.join();
}
