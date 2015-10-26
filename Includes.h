#pragma once

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#endif
#include <vector>
#include <string>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <map>
#include <stdio.h>

#include "MessageBuffer.h"
#include "ByteBuffer.h"
#include "Timer.h"
#include "Endianness.h"
#include "Logger.h"