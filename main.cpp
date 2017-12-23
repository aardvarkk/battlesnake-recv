#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

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
typedef uint16_t Counter;

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
};

inline Occupier get_occupier(Board const& board, Coord const& c) {
  return static_cast<Occupier>(board[c.row][c.col] & 0x00000FF);
}

inline Owner get_owner(Board const& board, Coord const& c) {
  return static_cast<Owner>((board[c.row][c.col] & 0x0000FF00) >> 8);
}

inline Counter get_counter(Board const& board, Coord const& c) {
  return static_cast<Counter>((board[c.row][c.col] & 0xFFFF0000) >> 16);
}

inline void set_occupier(Board& board, Coord const& c, Occupier occ) {
  board[c.row][c.col] &= 0xFFFF0000;
  board[c.row][c.col] |= static_cast<uint8_t>(occ);
}

inline void set_owner(Board& board, Coord const& c, uint8_t owner) {
  board[c.row][c.col] &= 0xFFFF00FF;
  board[c.row][c.col] |= owner << 8;
}

inline void set_counter(Board& board, Coord const& c, uint16_t length) {
  board[c.row][c.col] &= 0x0000FFFF;
  board[c.row][c.col] |= length << 16;
  // cout << setw(8) << setfill('0') << hex << board[c.row][c.col] << endl;
}

void draw_colored(char c, int color_idx) {
  cout << "\033[" << Colors[color_idx] << "m" << c << "\033[0m";
}

void draw_board(Board const& board) {
  for (int i = 0; i < board.size(); ++i) {
    for (int j = 0; j < board[i].size(); ++j) {
      Coord c(i, j);
      switch (get_occupier(board, c)) {
        case Empty: cout << '_'; break;
        case Snake: draw_colored('0' + get_counter(board, c), get_owner(board, c)); break;
        case Food:  cout << '*'; break;
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

int main(int argc, const char* argv[]) {
  auto state = GameState(ROWS, COLS);
  state.food_left.resize(1);
  state.food_left[0] = 10;
  set_occupier(state.board, Coord(0, 0), Snake);
  set_counter(state.board, Coord(0, 0), 3);
  set_occupier(state.board, Coord(5, 8), Food);
  draw(state);

  return -1;
}
