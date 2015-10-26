#pragma once

#include "Typedefs.h"
#include "ByteBuffer.h"

#pragma pack(push, 1)
struct MessageHeader
{
    uint32_t opcode;
    uint32_t size;
};
#pragma pack(pop)

class Packet : public ByteBuffer
{
public:
	// just container for later use
	Packet() : ByteBuffer(0), m_opcode(0)
	{
	}

	explicit Packet(uint16 opcode, size_t res = 50) : ByteBuffer(res), m_opcode(opcode) { }

	Packet(Packet&& packet) : ByteBuffer(std::move(packet)), m_opcode(packet.m_opcode)
	{
	}

	Packet(Packet const& right) : ByteBuffer(right), m_opcode(right.m_opcode)
	{
	}

	Packet& operator=(Packet const& right)
	{
		if (this != &right)
		{
			m_opcode = right.m_opcode;
			ByteBuffer::operator =(right);
		}

		return *this;
	}

	Packet(uint16 opcode, MessageBuffer&& buffer) : ByteBuffer(std::move(buffer)), m_opcode(opcode) { }

	void Initialize(uint16 opcode, size_t newres = 50)
	{
		clear();
		_storage.reserve(newres);
		m_opcode = opcode;
	}

	uint16 GetOpcode() const { return m_opcode; }
	void SetOpcode(uint16 opcode) { m_opcode = opcode; }

protected:
	uint16 m_opcode;
};