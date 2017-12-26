#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <vector>

using namespace std;

#include "server_http.hpp"
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

#include "json.hpp"

using json = nlohmann::json;

const int SIM_ROWS = 20;
const int SIM_COLS = 20;

typedef int Square;

// Byte 0 -- what occupies a space
enum Occupier : uint8_t {
  Empty   =  0,
  Player  = +1,
  Food    = +2
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

struct Snake {
	vector<Coord> coords;
	int health_points; // How much food each snake has remaining
	string id;
	Coord head() const { return coords.front(); }
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
};

typedef map<string, GameState> GameStates;

GameStates _states;
default_random_engine _rng;

enum class Move {
  Up, Down, Left, Right, None
};
typedef set<Move> Moves;

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

inline Occupier get_occupier(Board const& board, Coord const& c) {
  return static_cast<Occupier>(board[c.row][c.col] & 0x00000FF);
}

inline Owner get_owner(Board const& board, Coord const& c) {
  return static_cast<Owner>((board[c.row][c.col] & 0x0000FF00) >> 8);
}

inline Length get_length(Board const& board, Coord const& c) {
  return static_cast<Length>((board[c.row][c.col] & 0xFFFF0000) >> 16);
}

inline void set_occupier(Board& board, Coord const& c, Occupier occ) {
  board[c.row][c.col] &= 0xFFFF0000;
  board[c.row][c.col] |= static_cast<uint8_t>(occ);
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
      switch (get_occupier(board, c)) {
        case Empty:  cout << '_'; break;
        case Player: draw_colored('0' + get_length(board, c), get_owner(board, c)); break;
        case Food:   draw_colored('*', 1); break;
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
          set_occupier(board, c, Empty);
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
	set_occupier(state.board, Coord(5, 8), Food);

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

void filter_losing_collisions(GameState const& state, Moves& moves) {
	auto const& me = get_snake(state, state.me);

	// Don't go off edges
	if (me.head().row <= 0) moves.erase(Move::Up);
	if (me.head().col <= 0) moves.erase(Move::Left);
	if (me.head().row >= state.rows - 1) moves.erase(Move::Down);
	if (me.head().col >= state.cols - 1) moves.erase(Move::Right);

	// Don't collide with self
	for (auto const& body : me.coords) {
		if (body == rel_coord(me.head(), Move::Up)) moves.erase(Move::Up);
		if (body == rel_coord(me.head(), Move::Left)) moves.erase(Move::Left);
		if (body == rel_coord(me.head(), Move::Down)) moves.erase(Move::Down);
		if (body == rel_coord(me.head(), Move::Right)) moves.erase(Move::Right);
	}
}

Move get_move(GameState const& state, string& taunt) {
	// Start by considering all possible moves
	Moves moves = { Move::Up, Move::Down, Move::Left, Move::Right };

	// Remove any moves that would cause us to collide (and lose)
	// Walls, snake sides, and longer snake heads
	filter_losing_collisions(state, moves);

	// If we don't have any, we're boned!
	if (moves.empty()) {
		taunt = "Arghhh!";
		return Move::None;
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

	state.snakes.clear();
	for (auto const& s : j.at("snakes")) {
		state.snakes.push_back(process_snake(s));
	}
	state.me = j.at("you");

	return state;
}

void server() {
	HttpServer server;
	server.config.port = 5000;

	server.resource["^/start$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
		cout << "/start" << endl;

		auto j_in = json::parse(request->content.string());
		cout << j_in.dump(2) << endl;

		_states[j_in.at("game_id")] = GameState(j_in.at("height"), j_in.at("width"));

		json j_out;
		j_out["color"] = "#00ff00";
		j_out["name"] = "clifford";
		response->write(SimpleWeb::StatusCode::success_ok, j_out.dump(), { { "Content-Type", "application/json" } });
	};

	server.resource["^/move$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
		cout << "/move" << endl;

		auto j_in = json::parse(request->content.string());
		cout << j_in.dump(2) << endl;

		auto state = process_state(j_in);

		json j_out;
		string taunt;
		j_out["move"] = move_str(get_move(state, taunt));
		j_out["taunt"] = taunt;
		response->write(SimpleWeb::StatusCode::success_ok, j_out.dump(), { { "Content-Type", "application/json" } });
	};

	server.on_error = [](shared_ptr<HttpServer::Request> /*request*/, SimpleWeb::error_code const& ec) {
		cout << "Error: "  << ec.message() << endl;
	};

	thread server_thread([&server]() {
		server.start();
	});

	server_thread.join();
}

int main(int argc, const char* argv[]) {
	server();
	return 0;
}
