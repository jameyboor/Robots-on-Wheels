#include "Server.h"
#include "CommandHandler.h"
#include "Client.h"
#include "Network.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <error.h>

bool SetSocketBlocking(int fd, bool blocking)
{
	if (fd < 0) return false;

	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return false;
	flags = blocking ? (flags&~O_NONBLOCK) : (flags | O_NONBLOCK);
	int yes = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

void Server::Start()
{
	Log(LOG_LEVEL_INFO, "Starting server on port %u", m_port);
	m_listenThread = std::thread(&Server::Listen, this);
	m_checkThread = std::thread(&Server::CheckClients, this);
	m_inputThread = std::thread(&Server::StartInputThread, this);
}

void Server::Stop()
{
	if (m_isStopping)
		return;
	Log(LOG_LEVEL_INFO, "Stopping server..");
	m_stopEvent = true;
	m_isStopping = true;
}

void Server::Listen()
{
#ifdef __linux__
	m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	SetSocketBlocking(m_listenSocket, false);
	sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	clilen = sizeof(cli_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(m_port);

	bind(m_listenSocket, (sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(m_listenSocket, 3);
	Log(LOG_LEVEL_INFO, "Server started..listening");
	std::cout << '\a'; // beep

	while (!m_stopEvent)
	{
			int clientSocket = accept(m_listenSocket, (sockaddr *)&cli_addr, &clilen);
			if (clientSocket > 0)
			{
				Client* client = new Client(clientSocket, GenerateClientId(), inet_ntoa(cli_addr.sin_addr));
				clientList.insert(std::make_pair(client->GetId(), client));
				client->StartRead();
				Log(LOG_LEVEL_INFO, "Client %s accepted, sending test packet", inet_ntoa(cli_addr.sin_addr));
				Packet packet(COMMAND_MOTOR_CONTROL);
				packet << 1500;
				QueuePacket(client->GetId(), packet);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1500));
	}
#endif
}

void Server::DisconnectClient(uint32_t Id)
{
	m.lock();
	auto client = clientList.find(Id);
	if (client != clientList.end())
	{
		Client* cli = client->second; 
		Log(LOG_LEVEL_INFO, "Disconnecting Client %s", cli->GetIp().c_str());
		close(cli->GetSocket());
		cli->WaitForCleanup();
		clientList.erase(client);
		delete cli;
	}
	m.unlock();
}

void Server::CheckClients()
{
	while (!m_stopEvent)
	{
		for (auto itr = clientList.begin(); itr != clientList.end(); ++itr)
		{
			if (itr->second && itr->second->IsFinished())
				removalList.push_back(itr->first);
		}

		for (auto list : removalList)
		{
			DisconnectClient(list);
		}
		removalList.clear();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void Server::QueuePacket(uint32_t id, Packet packet)
{
	m.lock();
	writeQueue.insert(std::make_pair(id, std::move(packet)));
	m.unlock();
}

void Server::CheckQueue()
{
	if (writeQueue.empty())
		return;

	for (auto itr = writeQueue.begin(); itr != writeQueue.end(); ++itr)
	{
		if (Client* cli = GetClientById(itr->first))
		{
			Log(LOG_LEVEL_INFO, "Sending to Client %u", cli->GetId());
			cli->SendPacket(itr->second);
		}
		
	}
	writeQueue.clear();
}

int cli_hook_func()
{
	if (sServer->IsStopped())
		rl_done = 1;
	return 0;
}

int kb_hit_return()
{
	timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &fds);
}

char* command_finder(const char* text, int state)
{
	static int idx, len;
	const char* ret;
	auto commands = sCommandHandler->getCommandTable();

	if (!state)
	{
		idx = 0;
		len = strlen(text);
	}

	for (auto itr = commands.begin(); itr != commands.end(); ++itr)
	{
		ret = itr->name.c_str();
		if (strncmp(ret, text, len) == 0)
			return strdup(ret);
		if ((*itr).name.empty())
			break;
	}

	return ((char*)NULL);
}

char** cli_completion(const char* text, int start, int /*end*/)
{
	char** matches = NULL;

	if (start)
		rl_bind_key('\t', rl_abort);
	else
		matches = rl_completion_matches((char*)text, &command_finder);
	return matches;
}

void commandFinished()
{
	printf("Command> ");
	fflush(stdout);
}


void Server::StartInputThread()
{
	rl_attempted_completion_function = cli_completion;
	rl_event_hook = cli_hook_func;

	printf("Command>");

	while (!m_stopEvent)
	{
		fflush(stdout);
		if (sCommandHandler->IsMotorStatus())
		{
			char buf = 0;
			struct termios old = { 0 };
			if (tcgetattr(0, &old) < 0)
				perror("tcsetattr()");
			old.c_lflag &= ~ICANON;
			old.c_lflag &= ~ECHO;
			old.c_cc[VMIN] = 1;
			old.c_cc[VTIME] = 0;
			if (tcsetattr(0, TCSANOW, &old) < 0)
				perror("tcsetattr ICANON");
			if (read(0, &buf, 1) < 0)
				perror("read()");
			old.c_lflag |= ICANON;
			old.c_lflag |= ECHO;
			if (tcsetattr(0, TCSADRAIN, &old) < 0)
				perror("tcsetattr ~ICANON");

			std::string command = "";
			command += buf;
			sCommandHandler->HandleCommand(command);
		}
		else
		{
			char *command_str;
			command_str = readline("Command>");
			rl_bind_key('\t', rl_complete);
			if (command_str != NULL)
			{
				for (int x = 0; command_str[x]; ++x)
					if (command_str[x] == '\r' || command_str[x] == '\n')
					{
						command_str[x] = 0;
						break;
					}

				if (!*command_str)
				{
					free(command_str);
					continue;
				}

				std::string command(command_str);

				fflush(stdout);
				sCommandHandler->HandleCommand(command);
				commandFinished();
				add_history(command.c_str());
				free(command_str);
			}
			else if (feof(stdin))
				m_stopEvent = true;
		}
	}
}

Client * Server::GetClientById(uint32_t id)
{
    Client* client = nullptr; 
    if (clientList.find(id) != clientList.end()) 
        client = clientList[id]; 
    return client;
}

void Server::Quit()
{
	m_inputThread.join();
	m_checkThread.join();
	m_listenThread.join();
}