#pragma once

#include "types.h"

inline bool get_flag(Board const& board, Coord const& c, int flag) {
	return ((board[c.row][c.col] & 0x000000FF) & flag) != 0;
}

inline bool get_flag(Board const& board, int row, int col, int flag) {
	return ((board[row][col] & 0x000000FF) & flag) != 0;
}

inline Owner get_owner(Board const& board, Coord const& c) {
	return static_cast<Owner>((board[c.row][c.col] & 0x0000FF00) >> 8);
}

inline Owner get_owner(Board const& board, int row, int col) {
	return static_cast<Owner>((board[row][col] & 0x0000FF00) >> 8);
}

inline Counter get_counter(Board const& board, Coord const& c) {
	return static_cast<Counter>((board[c.row][c.col] & 0xFFFF0000) >> 16);
}

inline Counter get_counter(Board const& board, int row, int col) {
	return static_cast<Counter>((board[row][col] & 0xFFFF0000) >> 16);
}

inline bool get_is_empty(Board const& board, Coord const& c) {
	return !(board[c.row][c.col] & 0x000000FF);
}

inline bool get_is_enterable(Board const& board, Coord const& c) {
	return (board[c.row][c.col] & 0x000000FF) != OccupierFlag::Player;
}

inline bool get_is_enterable(Board const& board, int row, int col) {
	return (board[row][col] & 0x000000FF) != OccupierFlag::Player;
}

inline void clear_flag(Board& board, Coord const& c, int flag) {
	board[c.row][c.col] |= ~flag;
}

inline void clear_flag(Board& board, int row, int col, int flag) {
	board[row][col] |= ~flag;
}

inline void set_flag(Board& board, Coord const& c, int flag) {
	board[c.row][c.col] |=  flag;
}

inline void set_flag(Board& board, int row, int col, int flag) {
	board[row][col] |=  flag;
}

inline void clear_flags(Board& board, Coord const& c) {
	board[c.row][c.col] &= 0xFFFF0000;
}

inline void set_owner(Board& board, Coord const& c, uint8_t owner) {
	board[c.row][c.col] &= 0xFFFF00FF;
	board[c.row][c.col] |= owner << 8;
}

inline void set_owner(Board& board, int row, int col, uint8_t owner) {
	board[row][col] &= 0xFFFF00FF;
	board[row][col] |= owner << 8;
}

inline void set_counter(Board& board, Coord const& c, uint16_t counter) {
	board[c.row][c.col] &= 0x0000FFFF;
	board[c.row][c.col] |= counter << 16;
	// cout << setw(8) << setfill('0') << hex << board[c.row][c.col] << endl;
}

inline void set_counter(Board& board, int row, int col, uint16_t counter) {
	board[row][col] &= 0x0000FFFF;
	board[row][col] |= counter << 16;
	// cout << setw(8) << setfill('0') << hex << board[c.row][c.col] << endl;
}

inline Coord rel_coord(Coord const& in, Move dir) {
	switch (dir) {
		case Move::Up:    return Coord(in.row - 1, in.col);
		case Move::Down:  return Coord(in.row + 1, in.col);
		case Move::Left:  return Coord(in.row, in.col - 1);
		case Move::Right: return Coord(in.row, in.col + 1);
		case Move::None:  return Coord(in.row, in.col);
	}
}

inline Move rel_dir(Coord const& from, Coord const& to) {
	for (auto const& m : AllMoves) {
		if (to == rel_coord(from, m)) return m;
	}
	return Move::None;
}

inline int taxicab_dist(Coord const& a, Coord const& b) {
	return abs(a.row - b.row) + abs(a.col - b.col);
}

inline bool in_bounds(int row, int col, GameState const& state) {
	if (row < 0) return false;
	if (col < 0) return false;
	if (row >= state.rows) return false;
	if (col >= state.cols) return false;
	return true;
}

inline bool in_bounds(Coord const& a, GameState const& state) {
	return in_bounds(a.row, a.col, state);
}

inline bool are_neighbours(Coord const& a, Coord const& b) {
	return abs(a.row - b.row) + abs(a.col - b.col) == 1;
}

inline std::string move_str(Move const& dir) {
	switch (dir) {
		case Move::Up:    return "up";
		case Move::Down:  return "down";
		case Move::Left:  return "left";
		case Move::Right: return "right";
		case Move::None:  return "none"; // only used for certain death
	}
}

inline void stepdown_counter(Board& board) {
	for (int i = 0; i < board.size(); ++i) {
		for (int j = 0; j < board[i].size(); ++j) {
			auto val = get_counter(board, i, j);
			if (val > 0) {
				set_counter(board, i, j, static_cast<Counter>(val - 1));
				if (val == 1) {
					clear_flag(board, i, j, OccupierFlag::Player);
				}
			}
		}
	}
}

inline void stepdown_food(GameState& state) {
//  for (int i = 0; i < state.food_left.size(); ++i) {
//    state.food_left[i]--;
//  }
}


