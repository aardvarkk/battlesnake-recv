#pragma once

#include "types.h"

struct SnakeBoard {
	SnakeBoard() : access_area(0), avg_turns(0), first_area(0) {}

	Coord head; // starting location for this snake
	Board dists; // counter is minimum distance for snake to get to each board square
	Board board; // counter is snake length for other snakes on the board
	Tracks tracks; // possible path to get to each of the squares on the board in min turns

	int   access_area; // All area that a snake can access
	float avg_turns;   // Average number of turns it would take for snake to get to accessible area
	int   first_area;  // Areas that snakes have first access to
};

struct Voronoi {
	Board board; // Overall board showing min distance to each point
	std::vector<SnakeBoard> snake_boards;  // Boards showing distance to each square per-snake
};

Voronoi voronoi(GameState const& state);