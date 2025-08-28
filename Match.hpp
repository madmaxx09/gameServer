#pragma once

#include "libs.hpp"
#include "Player.hpp"

class Match
{
	public:
		Match() {};
		Match(Player &player1, Player &player2) : _player1(player1), _player2(player2) {};
		~Match() {};

		void addSecondPlayer(Player &player);

	private:
		Player _player1;
		Player _player2;



};