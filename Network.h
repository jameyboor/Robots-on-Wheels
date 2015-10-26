#pragma once


#include "Includes.h"
#include "Packet.h"
#include "Client.h"

#include <functional>
#include <stdio.h>
#include <string.h>
#include <map>



struct NetworkCommand
{
public:
	NetworkCommand(uint32 opc, std::string nam, std::function<void(Client* client, Packet& packet)> handle) : opcode(opc), name(nam), handler(handle) {}
	uint32_t opcode;
	std::string name;
	std::function<void(Client* client, Packet& packet)> handler;
};

enum Commands
{
	COMMAND_MOTOR_CONTROL = 1, // command for motor control, payload : to be decided, client -> server
	COMMAND_REQUEST_DISTANCE, // request distance from distance sensor, client -> server
	COMMAND_REQUEST_DISTANCE_RESPONSE, // response to COMMAND_REQUEST_DISTANCE, payload : the current distance in centimeters
	COMMAND_STOP,
	COMMAND_PING,
	COMMAND_SET_SPEED // payload int32 speed (metric)
};

class NetworkHandler
{
public:
	NetworkHandler() {

		commandTable.insert(std::make_pair(COMMAND_MOTOR_CONTROL,
		NetworkCommand(COMMAND_MOTOR_CONTROL, "Motor Control", HandleMotorControlCommand)));

		commandTable.insert(std::make_pair(COMMAND_REQUEST_DISTANCE, NetworkCommand(COMMAND_REQUEST_DISTANCE, "Request Distance", HandleRequestDistanceCommand)));

	}

	static NetworkHandler* instance()
	{
		static NetworkHandler instance;
		return &instance;
	}

	void CallHandler(Client* client, Packet* packet)
	{
		auto handle = commandTable.find(packet->GetOpcode());
		if (handle != commandTable.end())
			handle->second.handler(client, *packet);
		else
			Log(LOG_LEVEL_WARNING, "Client %u (%s) sent packet with opcode %u, not recognized.", client->GetId(), client->GetIp().c_str(), packet->GetOpcode());
	}

	static void HandleMotorControlCommand(Client* client, Packet& packet);
	static void HandleRequestDistanceCommand(Client* client, Packet& packet);

private:
	std::map<uint32_t, NetworkCommand> commandTable;
};

#define sNetworkHandler NetworkHandler::instance()