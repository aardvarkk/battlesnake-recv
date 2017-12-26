#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "server_http.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
using namespace boost::property_tree;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

using namespace std;

const int ROWS = 20;
const int COLS = 20;

typedef int Square;

// Byte 0 -- what occupies a space
enum Occupier : uint8_t {
  Empty   =  0,
  Snake   = +1,
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

  int row;
  int col;
};

struct GameState {
  GameState(int rows, int cols) {
    board.resize(rows);
    for (int i = 0; i < cols; ++i) {
      board[i] = vector<Square>(cols);
    }
  }

  Board board; // Board state
  vector<int> food_left; // How much food each snake has remaining
  vector<Coord> heads; // Where the "heads" of the snakes are (so it's quick to move them around)
};

enum class Move {
  Up, Down, Left, Right
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
        case Empty: cout << '_'; break;
        case Snake: draw_colored('0' + get_length(board, c), get_owner(board, c)); break;
        case Food:  draw_colored('*', 1); break;
        default: cout << '?';
      }
    }
    cout << endl;
  }
}

void draw(GameState const& state) {
  for (int i = 0; i < state.food_left.size(); ++i) {
    cout << i << ": " << state.food_left[i] << " ";
  }
  cout << endl;
  draw_board(state.board);
}

void add_snake(GameState& state, Coord const& c, int length, int food) {
  int idx = state.heads.size();
  state.heads.push_back(c);
  state.food_left.push_back(food);
  set_occupier(state.board, state.heads[idx], Snake);
  set_length(state.board, state.heads[idx], length);
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
  for (int i = 0; i < state.food_left.size(); ++i) {
    state.food_left[i]--;
  }
}

Coord rel_coord(Coord const& in, Move dir) {
  switch (dir) {
    case Move::Up:    return Coord(in.row - 1, in.col);
    case Move::Down:  return Coord(in.row + 1, in.col);
    case Move::Left:  return Coord(in.row, in.col - 1);
    case Move::Right: return Coord(in.row, in.col + 1);
  }
}

inline string move_str(Move const& dir) {
	switch (dir) {
		case Move::Up:    return "up";
		case Move::Down:  return "down";
		case Move::Left:  return "left";
		case Move::Right: return "right";
	}
}

// TODO: Deal with collisions, starvation, etc. (all game logic!)
// TODO: When do you die of starvation? If you have no food at start of move or end?
GameState run_moves(GameState const& in, vector<Move> const& moves) {
  GameState out = in;

  for (int i = 0; i < moves.size(); ++i) {
    auto to = rel_coord(in.heads[i], moves[i]);
    set_occupier(out.board, to, Snake);
    set_owner(out.board, to, i);
    set_length(out.board, to, get_length(out.board, in.heads[i]) + 1);
    out.heads[i] = to;
  }

  stepdown_length(out.board);
  stepdown_food(out);

  return out;
}

GameState random_step(GameState const& in) {
  return run_moves(in, { Move::Down });
}

void simulate() {
	auto state = GameState(ROWS, COLS);
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

void send_json(ptree const& json, shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> &response) {
	SimpleWeb::CaseInsensitiveMultimap header;
	header.emplace("Content-Type", "application/json");
	stringstream ss;
	write_json(ss, json);
	response->write(SimpleWeb::StatusCode::success_ok, ss, header);
}

Move get_move(ptree const& json) {
	return Move::Right;
}

void server() {
	HttpServer server;
	server.config.port = 5000;

	server.resource["^/start$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
		cout << "/start" << endl;
		ptree json;
		json.put<string>("color", "#00ff00");
		json.put<string>("name", "clifford");
		send_json(json, response);
	};

	server.resource["^/move$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
		cout << "/move" << endl;

//		string method, path, query_string, version;
//		SimpleWeb::CaseInsensitiveMultimap header;
//		SimpleWeb::RequestMessage::parse(request->content, method, path, query_string, version, header);

		ptree json_in;
		read_json(request->content, json_in);
		print_ptree(json_in);

		ptree json_out;
		json_out.put<string>("move", move_str(get_move(json_in)));
		send_json(json_out, response);
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
