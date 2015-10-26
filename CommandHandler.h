#pragma once
#include "Includes.h"
#include <functional>
#include <algorithm>

struct Command
{
	Command(std::string cmd, std::function<void(const char* args)> hnd) : name(cmd), handle(hnd) {}
	std::string name;
	std::function<void(const char* args)> handle;
};

class CommandHandler
{
public:
	CommandHandler() : m_isMotor(false) {
		motorCommandTable.push_back(Command("6", HandleMotorRechtsCommand));
		motorCommandTable.push_back(Command("4", HandleMotorLinksCommand));
		motorCommandTable.push_back(Command("8", HandleMotorVooruitCommand));
		motorCommandTable.push_back(Command("2", HandleMotorAchteruitCommand));
		motorCommandTable.push_back(Command("5", HandleMotorStopCommand));
		motorCommandTable.push_back(Command("s", HandleMotorQuitCommand));

		commandTable.push_back(Command("quit", HandleQuitCommand));
		commandTable.push_back(Command("send", HandleSendCommand));
		commandTable.push_back(Command("clientinfo", HandleClientInfoCommand));
		commandTable.push_back(Command("motor", HandleMotorCommand));
		commandTable.push_back(Command("distance", HandleDistanceCommand));
        commandTable.push_back(Command("speed", HandleMotorSpeedCommand));
	}
	
	static CommandHandler* instance()
	{
		static CommandHandler instance;
		return &instance;
	}

	void ToggleMotor();
	bool IsMotorStatus() const { return m_isMotor; }

	std::vector<Command>& getCommandTable() { return commandTable;}

    void HandleCommand(std::string command);

	static void HandleQuitCommand(const char* args);
	static void HandleSendCommand(const char* args);
	static void HandleClientInfoCommand(const char* args);
	static void HandleMotorCommand(const char* args);
	static void HandleDistanceCommand(const char* args);
    static void HandleMotorSpeedCommand(const char* args);

	//motor commands only
	static void HandleMotorRechtsCommand(const char* args);
	static void HandleMotorLinksCommand(const char* args);
	static void HandleMotorAchteruitCommand(const char* args);
	static void HandleMotorVooruitCommand(const char* args);
	static void HandleMotorStopCommand(const char* args);
	static void HandleMotorQuitCommand(const char* args);

private:
	std::vector<Command> commandTable;
	std::vector<Command> motorCommandTable;
	bool m_isMotor;
};

#define sCommandHandler CommandHandler::instance()