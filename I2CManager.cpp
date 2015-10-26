#include "I2CManager.h"
#include "Server.h"
#include "Client.h"

void I2CMgr::DistanceSensor::StartMeasure()
{
	uint8_t data = DistanceSensorInfo::COMMAND_START_MEASURE;
	I2CWriteBytes(&data, 1, DistanceSensorInfo::REGISTER_COMMAND);
	std::this_thread::sleep_for(std::chrono::microseconds(200000));
}

uint16_t I2CMgr::DistanceSensor::GetNewDistance()
{
	uint8_t distanceBytes[] = { 0, 0 };
	I2CReadByte(distanceBytes[0], DistanceSensorInfo::REGISTER_RESULT_LOW_BYTE);
	I2CReadByte(distanceBytes[1], DistanceSensorInfo::REGISTER_RESULT_HIGH_BYTE);
	return (distanceBytes[0] | (distanceBytes[1] << 8));
}

void I2CMgr::DistanceSensor::UpdateDistance()
{
	WriteGuard(m);
	StartMeasure();
	m_distance = GetNewDistance();
}

void I2CMgr::checkWall()
{
	while (!sServer->IsStopped())
	{
		sI2CMgr->GetDistanceSensor()->UpdateDistance();
		uint16_t dist = sI2CMgr->GetDistanceSensor()->GetDistance();
		if (dist < 60 && sI2CMgr->GetMotorMgr()->IsGoingForward())
		{
            auto achteruit = GetMotorMgr()->GenerateMovementTemplate(DIRECTION_ACHTERUIT, DIRECTION_ACHTERUIT, 900, 900);
            auto stop = GetMotorMgr()->GenerateMovementTemplate(DIRECTION_STOP, DIRECTION_STOP, 0, 0);
            GetMotorMgr()->WriteDirection(&achteruit);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
            GetMotorMgr()->WriteDirection(&stop);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
	}
}

void I2CMgr::MotorMgr::WriteDirection(MovementInfo* info)
{
    if (I2CWriteBytes(reinterpret_cast<uint8*>(info), sizeof(MovementInfo)) > 0)
    {
        if (info->m_partOneDirection == DIRECTION_VOORUIT && info->m_partTwoDirection == DIRECTION_VOORUIT)
            m_isForward = true;
        else
            m_isForward = false;
    }
}

I2CMgr::MovementInfo I2CMgr::MotorMgr::GenerateDefaultMovement()
{
    return MovementInfo(3, 0xa5, MovementConstants::DIRECTION_VOORUIT, 3, 0xa5, MovementConstants::DIRECTION_VOORUIT);
}

I2CMgr::MovementInfo I2CMgr::MotorMgr::GenerateMovementTemplate(MovementConstants leftDirection, MovementConstants rightDirection, uint16_t speedLeft, uint16_t speedRight)
{
    return MovementInfo((speedLeft >> 8) & 0xff, speedLeft & 0xff, leftDirection, (speedRight >> 8) & 0xff, speedRight & 0xff, rightDirection);
}

void I2CMgr::MotorMgr::SetMovementInfoSpeed(MovementInfo* info, bool left, uint16_t speed)
{
    if (left)
    {
        info->m_partOnePowerHigh = (speed >> 8) & 0xff;
        info->m_partOnePowerLow = speed & 0xff;
    }
    else
    {
        info->m_partTwoPowerHigh = (speed >> 8) & 0xff;
        info->m_partTwoPowerLow = speed & 0xff;
    }
}
