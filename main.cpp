#include "libs.hpp"
#include "GameServer.hpp"
#include <iostream>

int main(int argc, char **argv)
{	
	//ok so one UDP thread with match logic being instanciated in a class/struct Match
	//each time a udp packet is sent, it sends a hash to find the corresponding match
	//so when tcp is established, client receives this "match id"
	if (argc != 2)
	{
		std::cout << "Please provide a port number" << std::endl;
		return 1;
	}

	GameServer server(666, atoi(argv[1]));

	server.run();

	return 0;
}