#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>

typedef int Square;

// Byte 0 -- what occupies a space
class OccupierFlag {
public:
	static const uint8_t Player  = 1 << 0;
	static const uint8_t Food    = 1 << 1;
	static const uint8_t Visited = 1 << 2;
	static const uint8_t Area    = 1 << 3;
	static const uint8_t CutVert = 1 << 4;
};

// Byte 1 -- by index in snakes
typedef uint8_t Owner;

// Bytes 2-3 -- helps to progress board state
typedef uint16_t Counter;

typedef std::vector< std::vector<Square> > Board;
typedef Board LabelledBoard;

struct Coord {
	Coord() = default;
	Coord(int r, int c) : row(r), col(c) {}
	bool operator==(Coord const& other) const {
		return this->row == other.row && this->col == other.col;
	}
	bool operator<(Coord const& other) const {
		if (this->row < other.row) return true;
		if (this->row == other.row) return this->col < other.col;
		return false;
	}

	int row;
	int col;
};

typedef std::set<Coord>    UniqCoords;
typedef std::vector<Coord> Coords;

inline std::ostream& operator<<(std::ostream& os, const Coord& c)
{
	os << "(" << c.row << "," << c.col << ")";
	return os;
}

enum class Move {
	Up, Down, Left, Right, None
};
typedef std::set<Move> Moves;

const std::set<Move> AllMoves = {
	Move::Up, Move::Down, Move::Left, Move::Right
};

struct Snake {
	Snake() = default;
	Coords coords;
	int health; // How much food each snake has remaining
	std::string id;
	Coord head() const { return coords.front(); }
	Coord tail() const { return coords.back(); }
	int length() const { return coords.size(); }
	int drawn_length() const {
		UniqCoords uniq;
		for (auto const& c : coords) uniq.insert(c);
		return uniq.size();
	}
};

struct GameState {
	GameState() : rows(0), cols(0) {}
	GameState(int rows, int cols) : rows(rows), cols(cols) {
		board.resize(rows);
		for (int i = 0; i < cols; ++i) {
			board[i] = std::vector<Square>(cols);
		}
	}

	int rows, cols;
	Board board; // Board state
	std::string my_id;
	Snake me;
	std::vector<Snake> snakes;
	Coords food;
};

typedef std::map<std::string, GameState> GameStates;
