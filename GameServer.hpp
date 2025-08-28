#pragma once

#include "libs.hpp"
#include "Match.hpp"

typedef struct s_room 
{
	//me faut l'addresse ou le fd des deux clients ici ?? 
	uint32_t player1Addr = 0;
	int player1Fd;
	uint32_t player2Addr = 0;
	int player2Fd;
} t_room;

class GameServer
{
	public:
		GameServer(int udpPort, int tcpPort) : _udpPort(udpPort), _tcpPort(tcpPort) {
		}
		~GameServer() {};


		GameServer() = delete;
		GameServer(const GameServer &) = delete;
		GameServer& operator=(const GameServer &) = delete;

		void run();
		
		private:
		int _udpPort;
		int _tcpPort;
		
		//TCP ONLY DATA
		std::unordered_map<int, uint32_t> pendingClients;
		std::unordered_set<int> authenticatedClients;
		std::unordered_map<int, t_room> roomInstances;
		std::mutex log_mutex;
		
		
		//SHARED DATA
		std::unordered_map<uint32_t, Match> matchInstances;
		
		//METHODS
		void tcpRoutineThread();
		void udpRoutineThread();
		
		
		
		void log(const std::string msg);
		int manageRoomInstances(int clientFd, int &matchCode);
		bool createMatchInstance(t_room &room);
		std::unordered_map<std::string, std::string> parseIdString(std::string &msg);
		bool verifyIdentity(std::string &msg, int &matchCode);
		std::string computeHMAC(const std::string &clientString);
};