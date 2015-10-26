#include "CommandHandler.h"
#include "Server.h"
#include "Client.h"
#include "I2CManager.h"
#include "Packet.h"

void CommandHandler::ToggleMotor()
{
	m_isMotor = !m_isMotor;
	termios old = { 0 };
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	if (m_isMotor)
	{
		old.c_lflag &= ~ICANON;
		old.c_lflag &= ~ECHO;
		old.c_cc[VMIN] = 1;
		old.c_cc[VTIME] = 0;
		if (tcsetattr(0, TCSANOW, &old) < 0)
			perror("tcsetattr ICANON");
	}
	else
	{
		old.c_lflag |= ICANON;
		old.c_lflag |= ECHO;
		if (tcsetattr(0, TCSADRAIN, &old) < 0)
			perror("tcsetattr ~ICANON");
	}
}

void CommandHandler::HandleCommand(std::string command)
{
    std::string newArgs = command;
    char const* text = newArgs.c_str();
    std::string cmd = "";

    while (*text != ' ' && *text != '\0')
    {
        cmd += *text;
        ++text;
    }

    while (*text == ' ') ++text;

    std::vector<Command>::iterator place = (m_isMotor ? std::find_if(motorCommandTable.begin(), motorCommandTable.end(), [&cmd](const Command &arg) {
        return arg.name == cmd; }) : motorCommandTable.end());

    if (place == motorCommandTable.end())
    {
        for (auto itr = commandTable.begin(); itr != commandTable.end(); ++itr)
        {
            if ((*itr).name == cmd)
            {
                place = itr;
                break;
            }
        }
    }

    if (place != motorCommandTable.end())
        (*place).handle(text);
    else
        Log(LOG_LEVEL_WARNING, "Command %s not found in general command table %s.", cmd.c_str(), m_isMotor ? "or motor command table" : "");
}

void CommandHandler::HandleQuitCommand(const char * args)
{
	sServer->Stop();
}

void CommandHandler::HandleSendCommand(const char * args)
{
	if (!*args)
		return;

	const char* clientId = strtok((char*)args, " ");
	const char* cOpcode = strtok(NULL, " ");
	std::vector<int> values;
	const char* val = strtok(NULL, " ");
	while (val)
	{
		values.push_back(atoi(val));
		val = strtok(NULL, " ");
	}
	if (!clientId)
	{
		Log(LOG_LEVEL_INFO, "You must specify a client ID");
		return;
	}
	uint32_t opcode = 1;
	if (opcode)
		opcode = atoi(cOpcode);

	Packet packet(opcode);
	for (auto var : values)
	{
		packet << var;
	}
	sServer->QueuePacket(atoi(clientId), std::move(packet));
}

void CommandHandler::HandleClientInfoCommand(const char * args)
{
	std::ostringstream ss;
	ss << "\n" << "Connected Clients : " << sServer->GetClientCount() << "\n\n";
	for (auto clientInfo : sServer->GetClientList())
	{
		auto client = clientInfo.second;
		ss << "Client #" << client->GetId() << ":\n" << "Sent Packets to client : " << client->GetSentPackets()
			<< "\nReceived Packets from client : " << client->GetReceivedPackets();
	}
	Log(LOG_LEVEL_INFO, ss.str());
}

void CommandHandler::HandleMotorCommand(const char * args)
{
	sCommandHandler->ToggleMotor();
}

void CommandHandler::HandleDistanceCommand(const char * args)
{
	uint16_t dist = sI2CMgr->GetDistanceSensor()->GetDistance();
	Log(LOG_LEVEL_INFO, "Distance is %u", dist);
}

void CommandHandler::HandleMotorSpeedCommand(const char * args)
{
    sI2CMgr->GetMotorMgr()->SetCurrentSpeed(atoi(args));
}

void CommandHandler::HandleMotorRechtsCommand(const char * args)
{
    auto movement = sI2CMgr->GetMotorMgr()->GenerateMovementTemplate(I2CMgr::MovementConstants::DIRECTION_VOORUIT, I2CMgr::MovementConstants::DIRECTION_ACHTERUIT, sI2CMgr->GetMotorMgr()->GetCurrentSpeed(), sI2CMgr->GetMotorMgr()->GetCurrentSpeed());
    sI2CMgr->GetMotorMgr()->WriteDirection(&movement);
}

void CommandHandler::HandleMotorLinksCommand(const char * args)
{
    auto movement = sI2CMgr->GetMotorMgr()->GenerateMovementTemplate(I2CMgr::MovementConstants::DIRECTION_ACHTERUIT, I2CMgr::MovementConstants::DIRECTION_VOORUIT, sI2CMgr->GetMotorMgr()->GetCurrentSpeed(), sI2CMgr->GetMotorMgr()->GetCurrentSpeed());
    sI2CMgr->GetMotorMgr()->WriteDirection(&movement);
}

void CommandHandler::HandleMotorAchteruitCommand(const char * args)
{
    auto movement = sI2CMgr->GetMotorMgr()->GenerateMovementTemplate(I2CMgr::MovementConstants::DIRECTION_ACHTERUIT, I2CMgr::MovementConstants::DIRECTION_ACHTERUIT, sI2CMgr->GetMotorMgr()->GetCurrentSpeed(), sI2CMgr->GetMotorMgr()->GetCurrentSpeed());
    sI2CMgr->GetMotorMgr()->WriteDirection(&movement);
}

void CommandHandler::HandleMotorVooruitCommand(const char * args)
{
    auto movement = sI2CMgr->GetMotorMgr()->GenerateMovementTemplate(I2CMgr::MovementConstants::DIRECTION_VOORUIT, I2CMgr::MovementConstants::DIRECTION_VOORUIT, sI2CMgr->GetMotorMgr()->GetCurrentSpeed(), sI2CMgr->GetMotorMgr()->GetCurrentSpeed());
    sI2CMgr->GetMotorMgr()->WriteDirection(&movement);
}

void CommandHandler::HandleMotorStopCommand(const char * args)
{
    auto movement = sI2CMgr->GetMotorMgr()->GenerateMovementTemplate(I2CMgr::MovementConstants::DIRECTION_STOP, I2CMgr::MovementConstants::DIRECTION_STOP, 0, 0);
    sI2CMgr->GetMotorMgr()->WriteDirection(&movement);
}

void CommandHandler::HandleMotorQuitCommand(const char * args)
{
	sCommandHandler->ToggleMotor();
}

