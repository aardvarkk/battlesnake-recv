#pragma once

#include "types.h"

struct SnakeCoord {
	Coord c;       // The current coordinate
	int snake_idx; // Index of snake in game state
	Coords path;   // Path taken to get to this coord
};

Voronoi voronoi(GameState const& state);