#pragma once

#include "Includes.h"

#include <wiringPi.h>
#include <wiringPiI2C.h>

#define I2C_MOTOR_DEVICE 0x32
#define I2C_DISTANCE_DEVICE 0x70
#define DEFAULT_MOTOR_SPEED 930

class I2CMgr
{
public:
	I2CMgr() {}

	void Setup()
	{
		wiringPiSetup();
		motorMgr = new MotorMgr();
		distanceSensor = new DistanceSensor();
		checkThread = std::thread(&I2CMgr::checkWall, this);
	}

	static I2CMgr* instance()
	{
		static I2CMgr instance;
		return &instance;
	}

	void checkWall();

	~I2CMgr()
	{
		delete distanceSensor;
		delete motorMgr;
		checkThread.join();
	}
#pragma pack(push, 1)
    struct MovementInfo
    {
        MovementInfo() {}

        MovementInfo(uint8_t partOnePowerHigh, uint8_t partOnePowerLow, uint8_t partOneDirection, uint8_t partTwoPowerHigh, uint8_t partTwoPowerLow, uint8_t partTwoDirection)
            : m_command(7), m_partOnePowerHigh(partOnePowerHigh), m_partOnePowerLow(partOnePowerLow), m_partOneDirection(partOneDirection), m_partTwoPowerHigh(partTwoPowerHigh), m_partTwoPowerLow(partTwoPowerLow), m_partTwoDirection(partTwoDirection) {}

        uint8_t m_command;
        uint8_t m_partOnePowerHigh;
        uint8_t m_partOnePowerLow;
        uint8_t m_partOneDirection;
        uint8_t m_partTwoPowerHigh;
        uint8_t m_partTwoPowerLow;
        uint8_t m_partTwoDirection;
    };
#pragma pack(pop)

    enum MovementConstants
    {
        DIRECTION_STOP = 0,
        DIRECTION_VOORUIT = 2,
        DIRECTION_ACHTERUIT = 1,
    };

	class MotorMgr
	{
	public:
		MotorMgr() : m_isForward(false), m_currentSpeed(0)
		{
			m_i2cFd = wiringPiI2CSetup(I2C_MOTOR_DEVICE);
			uint8_t Totalpower[2] = { 4,250 };
			uint8_t Softstart[3] = { 0x91,23,0 };
			I2CWriteBytes(&Totalpower[0], 2);
			I2CWriteBytes(&Softstart[0], 3);
		}


		~MotorMgr()
		{
			if (m_i2cFd)
				close(m_i2cFd);
		}

        void WriteDirection(MovementInfo* info);

        MovementInfo GenerateDefaultMovement();
        MovementInfo GenerateMovementTemplate(MovementConstants leftDirection, MovementConstants rightDirection, uint16_t speedLeft = DEFAULT_MOTOR_SPEED, uint16_t speedRight = DEFAULT_MOTOR_SPEED);
        void SetMovementInfoSpeed(MovementInfo* info, bool left, uint16_t speed);

		bool IsGoingForward() const { return m_isForward; }
        void SetCurrentSpeed(uint16_t speed) { m_currentSpeed = speed; }
        uint16_t GetCurrentSpeed() const { return m_currentSpeed; }
	private:
		int m_i2cFd;
		bool m_isForward;
        uint16_t m_currentSpeed;

		int I2CWriteBytes(uint8_t* data, uint16_t length)
		{
			for (int i = 0; i < length; ++i)
			{
				int res = wiringPiI2CWrite(m_i2cFd, data[i]);
				if (res < 0)
				{
					Log(LOG_LEVEL_WARNING, "Writing byte %i went wrong in motor", i);
                    return res;
				}
			}
            return 1;
		}
	};

	enum DistanceSensorInfo
	{
		REGISTER_RESULT_HIGH_BYTE = 2,
		REGISTER_RESULT_LOW_BYTE = 3,
		REGISTER_COMMAND = 0,
		COMMAND_START_MEASURE = 0x51
	};

	class DistanceSensor
	{
	public:
		DistanceSensor() 
		{
			m_i2cFd = wiringPiI2CSetup(I2C_DISTANCE_DEVICE);
			pinMode(1, PWM_OUTPUT);
			digitalWrite(1, LOW);
			pwmSetClock(500);
			m_distance = 0;
			pwmWrite(1, 14);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		~DistanceSensor()
		{
			if (m_i2cFd)
				close(m_i2cFd);
		}

		void UpdateDistance();
		uint16_t GetDistance() { return m_distance; }
	private:
		int m_i2cFd;
		uint16_t m_distance;
		std::recursive_mutex m;

		uint16_t GetNewDistance();
		void StartMeasure();

		int I2CWriteBytes(uint8_t* data, uint16_t length, uint16_t reg)
		{
			for (int i = 0; i < length; ++i)
			{
				int res = wiringPiI2CWriteReg8(m_i2cFd, reg, data[i]);
				if (res < 0)
				{
					Log(LOG_LEVEL_WARNING, "Writing byte %i went wrong in distance sensor", i);
                    return res;
				}
			}
            return 1;
		}

		void I2CReadByte(uint8_t& data, uint16_t reg)
		{
			data = wiringPiI2CReadReg8(m_i2cFd, reg);
		}
	};


	int m_i2cFd;
	DistanceSensor* distanceSensor;
	MotorMgr* motorMgr;
	std::thread checkThread;

public :
	DistanceSensor* GetDistanceSensor() const { return distanceSensor; }
	MotorMgr* GetMotorMgr() const { return motorMgr; }
};
#define sI2CMgr I2CMgr::instance()
