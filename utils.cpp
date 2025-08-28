#include "libs.hpp"


void error_exit(std::string errorMsg)
{
	std::cerr << "Error : " << errorMsg << std::endl;
	exit(1);
}
