#include <algorithm>
#include <iomanip>
#include <iostream>
#include <deque>
#include <random>
#include <set>

using namespace std;

#include "httplib.h"
using namespace httplib;

#include "json.hpp"
using json = nlohmann::json;

const int SIM_ROWS = 20;
const int SIM_COLS = 20;

typedef int Square;

// Byte 0 -- what occupies a space
class OccupierFlag {
public:
	static const uint8_t Player  = 1 << 0;
	static const uint8_t Food    = 1 << 1;
	static const uint8_t Visited = 1 << 2;
};

// Byte 1 -- we are owner 0
typedef uint8_t Owner;

// Bytes 2-3 -- helps to progress board state
typedef uint16_t Length;

typedef vector< vector<Square> > Board;

struct Coord {
  Coord() {}
  Coord(int r, int c) : row(r), col(c) {}
	bool operator==(Coord const& other) const {
		return this->row == other.row && this->col == other.col;
	}

  int row;
  int col;
};

ostream& operator<<(ostream& os, const Coord& c)
{
	os << "(" << c.row << "," << c.col << ")";
	return os;
}

struct Snake {
	vector<Coord> coords;
	int health_points; // How much food each snake has remaining
	string id;
	Coord head() const { return coords.front(); }
	Coord tail() const { return coords.back(); }
	bool single_tail() const { return !(coords.back() == *(coords.end()-1)); } // Tail can stack at the start
};

struct GameState {
	GameState() : rows(0), cols(0) {}
  GameState(int rows, int cols) : rows(rows), cols(cols) {
    board.resize(rows);
    for (int i = 0; i < cols; ++i) {
      board[i] = vector<Square>(cols);
    }
  }

	int rows, cols;
  Board board; // Board state
	string me;
	vector<Snake> snakes;
	vector<Coord> food;
};

typedef map<string, GameState> GameStates;

GameStates _states;
default_random_engine _rng;

enum class Move {
  Up, Down, Left, Right, None
};
typedef set<Move> Moves;

const set<Move> AllMoves = {
	Move::Up, Move::Down, Move::Left, Move::Right
};

const string Colors[] = {
  "1;31", // Bright Red
  "1;32", // Bright Green
  "1;33", // Bright Yellow
  "1;34", // Bright Blue
  "1;35", // Bright Magenta
  "1;36", // Bright Cyan
  "31", // Red
  "32", // Green
  "33", // Yellow
  "34", // Blue
  "35", // Magenta
  "36", // Cyan
  "37", // White
  "1;30", // Bright Black
};

inline bool get_flag(Board const& board, Coord const& c, int flag) {
  return (board[c.row][c.col] & 0x000000FF) & flag;
}

inline Owner get_owner(Board const& board, Coord const& c) {
  return static_cast<Owner>((board[c.row][c.col] & 0x0000FF00) >> 8);
}

inline Length get_length(Board const& board, Coord const& c) {
  return static_cast<Length>((board[c.row][c.col] & 0xFFFF0000) >> 16);
}

inline bool is_empty(Board& board, Coord const& c) {
	return !(board[c.row][c.col] & 0x000000FF);
}

inline void clear_flag(Board& board, Coord const& c, int flag) {
	board[c.row][c.col] |= ~flag;
}

inline void set_flag(Board& board, Coord const& c, int flag) {
	board[c.row][c.col] |=  flag;
}

inline void clear_flags(Board& board, Coord const& c) {
  board[c.row][c.col] &= 0xFFFF0000;
}

inline void set_owner(Board& board, Coord const& c, uint8_t owner) {
  board[c.row][c.col] &= 0xFFFF00FF;
  board[c.row][c.col] |= owner << 8;
}

inline void set_length(Board& board, Coord const& c, uint16_t length) {
  board[c.row][c.col] &= 0x0000FFFF;
  board[c.row][c.col] |= length << 16;
  // cout << setw(8) << setfill('0') << hex << board[c.row][c.col] << endl;
}

string get_color(int idx) {
  return Colors[idx % 14];
}

void draw_colored(char c, int color_idx) {
  cout << "\033[" << get_color(color_idx) << "m" << c << "\033[0m";
}

void draw_board(Board const& board) {
  for (int i = 0; i < board.size(); ++i) {
    for (int j = 0; j < board[i].size(); ++j) {
      Coord c(i, j);
      switch (0 /*get_occupier(board, c)*/) {
//        case Empty:  cout << '_'; break;
//        case Player: draw_colored('0' + get_length(board, c), get_owner(board, c)); break;
//        case Food:   draw_colored('*', 1); break;
        default: cout << '?';
      }
    }
    cout << endl;
  }
}

void draw(GameState const& state) {
//  for (int i = 0; i < state.food_left.size(); ++i) {
//    cout << i << ": " << state.food_left[i] << " ";
//  }
  cout << endl;
  draw_board(state.board);
}

void add_snake(GameState& state, Coord const& c, int length, int food) {
//  int idx = state.heads.size();
//  state.heads.push_back(c);
//  state.food_left.push_back(food);
//  set_occupier(state.board, state.heads[idx], Snake);
//  set_length(state.board, state.heads[idx], length);
}

void stepdown_length(Board& board) {
  for (int i = 0; i < board.size(); ++i) {
    for (int j = 0; j < board[i].size(); ++j) {
      Coord c(i,j);
      auto val = get_length(board, c);
      if (val > 0) {
        set_length(board, c, val - 1);
        if (val == 1) {
          clear_flag(board, c, OccupierFlag::Player);
        }
      }
    }
  }
}

void stepdown_food(GameState& state) {
//  for (int i = 0; i < state.food_left.size(); ++i) {
//    state.food_left[i]--;
//  }
}

Coord rel_coord(Coord const& in, Move dir) {
  switch (dir) {
    case Move::Up:    return Coord(in.row - 1, in.col);
    case Move::Down:  return Coord(in.row + 1, in.col);
    case Move::Left:  return Coord(in.row, in.col - 1);
    case Move::Right: return Coord(in.row, in.col + 1);
		case Move::None:  return Coord(in.row, in.col);
  }
}

inline string move_str(Move const& dir) {
	switch (dir) {
		case Move::Up:    return "up";
		case Move::Down:  return "down";
		case Move::Left:  return "left";
		case Move::Right: return "right";
		case Move::None:  return "none"; // only used for certain death
	}
}

// TODO: Deal with collisions, starvation, etc. (all game logic!)
// TODO: When do you die of starvation? If you have no food at start of move or end?
GameState run_moves(GameState const& in, vector<Move> const& moves) {
  GameState out = in;

//  for (int i = 0; i < moves.size(); ++i) {
//    auto to = rel_coord(in.heads[i], moves[i]);
//    set_occupier(out.board, to, Snake);
//    set_owner(out.board, to, i);
//    set_length(out.board, to, get_length(out.board, in.heads[i]) + 1);
//    out.heads[i] = to;
//  }

  stepdown_length(out.board);
  stepdown_food(out);

  return out;
}

GameState random_step(GameState const& in) {
  return run_moves(in, { Move::Down });
}

void simulate() {
	auto state = GameState(SIM_ROWS, SIM_COLS);
	add_snake(state, Coord(0,0), 3, 100);
	set_flag(state.board, Coord(5, 8), OccupierFlag::Food);

	draw(state);

	state = random_step(state);

	draw(state);

	state = random_step(state);

	draw(state);

	state = random_step(state);

	draw(state);

	state = random_step(state);

	draw(state);

	state = random_step(state);

	draw(state);
}

Snake get_snake(GameState const& state, string const& id) {
	for (auto const& s : state.snakes) {
		if (s.id == id) return s;
	}
	return Snake();
}

void filter_wall_collisions(GameState const& state, Moves& moves) {
	auto const& me = get_snake(state, state.me);

	// Don't go off edges
	if (me.head().row <= 0) moves.erase(Move::Up);
	if (me.head().col <= 0) moves.erase(Move::Left);
	if (me.head().row >= state.rows - 1) moves.erase(Move::Down);
	if (me.head().col >= state.cols - 1) moves.erase(Move::Right);
}

void filter_self_collisions(GameState const& state, Moves& moves) {
	auto const& me = get_snake(state, state.me);

	// Don't collide with self
	for (auto const& body : me.coords) {
		// Tail is a special case...
		// If we're not about to eat food, it's OK to "collide" with the tail
		// because it's going to MOVE by the next frame!
		// Note that we only ignore our tail if it's a "single tail" (not a "stacked tail")
		// At this point we assume it's OK to hit our own tail
		// but we'll do a check later to filter it out if we're about to get food
		if (body == me.tail() && me.single_tail()) continue;

		// Check collisions with our own body (other than the tail)
		if (body == rel_coord(me.head(), Move::Up)) moves.erase(Move::Up);
		if (body == rel_coord(me.head(), Move::Left)) moves.erase(Move::Left);
		if (body == rel_coord(me.head(), Move::Down)) moves.erase(Move::Down);
		if (body == rel_coord(me.head(), Move::Right)) moves.erase(Move::Right);
	}
}

inline int taxicab_dist(Coord const& a, Coord const& b) {
	return abs(a.row - b.row) + abs(a.col - b.col);
}

bool in_bounds(Coord const& a, GameState const& state) {
	if (a.row < 0) return false;
	if (a.col < 0) return false;
	if (a.row >= state.rows) return false;
	if (a.col >= state.cols) return false;
	return true;
}

void add_unvisited_children(Coord const& c, GameState& state, deque<Coord>& children) {
	for (auto const& m : AllMoves) {
		auto child = rel_coord(c, m);
		if (!in_bounds(child, state)) continue;
		if (get_flag(state.board, child, OccupierFlag::Visited)) continue;
		set_flag(state.board, child, OccupierFlag::Visited);
		children.push_back(child);
	}
}
// Measures distance by marking sections of the game board as Visited
int turn_dist(Coord const& src, Coord const& dst, GameState const& const_state) {
	if (!in_bounds(src, const_state)) return INT_MAX;
	if (!in_bounds(dst, const_state)) return INT_MAX;

	int dist = 0;
	auto state = const_state;

	deque<Coord> unvisited;
	unvisited.push_back(src);
	set_flag(state.board, src, OccupierFlag::Visited);

	// Process all of our immediate children
	// Add all of their children during processing
	// If we don't make it to the destination in this run, go out "one turn"
	while (!unvisited.empty()) {

		// Holder for "next round"
		deque<Coord> children;

		// Go through all the coords at the current level and check them
		while (!unvisited.empty()) {
			auto c = unvisited.front();
			unvisited.pop_front();

			add_unvisited_children(c, state, children);

			if (c == dst) {
				cout << "Turn distance from " << src << " to " << dst << " is " << dist << endl;
				return dist;
			}
		}

		unvisited = children;
		++dist;
	}

	// Unreachable!
	cout << dst << " is unreachable from " << src << endl;
	return INT_MAX;
}

// Distance from any coordinate to food (in units of "number of moves")
// TODO: We can't use this "ideal" distance to food because there may not be an actual path of that distance
// Other snakes can cut us off so that we can't reach the food as soon as we expect
int min_dist_to_food(Coord const& c, GameState const& state) {
	int min_dist = INT_MAX;
	for (auto const& f : state.food) {
//		min_dist = min(min_dist, taxicab_dist(c, f));
		min_dist = min(min_dist, turn_dist(c, f, state));
	}
	return min_dist;
}

// Don't allow a move that would take us farther away from food than our remaining health points allow
// If our current position + proposed move is greater distance from food than our health points, eliminate it
void filter_starvation(GameState const& state, Moves& moves) {
	auto const& me = get_snake(state, state.me);

	Moves filtered;
	for (auto const& m : moves) {
		if (min_dist_to_food(rel_coord(me.head(), m), state) < me.health_points) {
			filtered.insert(m);
		}
	}

	moves = filtered;
}

Move get_move(GameState const& state, string& taunt) {
	// Start by considering all possible moves
	Moves moves = AllMoves;

	// Remove any moves that would cause us a guaranteed loss (wall collisions and self collisions)
	filter_wall_collisions(state, moves);
	filter_self_collisions(state, moves);

	// Any move we make is death!
	if (moves.empty()) {
		taunt = "Arghhh!";
		return Move::None;
	}

	// Get a copy of our options pre-rough-stuff
	// We can fall back to it if we have no other option
	Moves pre = moves;

	// TODO: Strongly avoid going down any paths that will lead to certain death
	// (we can wind into ourselves and not have a way out from our own trap)
	// How? We want to guarantee we have at least as many moves as our own length (+ any length gained by food)
	// or we don't go that way

	// Strongly avoid any collisions with other snake sides
	// (basically guaranteed loss unless they also collide in the same frame)
	// TODO: Don't collide with other snake sides and longer snake heads

	// Strongly prefer to not move farther away from food than our health allows,
	// but we can hope that another snake eats food and the new food regenerates closer to us
	filter_starvation(state, moves);

	// TODO: If we're about to get food, don't allow us to go into our tail spot... but that would imply
	// that the food is ON our tail (which is impossible), so I don't think we need to worry about this case!

	// We have no good options (very likely to die, but tiny chance of something else...)
	if (moves.empty()) {
		taunt = "Here goes nothing!";
		vector<Move> rmoves;
		for (auto move : pre) rmoves.push_back(move);
		shuffle(rmoves.begin(), rmoves.end(), _rng);
		return *rmoves.begin();
	}
	// We only have one possible move, so do it
	else if (moves.size() == 1) {
		taunt = "You leave me no choice...";
		return *moves.begin();
	}
	// We have a list of moves to choose from... for now choose randomly
	else {
		taunt = "Let's get freaky!";
		vector<Move> rmoves;
		for (auto move : moves) rmoves.push_back(move);
		shuffle(rmoves.begin(), rmoves.end(), _rng);
		return *rmoves.begin();
	}
}

Snake process_snake(json const& j) {
	Snake s;
	for (auto c : j.at("coords")) {
		s.coords.push_back(Coord(c.at(1), c.at(0)));
	}
	s.health_points = j.at("health_points");
	s.id = j.at("id");
	return s;
}

GameState process_state(json const& j) {
	if (!_states.count(j.at("game_id"))) throw exception();
	GameState state = _states.at(j.at("game_id"));

	for (auto const& f : j.at("food")) {
		state.food.push_back(Coord(f.at(1), f.at(0)));
	}

	state.snakes.clear();
	for (auto const& s : j.at("snakes")) {
		state.snakes.push_back(process_snake(s));
	}
	state.me = j.at("you");

	return state;
}

void server(int port) {
	Server server;

	server.post("/start", [](Request const& req, Response& res) {
		auto start = chrono::high_resolution_clock::now();

		cout << "/start" << endl;

		auto j_in = json::parse(req.body);
//		cout << j_in.dump(2) << endl;

		_states[j_in.at("game_id")] = GameState(j_in.at("height"), j_in.at("width"));

		json j_out;
		j_out["color"] = "#00ff00";
		j_out["name"] = "clifford";
		res.status = 200;
		res.headers = { { "Content-Type", "application/json" } };
		res.body = j_out.dump();

		auto end = chrono::high_resolution_clock::now();

		cout << "Responded in " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;
	});

	server.post("/move", [](Request const& req, Response& res) {
		auto start = chrono::high_resolution_clock::now();

		cout << "/move" << endl;

		auto j_in = json::parse(req.body);
//		cout << j_in.dump(2) << endl;

		auto state = process_state(j_in);

		json j_out;
		string taunt;
		j_out["move"] = move_str(get_move(state, taunt));
		j_out["taunt"] = taunt;
		res.status = 200;
		res.headers = { { "Content-Type", "application/json" } };
		res.body = j_out.dump();

//		cout << "output" << endl;
		cout << j_out.dump() << endl;

		auto end = chrono::high_resolution_clock::now();

		cout << "Responded in " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;
	});

	cout << "Listening on port " << port << endl;
	server.listen("0.0.0.0", port);
}

int main(int argc, const char* argv[]) {
	int port = 5000;
	if (argc > 1) {
		stringstream ss;
		ss << argv[1];
		ss >> port;
	}
	server(port);
	return 0;
}
