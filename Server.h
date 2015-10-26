#pragma once

#include "Includes.h"
#include "Packet.h"

class Client;
class Server
{
public:
	Server(uint32_t port) : m_port(port), m_listenSocket(0), m_stopEvent(false), m_clientId(0), m_isStopping(false) {}

	static Server* instance()
	{
		static Server instance(2865);
		return &instance;
	}

	uint32_t GetPort() const { return m_port; }
	void SetPort(uint32_t port) { m_port = port; }

	void Start();
	void Stop();
	void Quit();
	void Listen();
	void DisconnectClient(uint32_t Id);
	void CheckClients();

	bool IsStopped() { return m_stopEvent; }

	void QueuePacket(uint32_t id, Packet packet);
	void CheckQueue();
	void StartInputThread();

	uint32_t GetClientCount() const { return clientList.size(); }

	uint32_t GenerateClientId() { return ++m_clientId; }
    Client* GetClientById(uint32_t id);
	std::map<uint32_t, Client*>& GetClientList() { return clientList; }
	
private:
	
	//threads
	std::thread m_listenThread;
	std::thread m_inputThread;
	std::thread m_checkThread;
	std::recursive_mutex m;

	//networking
	uint32_t m_port;
	int32_t m_listenSocket;
	std::map<uint32_t, Client*> clientList;
	std::vector<uint32_t> removalList;
	std::multimap<uint32_t, Packet> writeQueue;

	//operational
	bool m_stopEvent;
	bool m_isStopping;
	uint32_t m_clientId;
};

#define sServer Server::instance()

