#pragma once

#include "libs.hpp"

class Player
{
	public:
		Player() {};
		Player(uint32_t playerAddr, int playerFd) : _playerAddr(playerAddr), _playerFd(playerFd) {};
		~Player() {};

		int getFd() { return _playerFd; }

	private:
		uint32_t	_playerAddr;
		int	_playerFd;

};