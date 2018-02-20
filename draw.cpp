#include <climits>

#include "draw.h"
#include "util.h"

using namespace std;

const int NumColors = 14;
const string Colors[NumColors] = {
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

string get_color(int idx) {
	return Colors[idx % NumColors];
}

void draw_colored(char c, int color_idx) {
	cout << "\033[" << get_color(color_idx) << "m" << c << "\033[0m";
}

void draw_board(GameState const& state) {
	auto const& board = state.board;

	vector<vector<pair<char, int>>> to_draw;
	to_draw.resize(board.size());
	for (auto& row : to_draw) row.resize(board.front().size());

	for (int i = 0; i < board.size(); ++i) {
		for (int j = 0; j < board[i].size(); ++j) {
			Coord c(i, j);
			if (get_is_empty(board, c)) {
				to_draw[i][j] = make_pair('_', NumColors-1);
			} else if (get_flag(board, c, OccupierFlag::Player)) {
				to_draw[i][j] = make_pair('0' + get_owner(board, c), get_owner(board, c));
			} else if (get_flag(board, c, OccupierFlag::Food)) {
				to_draw[i][j] = make_pair('*', 1);
			} else {
				to_draw[i][j] = make_pair('?', NumColors-1);
			}
		}
	}

	for (auto const& snake : state.snakes) {
		Coord h(snake.head().row, snake.head().col);
		auto prev = h;
		for (auto it = snake.coords.begin()+1; it != snake.coords.end(); ++it) {
			switch (rel_dir(*it, prev)) {
				case Move::Up:    to_draw[it->row][it->col].first = '^'; break;
				case Move::Down:  to_draw[it->row][it->col].first = 'v'; break;
				case Move::Left:  to_draw[it->row][it->col].first = '<'; break;
				case Move::Right: to_draw[it->row][it->col].first = '>'; break;
				default: break;
			}
			prev = *it;
		}
		to_draw[h.row][h.col] = make_pair('0' + get_owner(board, h), get_owner(board, h));
	}

	for (int i = 0; i < board.size(); ++i) {
		for (int j = 0; j < board[i].size(); ++j) {
			draw_colored(to_draw[i][j].first, to_draw[i][j].second);
		}
		cout << endl;
	}
	cout << endl;
}

void draw(GameState const& state) {
	int i = 0;
	for (auto const& snake : state.snakes) {
		cout << i++ << ": " << snake.health << " ";
	}
	cout << endl;
	draw_board(state);
}

void draw_labels(LabelledBoard const& labels) {
	vector<vector<pair<char, int>>> to_draw;
	to_draw.resize(labels.size());
	for (auto& row : to_draw) row.resize(labels.front().size());

	for (int i = 0; i < labels.size(); ++i) {
		for (int j = 0; j < labels[i].size(); ++j) {
			if (get_flag(labels, i, j, OccupierFlag::Inaccessible)) {
				to_draw[i][j] = make_pair('x', NumColors-1);
				continue;
			}

			if (get_flag(labels, i, j, OccupierFlag::CutVert)) {
				to_draw[i][j] = make_pair('!', get_owner(labels, i, j));
				continue;
			}

			to_draw[i][j] = make_pair('0' + get_owner(labels, i, j), get_owner(labels, i, j));
		}
	}

	for (int i = 0; i < labels.size(); ++i) {
		for (int j = 0; j < labels[i].size(); ++j) {
			draw_colored(to_draw[i][j].first, to_draw[i][j].second);
		}
		cout << endl;
	}
	cout << endl;
}

void draw_voronoi(Voronoi const& v)
{
	vector<vector<pair<char, int>>> to_draw;
	to_draw.resize(v.board.size());
	for (auto& row : to_draw) row.resize(v.board.front().size());

	for (int i = 0; i < v.board.size(); ++i) {
		for (int j = 0; j < v.board[i].size(); ++j) {
			if (get_flag(v.board, i, j, OccupierFlag::Inaccessible)) {
				to_draw[i][j] = make_pair('x', NumColors-1);
				continue;
			}

			uint8_t owner = get_owner(v.board, i, j);
			int color_idx = owner == Unowned ? NumColors-1 : owner;
			to_draw[i][j] = make_pair('0' + get_counter(v.board, i, j), color_idx);
		}
	}

	for (int i = 0; i < v.board.size(); ++i) {
		for (int j = 0; j < v.board[i].size(); ++j) {
			draw_colored(to_draw[i][j].first, to_draw[i][j].second);
		}
		cout << endl;
	}
	cout << endl;
}