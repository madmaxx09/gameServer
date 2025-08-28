#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <glm/glm.hpp>
//networking libs
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
//multithreading libs
#include <thread>
#include <mutex>

constexpr int MAX_EVENTS = 10;

void error_exit(std::string errorMsg);

// void tcpRoutineThread(int portNumber);

// void udpRoutineThread(int portNumber);