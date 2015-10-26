#include "ByteBuffer.h"
#include "MessageBuffer.h"

ByteBuffer::ByteBuffer(MessageBuffer&& buffer) : _rpos(0), _wpos(0), _storage(buffer.Move())
{
}

ByteBufferPositionException::ByteBufferPositionException(bool add, size_t pos,
	size_t size, size_t valueSize)
{
	std::ostringstream ss;

	ss << "Attempted to " << (add ? "put" : "get") << " value with size: "
		<< valueSize << " in ByteBuffer (pos: " << pos << " size: " << size
		<< ")";

	message().assign(ss.str());
}

ByteBufferSourceException::ByteBufferSourceException(size_t pos, size_t size,
	size_t valueSize)
{
	std::ostringstream ss;

	ss << "Attempted to put a "
		<< (valueSize > 0 ? "NULL-pointer" : "zero-sized value")
		<< " in ByteBuffer (pos: " << pos << " size: " << size << ")";

	message().assign(ss.str());
}