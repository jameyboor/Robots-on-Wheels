#pragma once
#include <exception>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <time.h>
#include <cmath>
#include <type_traits>
#include <sstream>
#include <string.h>

#include "Typedefs.h"
#include "Endianness.h"


class MessageBuffer;

// Root of ByteBuffer exception hierarchy
class ByteBufferException : public std::exception
{
public:
	~ByteBufferException() throw() { }

	char const* what() const throw() override { return msg_.c_str(); }

protected:
	std::string & message() throw() { return msg_; }

private:
	std::string msg_;
};

class ByteBufferPositionException : public ByteBufferException
{
public:
	ByteBufferPositionException(bool add, size_t pos, size_t size, size_t valueSize);

	~ByteBufferPositionException() throw() { }
};

class ByteBufferSourceException : public ByteBufferException
{
public:
	ByteBufferSourceException(size_t pos, size_t size, size_t valueSize);

	~ByteBufferSourceException() throw() { }
};

class ByteBuffer
{
public:
	const static size_t DEFAULT_SIZE = 0x1000;

	// constructor
	ByteBuffer() : _rpos(0), _wpos(0)
	{
		_storage.reserve(DEFAULT_SIZE);
	}

	ByteBuffer(size_t reserve) : _rpos(0), _wpos(0)
	{
		_storage.reserve(reserve);
	}

	ByteBuffer(ByteBuffer&& buf) : _rpos(buf._rpos), _wpos(buf._wpos),
		_storage(std::move(buf._storage)) { }

	ByteBuffer(ByteBuffer const& right) : _rpos(right._rpos), _wpos(right._wpos),
		_storage(right._storage) { }

	ByteBuffer(MessageBuffer&& buffer);

	ByteBuffer& operator=(ByteBuffer const& right)
	{
		if (this != &right)
		{
			_rpos = right._rpos;
			_wpos = right._wpos;
			_storage = right._storage;
		}
		return *this;
	}

	virtual ~ByteBuffer() { }

	void clear()
	{
		_storage.clear();
		_rpos = _wpos = 0;
	}

	template <typename T> void append(T value)
	{
		static_assert(std::is_fundamental<T>::value, "append(compound)");
		value = EndianConvert(value);
		append((uint8 *)&value, sizeof(value));
	}

	template <typename T> void put(size_t pos, T value)
	{
		static_assert(std::is_fundamental<T>::value, "append(compound)");
		value = EndianConvert(value);
		put(pos, (uint8 *)&value, sizeof(value));
	}

	ByteBuffer &operator<<(uint8 value)
	{
		append<uint8>(value);
		return *this;
	}

	ByteBuffer &operator<<(uint16 value)
	{
		append<uint16>(value);
		return *this;
	}

	ByteBuffer &operator<<(uint32 value)
	{
		append<uint32>(value);
		return *this;
	}

	ByteBuffer &operator<<(uint64 value)
	{
		append<uint64>(value);
		return *this;
	}

	// signed as in 2e complement
	ByteBuffer &operator<<(int8 value)
	{
		append<int8>(value);
		return *this;
	}

	ByteBuffer &operator<<(int16 value)
	{
		append<int16>(value);
		return *this;
	}

	ByteBuffer &operator<<(int32 value)
	{
		append<int32>(value);
		return *this;
	}

	ByteBuffer &operator<<(int64 value)
	{
		append<int64>(value);
		return *this;
	}

	// floating points
	ByteBuffer &operator<<(float value)
	{
		append<float>(value);
		return *this;
	}

	ByteBuffer &operator<<(double value)
	{
		append<double>(value);
		return *this;
	}

	ByteBuffer &operator<<(const std::string &value)
	{
		if (size_t len = value.length())
			append((uint8 const*)value.c_str(), len);
		append((uint8)0);
		return *this;
	}

	ByteBuffer &operator<<(const char *str)
	{
		if (size_t len = (str ? strlen(str) : 0))
			append((uint8 const*)str, len);
		append((uint8)0);
		return *this;
	}

	ByteBuffer &operator>>(bool &value)
	{
		value = read<char>() > 0 ? true : false;
		return *this;
	}

	ByteBuffer &operator>>(uint8 &value)
	{
		value = read<uint8>();
		return *this;
	}

	ByteBuffer &operator>>(uint16 &value)
	{
		value = read<uint16>();
		return *this;
	}

	ByteBuffer &operator>>(uint32 &value)
	{
		value = read<uint32>();
		return *this;
	}

	ByteBuffer &operator>>(uint64 &value)
	{
		value = read<uint64>();
		return *this;
	}

	//signed as in 2e complement
	ByteBuffer &operator>>(int8 &value)
	{
		value = read<int8>();
		return *this;
	}

	ByteBuffer &operator>>(int16 &value)
	{
		value = read<int16>();
		return *this;
	}

	ByteBuffer &operator>>(int32 &value)
	{
		value = read<int32>();
		return *this;
	}

	ByteBuffer &operator>>(int64 &value)
	{
		value = read<int64>();
		return *this;
	}

	ByteBuffer &operator>>(float &value)
	{
		value = read<float>();
		if (!std::isfinite(value))
			throw ByteBufferException();
		return *this;
	}

	ByteBuffer &operator>>(double &value)
	{
		value = read<double>();
		if (!std::isfinite(value))
			throw ByteBufferException();
		return *this;
	}

	ByteBuffer &operator>>(std::string& value)
	{
		value.clear();
		while (rpos() < size())                         // prevent crash at wrong string format in packet
		{
			char c = read<char>();
			if (c == 0)
				break;
			value += c;
		}
		return *this;
	}

	uint8& operator[](size_t const pos)
	{
		if (pos >= size())
			throw ByteBufferPositionException(false, pos, 1, size());
		return _storage[pos];
	}

	uint8 const& operator[](size_t const pos) const
	{
		if (pos >= size())
			throw ByteBufferPositionException(false, pos, 1, size());
		return _storage[pos];
	}

	size_t rpos() const { return _rpos; }

	size_t rpos(size_t rpos_)
	{
		_rpos = rpos_;
		return _rpos;
	}

	void rfinish()
	{
		_rpos = wpos();
	}

	size_t wpos() const { return _wpos; }

	size_t wpos(size_t wpos_)
	{
		_wpos = wpos_;
		return _wpos;
	}

	template<typename T>
	void read_skip() { read_skip(sizeof(T)); }

	void read_skip(size_t skip)
	{
		if (_rpos + skip > size())
			throw ByteBufferPositionException(false, _rpos, skip, size());
		_rpos += skip;
	}

	template <typename T> T read()
	{
		T r = read<T>(_rpos);
		_rpos += sizeof(T);
		return r;
	}

	template <typename T> T read(size_t pos) const
	{
		if (pos + sizeof(T) > size())
			throw ByteBufferPositionException(false, pos, sizeof(T), size());
		T val = *((T const*)&_storage[pos]);
		val = EndianConvert(val);
		return val;
	}

	void read(uint8 *dest, size_t len)
	{
		if (_rpos + len > size())
			throw ByteBufferPositionException(false, _rpos, len, size());
		std::memcpy(dest, &_storage[_rpos], len);
		_rpos += len;
	}


	uint8* contents()
	{
		if (_storage.empty())
			throw ByteBufferException();
		return _storage.data();
	}

	uint8 const* contents() const
	{
		if (_storage.empty())
			throw ByteBufferException();
		return _storage.data();
	}

	size_t size() const { return _storage.size(); }
	bool empty() const { return _storage.empty(); }

	void resize(size_t newsize)
	{
		_storage.resize(newsize, 0);
		_rpos = 0;
		_wpos = size();
	}

	void reserve(size_t ressize)
	{
		if (ressize > size())
			_storage.reserve(ressize);
	}

	void append(const char *src, size_t cnt)
	{
		return append((const uint8 *)src, cnt);
	}

	template<class T> void append(const T *src, size_t cnt)
	{
		return append((const uint8 *)src, cnt * sizeof(T));
	}

	void append(const uint8 *src, size_t cnt)
	{
		if (!cnt)
			throw ByteBufferSourceException(_wpos, size(), cnt);

		if (!src)
			throw ByteBufferSourceException(_wpos, size(), cnt);


		if (_storage.size() < _wpos + cnt)
			_storage.resize(_wpos + cnt);
		std::memcpy(&_storage[_wpos], src, cnt);
		_wpos += cnt;
	}

	void append(const ByteBuffer& buffer)
	{
		if (buffer.wpos())
			append(buffer.contents(), buffer.wpos());
	}


	void put(size_t pos, const uint8 *src, size_t cnt)
	{
		if (pos + cnt > size())
			throw ByteBufferPositionException(true, pos, cnt, size());

		if (!src)
			throw ByteBufferSourceException(_wpos, size(), cnt);

		std::memcpy(&_storage[pos], src, cnt);
	}

protected:
	size_t _rpos, _wpos;
	std::vector<uint8> _storage;
};

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, std::vector<T> v)
{
	b << (uint32)v.size();
	for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
	{
		b << *i;
	}
	return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, std::vector<T> &v)
{
	uint32 vsize;
	b >> vsize;
	v.clear();
	while (vsize--)
	{
		T t;
		b >> t;
		v.push_back(t);
	}
	return b;
}

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, std::list<T> v)
{
	b << (uint32)v.size();
	for (typename std::list<T>::iterator i = v.begin(); i != v.end(); ++i)
	{
		b << *i;
	}
	return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, std::list<T> &v)
{
	uint32 vsize;
	b >> vsize;
	v.clear();
	while (vsize--)
	{
		T t;
		b >> t;
		v.push_back(t);
	}
	return b;
}

template <typename K, typename V>
inline ByteBuffer &operator<<(ByteBuffer &b, std::map<K, V> &m)
{
	b << (uint32)m.size();
	for (typename std::map<K, V>::iterator i = m.begin(); i != m.end(); ++i)
	{
		b << i->first << i->second;
	}
	return b;
}

template <typename K, typename V>
inline ByteBuffer &operator>>(ByteBuffer &b, std::map<K, V> &m)
{
	uint32 msize;
	b >> msize;
	m.clear();
	while (msize--)
	{
		K k;
		V v;
		b >> k >> v;
		m.insert(make_pair(k, v));
	}
	return b;
}

template<> inline std::string ByteBuffer::read<std::string>()
{
	std::string tmp;
	*this >> tmp;
	return tmp;
}

template<>
inline void ByteBuffer::read_skip<char*>()
{
	std::string temp;
	*this >> temp;
}

template<>
inline void ByteBuffer::read_skip<char const*>()
{
	read_skip<char*>();
}

template<>
inline void ByteBuffer::read_skip<std::string>()
{
	read_skip<char*>();
}

