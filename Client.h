#pragma once
#include "Includes.h"
#include "Packet.h"

class Client
{
public:
	explicit Client(int32_t socket, uint32_t id, std::string ip) : m_receivedPackets(0), m_sentPackets(0), m_socket(socket), m_id(id), m_started(false), m_ip(ip) { m_headerBuffer.Resize(sizeof(MessageHeader)); }

	friend class Server;

	void StartRead();
	void Read();
	void HandleHeaderRead();
	void HandleDataRead();

	void WaitForCleanup();

	auto GetId() const { return m_id; }
	auto GetSocket() const { return m_socket; }
	auto GetIp() const { return m_ip; }
	auto GetReceivedPackets() const { return m_receivedPackets; }
	auto GetSentPackets() const { return m_sentPackets; }

	auto IsFinished() const{ return !m_started; }

	void SetId(uint32_t id) { m_id = id; }

private:
	int32_t m_socket;
	MessageBuffer m_readBuffer;

	MessageBuffer m_headerBuffer;
	MessageBuffer m_packetBuffer;

	std::string m_ip;
	uint32_t m_id;

	uint32 m_receivedPackets; // packet received from this client
	uint32 m_sentPackets; //packets sent to this client

	void SendPacket(Packet pck);

	//threads
	std::thread m_readThread;
	std::mutex m;
	volatile bool m_started;
};