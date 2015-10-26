#include "Network.h"
#include "Server.h"
#include "Client.h"
#include "I2CManager.h"
#include "Packet.h"

void NetworkHandler::HandleMotorControlCommand(Client* client, Packet& packet)
{
    int32 leftSlide = 0, rightSlide = 0;
    packet >> leftSlide >> rightSlide;
    leftSlide = (leftSlide > 0 ? std::min(100, leftSlide) : std::max(-100, leftSlide));
    rightSlide = (rightSlide > 0 ? std::min(100, rightSlide) : std::max(-100, rightSlide));

    bool leftNegative = false, rightNegative = false;

    if (leftSlide < 0)
        leftNegative = true;
    if (rightSlide < 0)
        rightNegative = true;

    int32 leftPower = 0, rightPower = 0;
    uint32 minPower = 700;
    uint32 maxPower = 1024;
    uint32 diff = maxPower - minPower;
    float percentualDiff = (float)diff / 100.0f; // get the difference in value for 1% of the difference max - min power
    uint32 leftVal = (percentualDiff * std::abs(leftSlide));
    uint32 rightVal = (percentualDiff * std::abs(rightSlide));
    leftPower = leftVal + minPower;
    rightPower = rightVal + minPower; // add the minPower to the value gathered from the slider + diff per percent
    I2CMgr::MovementInfo info = sI2CMgr->GetMotorMgr()->GenerateMovementTemplate(
        leftNegative ? I2CMgr::MovementConstants::DIRECTION_ACHTERUIT : I2CMgr::MovementConstants::DIRECTION_VOORUIT,
        rightNegative ? I2CMgr::MovementConstants::DIRECTION_ACHTERUIT : I2CMgr::MovementConstants::DIRECTION_VOORUIT,
        leftPower, rightPower);
    sI2CMgr->GetMotorMgr()->WriteDirection(&info);
}

void NetworkHandler::HandleRequestDistanceCommand(Client * client, Packet& /*packet*/)
{
	//Client requesting distance, get it.
    Packet pckt(COMMAND_REQUEST_DISTANCE_RESPONSE);
    pckt << sI2CMgr->GetDistanceSensor()->GetDistance();
    sServer->QueuePacket(client->GetId(), std::move(pckt));
}
