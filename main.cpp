#include <cstdint>
#include <iostream>
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

typedef vector< vector<Square> > GameState;

enum class Move {
  Up, Down, Left, Right
};

inline Occupier get_occupier(GameState const& state, int i, int j) {
  return static_cast<Occupier>(state[i][j] & 0x000F);
}

inline Owner get_owner(GameState const& state, int i, int j) {
  return static_cast<Owner>((state[i][j] & 0x00F0) >> 8);
}

inline Counter get_counter(GameState const& state, int i, int j) {
  return static_cast<Counter>((state[i][j] & 0xFF00) >> 16);
}

inline void set_occupier(GameState& state, int i, int j, Occupier occ) {
  state[i][j] &= 0xFFF0;
  state[i][j] |= static_cast<uint8_t>(occ);
}

inline void set_owner(GameState& state, int i, int j, uint8_t owner) {
  state[i][j] &= 0xFF0F;
  state[i][j] |= owner << 8;
}

inline void set_counter(GameState& state, int i, int j, uint16_t length) {
  state[i][j] &= 0x00FF;
  state[i][j] |= length << 16;
}

void draw(GameState const& state) {
  for (int i = 0; i < state.size(); ++i) {
    for (int j = 0; j < state[i].size(); ++j) {
      char c;
      switch (get_occupier(state, i, j)) {
        case Empty: c = '_'; break;
        case Snake: c = '0' + get_owner(state, i, j); break;
        case Food:  c = '*'; break;
        default: c = '?';
      }
      cout << c;
    }
    cout << endl;
  }
}
int main(int argc, const char* argv[]) {

  auto board = GameState();
  board.resize(ROWS);
  for (int i = 0; i < COLS; ++i) {
    board[i] = vector<Square>(COLS);
  }
  set_occupier(board, 0, 0, Snake);
  set_occupier(board, 5, 8, Food);
  draw(board);

  return -1;
}
