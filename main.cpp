#include <algorithm>
#include <iomanip>
#include <iostream>
#include <deque>
#include <queue>
#include <random>
#include <set>

// RULES:
// - If two snakes of same length hit each other they both die

// TODO:

// - NEVER move into a spot where in the *best* case scenario (tails disappearing with no heads added) we can't fit our length
// Don't have to be clever about it, just project ourselves forward to ensure there's at least ONE path (depth first search) long enough for us
// For instance, there's a 1-block slot and we move towards it even though we'd have nowhere to go afterwards
// Easiest might be to flood-fill the point we're going to and just look at how many empty squares it's connected to
// i.e. never enter an AREA we can't fit ourselves in

// - If we can "race" all other snakes to food (guaranteed available path to reach it AND escape?) we should probably do it
// Check taxicab distance from all snakes to food and try to keep ourselves 1 turn ahead (keep their taxicab distances to food at ours + 1 at a minimum)

// - We'll surround food with our own body such that once we get it we have nowhere to go *after*
// It's OK to leave food until near-starvation, but we have to make damn sure we can get OUT after getting the food!

// - Have no logic for head collisions
// - Strongly avoid "head" spaces that can be moved to by other snakes if they're longer than us

// - Subtract a value from the "counter" to see how many turns we have to "stall" until we'd get full access to different areas

// - Move to the space your opponent wants in TWO turns (unless you are longer, in which case move for the one turn and if he collides he dies!)

// - You can perfectly encircle if your length is even -- for instance, with length 8 you can perfectly surround food
// If odd, you can mostly encircle (but space between your tail and head)
// To encircle, just try to be a rectangle at first (until we get so big that we have to cover more area)

// - FEATURES TO CONSIDER:
//  - Do we have the most/least health points of all remaining snakes? Where are we in the rankings?
//  - Are we the closest/farthest from food of all remaining snakes?
//  - Are we the shortest/longest of all remaining snakes?
//  - Do we have the most/least possible area to move to of all remaining snakes?

// - Really, we're trying to maximize our turns remaining while minimizing everybody else's
//  - Maximum of turns remaining is just health remaining -- aren't really guaranteed any more than that
//  - Minimum is 0 (no possible move left to not die on the next turn)
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

// Byte 1 -- by index in snakes
typedef uint8_t Owner;

// Bytes 2-3 -- helps to progress board state
typedef uint16_t Counter;

typedef vector< vector<Square> > Board;

struct Coord {
  Coord() = default;
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
	Snake() = default;
	vector<Coord> coords;
	int health_points; // How much food each snake has remaining
	string id;
	Coord head() const { return coords.front(); }
	Coord tail() const { return coords.back(); }
	int length() const { return coords.size(); }
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
  return ((board[c.row][c.col] & 0x000000FF) & flag) != 0;
}

inline Owner get_owner(Board const& board, Coord const& c) {
  return static_cast<Owner>((board[c.row][c.col] & 0x0000FF00) >> 8);
}

inline Counter get_counter(Board const& board, Coord const& c) {
  return static_cast<Counter>((board[c.row][c.col] & 0xFFFF0000) >> 16);
}

inline bool get_is_empty(Board const& board, Coord const& c) {
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

inline void set_counter(Board& board, Coord const& c, uint16_t counter) {
  board[c.row][c.col] &= 0x0000FFFF;
  board[c.row][c.col] |= counter << 16;
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
			if (get_is_empty(board, c)) {
				cout << '_';
			} else if (get_flag(board, c, OccupierFlag::Player)) {
				draw_colored('0' + (get_counter(board, c) % 10), get_owner(board, c));
			} else if (get_flag(board, c, OccupierFlag::Food)) {
				draw_colored('*', 1);
			} else {
				cout << '?';
			}
    }
    cout << endl;
  }
}

void draw(GameState const& state) {
	int i = 0;
	for (auto const& snake : state.snakes) {
		cout << i++ << ": " << snake.health_points << " ";
	}
  cout << endl;
  draw_board(state.board);
}

void add_snake(GameState& state, Coord const& c, int length, int food) {
//  int idx = state.heads.size();
//  state.heads.push_back(c);
//  state.food_left.push_back(food);
//  set_occupier(state.board, state.heads[idx], Snake);
//  set_counter(state.board, state.heads[idx], counter);
}

void stepdown_counter(Board& board) {
  for (int i = 0; i < board.size(); ++i) {
    for (int j = 0; j < board[i].size(); ++j) {
      Coord c(i,j);
      auto val = get_counter(board, c);
      if (val > 0) {
        set_counter(board, c, static_cast<Counter>(val - 1));
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
//    set_counter(out.board, to, get_counter(out.board, in.heads[i]) + 1);
//    out.heads[i] = to;
//  }

  stepdown_counter(out.board);
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

// Don't collide with "bodies"
// Tail is OK (counter of 1) because it will be gone next turn
// We don't know where heads are going to be
// But anything else with a counter > 1 is either our body or another snake's
void filter_body_collisions(GameState const& state, Moves& moves) {
	auto const& me = get_snake(state, state.me);
	auto head = me.head();

	Moves filtered;
	for (auto const& m : moves) {
		if (get_counter(state.board, rel_coord(head, m)) > 1) continue;
		filtered.insert(m);
	}
	moves = filtered;
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

// Valid children are:
// 1. Not already visited
// 2. Not occupied by a non-tail player
void add_valid_children(Coord const& c, GameState& state, deque<Coord>& children) {
	for (auto const& m : AllMoves) {
		auto child = rel_coord(c, m);
		if (!in_bounds(child, state)) continue;
		if (get_flag(state.board, child, OccupierFlag::Visited)) continue;
		if (get_counter(state.board, child) > 1) continue;
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

			add_valid_children(c, state, children);

			if (c == dst) {
				cout << "Turn distance from " << src << " to " << dst << " is " << dist << endl;
				return dist;
			}
		}

		unvisited = children;
		++dist;

		// As we move out our tail will move so we can access more stuff
		stepdown_counter(state.board);
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
	bool clear_path_exists = false;
	for (auto const& m : moves) {
		auto min_dist = min_dist_to_food(rel_coord(me.head(), m), state);

		if (min_dist < INT_MAX) {
			clear_path_exists = true;
		}

		if (min_dist < me.health_points) {
			filtered.insert(m);
		}
	}

	// If we found at least one open path to food then we'll filter so we take that path
	if (clear_path_exists) {
		moves = filtered;
	}
	// There's no current path to food, so don't filter down to nothing or we'll think we're dead!
	// Just return non-colliding paths and we'll get smarter about what to do next...
	else {
		return;
	}
}

Move random_move(Moves const& allowable = AllMoves) {
	vector<Move> rmoves;
	for (auto move : allowable) rmoves.push_back(move);
	shuffle(rmoves.begin(), rmoves.end(), _rng);
	return *rmoves.begin();
}

// TODO: Improve by "stepping down" on each "turn" so that we properly measure the movement of our own tail
// and don't underestimate the area available to us
int accessible_area(GameState const &state, Coord const &start) {
	int area = 0;

	auto board = state.board;

	queue<Coord> unvisited;
	unvisited.push(start);

	while (!unvisited.empty()) {
		auto c = unvisited.front();
		unvisited.pop();

		if (!in_bounds(c, state)) continue;
		if (get_flag(board, c, OccupierFlag::Visited)) continue;
		if (get_counter(board, c) > 1) continue; // Non-tail player
		if (get_flag(board, c, OccupierFlag::Food)) --area; // Food spot is a null op, because we'll grow by one to use it

		set_flag(board, c, OccupierFlag::Visited);
		++area;

		for (auto const& dir : AllMoves) {
			unvisited.push(rel_coord(c, dir));
		}
	}

	return area;
}

// For each of the moves, flood fill and count the area available in that direction
// If the area isn't big enough to hold our length, don't go into it
void filter_inadequate_areas(GameState const& state, Moves& moves) {
	Moves filtered;
	auto me = get_snake(state, state.me);
	for (auto const& m : moves) {
		auto area = accessible_area(state, rel_coord(me.head(), m));
		if (area >= me.length()) {
			filtered.insert(m);
			cout << "Moving " << move_str(m) << " is OK since we can fit length " << me.length() << " in area " << area << endl;
		} else {
			cout << "Ignoring " << move_str(m) << " since we can't fit length " << me.length() << " in area " << area << endl;
		}
	}
}

Move get_move(GameState const& state, string& taunt) {
	// Start by considering all possible moves
	Moves moves = AllMoves;

	// Remove any moves that would cause us a guaranteed loss (wall collisions and body collisions)
	filter_wall_collisions(state, moves);
	filter_body_collisions(state, moves);
	filter_inadequate_areas(state, moves);

	// Any move we make is death!
	if (moves.empty()) {
		taunt = "Arghhh!";
		return Move::None;
	}

	// Get a copy of our options pre-rough-stuff
	// We can fall back to it if we have no other option
	Moves pre = moves;

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
	// We have a list of moves to choose from...
	else {

		auto me = get_snake(state, state.me);

		if (true /*me.length() < 8*/) {
			int min_dist = INT_MAX;
			Moves food_moves;
			for (auto const& m : moves) {
				int dist = min_dist_to_food(rel_coord(me.head(), m), state);
				if (dist < min_dist) {
					food_moves.clear();
					food_moves.insert(m);
					min_dist = dist;
				} else if (dist == min_dist) {
					food_moves.insert(m);
				}
			}

			if (food_moves.size() == 1) {
				taunt = "I KNOW how to eat!";
				return *food_moves.begin();
			} else {
				taunt = "Eat the FOOD, TINA!";
				return random_move(food_moves);
			}
		}
		else {
			taunt = "Let's get freaky!";
			return random_move(moves);
		}
	}
}

void process_snake(json const& j, uint8_t owner, GameState& state) {
	Snake s;

	for (auto jc : j.at("coords")) {
		Coord c(jc.at(1), jc.at(0));
		s.coords.push_back(c);
		set_flag(state.board, c, OccupierFlag::Player);
		set_owner(state.board, c, owner);
	}

	// Set counter in reverse so head has max value
	int ctr = 0;
	for (auto it = s.coords.rbegin(); it != s.coords.rend(); ++it) {
		set_counter(state.board, *it, static_cast<Counter>(++ctr));
	}

	s.health_points = j.at("health_points");
	s.id = j.at("id");
	state.snakes.push_back(s);
}

GameState process_state(json const& j) {
	if (!_states.count(j.at("game_id"))) throw exception();
	GameState state = _states.at(j.at("game_id"));

	for (auto const& f : j.at("food")) {
		Coord c(f.at(1), f.at(0));
		state.food.push_back(c);
		set_flag(state.board, c, OccupierFlag::Food);
	}

	state.snakes.clear();
	uint8_t owner = 0;
	for (auto const& s : j.at("snakes")) {
		process_snake(s, owner++, state);
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
		draw(state);

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
