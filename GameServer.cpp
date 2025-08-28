#include "GameServer.hpp"

const char* HMAC_SECRET_KEY = "MAXDOR\0";

void GameServer::run() 
{
	std::thread tcpThread(&GameServer::tcpRoutineThread, this);
	std::thread udpThread(&GameServer::udpRoutineThread, this);

	tcpThread.join();
	udpThread.join();
}

std::unordered_map<std::string, std::string> GameServer::parseIdString(std::string &msg)
{
	std::unordered_map<std::string, std::string> params;
	std::istringstream stream(msg);
	std::string pair;

	while(std::getline(stream, pair, '&'))
	{
		size_t pos = pair.find('=');
		if (pos != std::string::npos)
		{
			std::string key = pair.substr(0, pos);
			std::string value = pair.substr(pos + 1);
			params[key] = value;
		}
	}
	return params;
}

std::string GameServer::computeHMAC(const std::string &clientString)
{
	unsigned char* binaryBuffer;

	binaryBuffer = HMAC(EVP_sha256(), HMAC_SECRET_KEY, strlen(HMAC_SECRET_KEY), (const unsigned char*)clientString.c_str(), clientString.length(), nullptr, nullptr);

	std::ostringstream oss;

	for (int i = 0; i < 32; i++)
	{
		oss << std::hex << std::setw(2) << std::setfill('0') << (int)binaryBuffer[i];
	}
	return oss.str();
}

bool GameServer::verifyIdentity(std::string &msg, int &roomCode)
{
	//utiliser un HMAC pour vérifier l'identité du client si erreur ou fake return false
	auto idParams = parseIdString(msg);

	if (idParams.count("username") == 0 || idParams.count("timestamp") == 0 || idParams.count("hmac") == 0) 
	{
		log("Wrong auth format sent msg was : " + msg);
		roomCode = 42;
		return true;
	}
	
	std::string clientSideString = "username=" + idParams["username"] + "&timestamp=" + idParams["timestamp"];
	std::string expectedHmac = computeHMAC(clientSideString);

	if (idParams["hmac"] != expectedHmac)
		return false;

	time_t now = time(nullptr);
	time_t ts = std::stol(idParams["timestamp"]);
	if (std::abs(now - ts) > 3)
	{
		std::cerr << "Timestamp expired" << std::endl;
		return false;
	}
	log(std::to_string(roomCode));
	roomCode = static_cast<uint32_t>(std::stoi(idParams["roomCode"]));
	log(std::to_string(roomCode));
	
	return true;
}

void GameServer::tcpRoutineThread()
{
	//server tcp setup
	int serverSocket;
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		error_exit("TCP socket setup failed");

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//INADDR_ANY;
	serverAddr.sin_port = htons(_tcpPort);

	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		close(serverSocket);
		error_exit("TCP bind failed");
	}

	if (listen(serverSocket, 5) == -1)
	{
		close(serverSocket);
		error_exit("Listen failed");
	}

	log("TCP : Server listening on port : " + std::to_string(_tcpPort));


	int epollfd;

	if ((epollfd = epoll_create1(0)) == -1)
	{
		close(serverSocket);
		error_exit("Epoll create failed");
	}

	epoll_event ev{}, events[MAX_EVENTS];

	ev.events = EPOLLIN;
	ev.data.fd = serverSocket;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverSocket, &ev) == -1)
	{
		close(epollfd);
		close(serverSocket);
		error_exit("epoll_ctl failed");
	}

	while (1)
	{
		int n = epoll_wait(epollfd, events, MAX_EVENTS, -1);

		if (n == -1)
		{
			perror("epoll_wait");
			break;
		}

		for (int i = 0; i < n; i++)
		{	
			if (events[i].data.fd == serverSocket) 
			{
				sockaddr_in clientAddr{};
				socklen_t clientLen = sizeof(clientAddr);

				int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
				if (clientSocket == -1)
				{
					perror("accept");
					continue;
				}

				epoll_event clientEvent{};
				clientEvent.events = EPOLLIN;
				clientEvent.data.fd = clientSocket;
				epoll_ctl(epollfd, EPOLL_CTL_ADD, clientSocket, &clientEvent);

				log("Client connected through fd : " + std::to_string(clientEvent.data.fd));
				
				pendingClients[clientSocket] = clientAddr.sin_addr.s_addr;
			}
			else
			{
				char buffer[1024];

				int clientFd = events[i].data.fd;
				int bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
				
				std::string msg(buffer);

				if (bytesRead <= 0)
				{
					epoll_ctl(epollfd, EPOLL_CTL_DEL, clientFd, nullptr);
					close(clientFd);
					pendingClients.erase(clientFd);
					authenticatedClients.erase(clientFd);
					log("Client disconnected from fd : " + std::to_string(clientFd));
				}
				else if(pendingClients.find(clientFd) != pendingClients.end()) //first connection
				{
					int roomCode;
					if (verifyIdentity(msg, roomCode))
					{
						log("Id successfull managing room instance");
						//send to the client wether he is player 1 or 2 for initial coordinates on client side
						int result = manageRoomInstances(clientFd, roomCode);
						(void)result;
						//send UDP server informations to the client
						//std::string udpInfos = "adress=127.0.0.1&port=" + std::to_string(_udpPort) + "&playerNumber=" + playerNumber;
						// if (send(clientFd, udpInfos.c_str(), udpInfos.length(), 0) == -1)
						// 	continue;
						authenticatedClients.insert(clientFd);
					}
					pendingClients.erase(clientFd);
				}
				else //what is my else statement, what could/should the client send me through tcp ?
				{	//initial player data ?
					std::cout << "Entered empty else statement" << std::endl;
					//should send wheter or not he received udpInfos ? 
				}
			}
		}
	}
}

void GameServer::udpRoutineThread()
{
	int udpSocket;

	if ((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		error_exit("UDP socket setup failed");
	
	sockaddr_in udpAddr{};
	udpAddr.sin_family = AF_INET;
	udpAddr.sin_addr.s_addr = INADDR_ANY;
	udpAddr.sin_port = htons(_udpPort);

	if (bind(udpSocket, (sockaddr*)&udpAddr, sizeof(udpAddr)) == -1)
		error_exit("UDP bind failed");
	
	int epollfd;

	if ((epollfd = epoll_create1(0)) == -1)
		error_exit("Epoll create failed");

	epoll_event ev{}, events[MAX_EVENTS];

	ev.events = EPOLLIN;
	ev.data.fd = udpSocket;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, udpSocket, &ev) == -1)
	{
		close(epollfd);
		close(udpSocket);
		error_exit("epoll_ctl failed");
	}

	log("UDP : Socket ready to receive data");

	while (1)
	{
		int n = epoll_wait(epollfd, events, MAX_EVENTS, -1);

		if (n == -1)
			error_exit("epoll_wait failed");
		
		for (int i = 0; i < n; i++)
		{
			if (events[i].data.fd == udpSocket)
			{
				char buffer[1024];
				sockaddr_in clientAddr{};
				socklen_t clientLen = sizeof(clientAddr);

				ssize_t packet = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &clientLen);

				if (packet == -1)
				{
					perror("recvfrom");
					continue;
				}

				buffer[packet] = '\n';
				std::cout << "Received from client" << buffer << std::endl;
				//needs to analyse received player info and send back a packet to both with ?latest? informations
				//player will send a byte to indicate who he is in the match instance ? 
				

			}
		}
	}

}


//3 cases available here
// - First player to the room : return 0
// - Second player to the room and succes : return 1
// - Second player to the room and error : return -1
int GameServer::manageRoomInstances(int clientFd, int &roomCode)
{
	//then create new match instance
	//need to add locks When touching match instances
	if (roomInstances.find(roomCode) != roomInstances.end())
	{
		roomInstances[roomCode].player2Addr = pendingClients[clientFd];
		roomInstances[roomCode].player2Fd = clientFd;
		log("Room instance filled, creating match");
		return createMatchInstance(roomInstances[roomCode]);
	}
	t_room room{};
	room.player1Addr = pendingClients[clientFd];
	room.player1Fd = clientFd;
	log("Room instance created");
	roomInstances[roomCode] = room;
	return 0;
}


//these function could be reWritten with loops in order to decide on server launch if we want more than 2 players per lobby
bool GameServer::createMatchInstance(t_room &room)
{
	Player player1(room.player1Addr, room.player1Fd);
	Player player2(room.player2Addr, room.player2Fd);

	Match match(player1, player2);

	u_int32_t matchKey = 65; //generaterandomKey
	matchInstances[matchKey] = match;
	

	// for (int playerNumber = 0; playerNumber < 2; playerNumber++)
	// {
	// 	std::string udpInfos = "adress=127.0.0.1&port=" + std::to_string(_udpPort) +
	// 		"&playerNumber=" + std::to_string(playerNumber) +
	// 		"&matchcode=" + std::to_string(matchKey);
	// 	if (send(clientFd, udpInfos.c_str(), udpInfos.length(), 0) == -1)

	// }
	std::string udpInfos = "adress=127.0.0.1&port=" + std::to_string(_udpPort) +
		"&playerNumber=1" +
		"&matchcode=" + std::to_string(matchKey);
	send(player1.getFd(), udpInfos.c_str(), udpInfos.length(), 0);

	udpInfos = "adress=127.0.0.1&port=" + std::to_string(_udpPort) +
		"&playerNumber=2" +
		"&matchcode=" + std::to_string(matchKey);
	send(player2.getFd(), udpInfos.c_str(), udpInfos.length(), 0);

	//success players could almost start streaming through
	return 1;
}

void GameServer::log(const std::string msg)
{	
	std::lock_guard<std::mutex> lock(log_mutex);
	std::cout << msg << std::endl;
}