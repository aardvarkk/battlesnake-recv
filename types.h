#pragma once

#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <vector>

typedef int Square;

// Byte 0 -- what occupies a space
class OccupierFlag {
public:
	static const uint8_t Player       = 1 << 0;
	static const uint8_t Food         = 1 << 1;
	static const uint8_t Visited      = 1 << 2;
	static const uint8_t Accessible   = 1 << 3;
	static const uint8_t Inaccessible = 1 << 4;
	static const uint8_t CutVert      = 1 << 5;
};

// Byte 1 -- by index in snakes
typedef uint8_t Owner;

// Flag to indicate shared or non-single ownership
const uint8_t Unowned = 0xFF;

// Bytes 2-3 -- helps to progress board state
typedef uint16_t Counter;

class Board : public std::vector< std::vector<Square> >
{
public:
	Board() : std::vector< std::vector<Square> >() {}
	Board(int rows, int cols, Square val = 0) {
		resize(rows);
		for (auto& row : *this) {
			row = std::vector<Square>(cols, val);
		}
	}

	void set(Square val) {
		for (auto& row : *this) {
			for (auto& col : row) {
				col = val;
			}
		}
	}
};

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
typedef std::deque<Coord>  Coords;

inline std::ostream& operator<<(std::ostream& os, const Coord& c)
{
	os << "(" << c.row << "," << c.col << ")";
	return os;
}

struct Snake {
	Snake() : health(0), idx(0) {};

	int health; // How much food each snake has remaining
	int idx; // Index in snake array

	Coords coords;
	std::string id;

	Coord head() const { return coords.front(); }
	Coord tail() const { return coords.back(); }
	int length() const { return static_cast<int>(coords.size()); }
	int drawn_length() const {
		UniqCoords uniq;
		for (auto const& c : coords) uniq.insert(c);
		return static_cast<int>(uniq.size());
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
	int my_idx;
	Snake me;
	std::vector<Snake> snakes;
	Coords food;
};

typedef std::map<std::string, GameState> GameStates;

typedef std::vector< std::vector<Coords> > Tracks;
