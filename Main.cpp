#include "Server.h"
#include "I2CManager.h"

int main(int argc, char** argv)
{
	sServer->Start();
	sI2CMgr->Setup();
	while (!sServer->IsStopped())
	{
		sServer->CheckQueue();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	sServer->Quit();
}

