#pragma once
#include <stdint.h>

inline uint32_t EndianConvert(uint32_t n)
{
	unsigned char *np = (unsigned char *)&n;

	return ((uint32_t)np[0] << 24) |
		((uint32_t)np[1] << 16) |
		((uint32_t)np[2] << 8) |
		(uint32_t)np[3];
}