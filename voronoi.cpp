#include "voronoi.h"
#include "util.h"

#include <deque>

using namespace std;

struct SnakeCoord {
	Coord c;       // The current coordinate
	int snake_idx; // Index of snake in game state
	Coords path;   // Path taken to get to this coord
};

// Start filling from each snake head
// Reduce counter on each iteration to account for tail movement
// Track optimal paths to get to each square per-snake
Voronoi voronoi(GameState const& state) {
	Voronoi v;
	v.board = Board(state.rows, state.cols);

	deque<SnakeCoord> accessible;

	int i = 0;
	for (auto const& s : state.snakes) {
		SnakeBoard sb;
		sb.head = s.head();
		sb.board = state.board;
		sb.dists = Board(state.rows, state.cols);
		sb.tracks.resize(state.rows);
		for (auto& track : sb.tracks) track.resize(state.cols);
		v.snake_boards.push_back(sb);

		SnakeCoord sc;
		sc.snake_idx = i++;
		sc.c = sb.head;
		accessible.push_back(sc);

		// We mark squares as visited when they get pushed onto the queue (so we don't add them multiple times)
		// But we do have to mark the start positions since we manually add them to the queue
		set_flag(sb.dists, sc.c, OccupierFlag::Visited);
	}

	// Number of turns it took to get here -- we're starting at heads, so it takes 0 turns to get here
	int level = 0;

	// Process a single "turn move" (one degree of flower fill from everything at this "level")
	while (!accessible.empty()) {
		deque<SnakeCoord> next_level;

		// Reduce all board counts so we account for tail movement
		for (auto& sb : v.snake_boards)
			stepdown_counter(sb.board);

//		for (auto const& sc : accessible)
//			cout << "Accessible " << sc.c << endl;

		// Process each point in the current level
		while (!accessible.empty()) {
			auto sc = accessible.front();
			accessible.pop_front();

			// Mark the number of turns it took to get here for the given snake
			auto& sb = v.snake_boards[sc.snake_idx];

			// Mark how many turns it took *this snake* to get here
			set_counter(sb.dists, sc.c, level);
			sb.access_area++;
			sb.avg_turns += level;

			// Mark the path that we took to get here
			sb.tracks[sc.c.row][sc.c.col] = sc.path;

			// Try out neighbours...
			for (auto const& m : AllMoves) {
				auto rel = rel_coord(sc.c, m);

				// Add to the queue for the next "level" if the point hasn't been visited by *this* snake
				// Not visited and in bounds and not occupied by an existing snake
				if (!in_bounds(rel, state)) continue;
				if (get_flag(sb.dists, rel, OccupierFlag::Visited)) continue;
				if (get_flag(sb.board, rel, OccupierFlag::Player)) continue;

				SnakeCoord next;
				next.snake_idx = sc.snake_idx;
				next.c = rel;
				next.path = sc.path;
				next.path.push_back(sc.c);

//				cout << "Added " << rel << endl;

				next_level.push_back(next);

				// Mark as visited so that we don't add the same coordinate within the inner loop
				set_flag(sb.dists, next.c, OccupierFlag::Visited);
			}
		}

		// Assign the next level of points to examine
		accessible = next_level;

		++level;
	}

	// Set up the final Voronoi board as the minimums for each snake board
	// If there's a tie, set Unowned (but still a minimum distance)
	uint8_t snake_count = state.snakes.size();

	for (int i = 0; i < state.rows; ++i) {
		for (int j = 0; j < state.cols; ++j) {

			// Go through all snake boards to find minimum
			int min_dist = INT_MAX;
			vector<uint8_t> owners;

			for (uint8_t s = 0; s < snake_count; ++s) {
				auto dist = get_counter(v.snake_boards[s].dists, i, j);

				// Only want to count the distance as valid if the snake actually visited
				if (!get_flag(v.snake_boards[s].dists, i, j, OccupierFlag::Visited)) continue;
				if (dist > min_dist) continue;

				if (dist < min_dist) {
					owners.clear();
					min_dist = dist;
				}
				owners.push_back(s);
			}

			set_counter(v.board, i, j, min_dist);

			// No snake has access to this square
			if (owners.empty()) {
				set_owner(v.board, i, j, Unowned);
				set_flag(v.board, i, j, OccupierFlag::Inaccessible);
			}
				// One snake has a fastest path to this square
			else if (owners.size() == 1) {
				int idx = *owners.begin();
				set_owner(v.board, i, j, idx);
				v.snake_boards[idx].first_area++;
			}
				// Multiple snakes can get to the same spot at the same time
			else {
				set_owner(v.board, i, j, Unowned);
			}
		}
	}

	// Post-process the stats for the snakes
	for (auto& sb : v.snake_boards) {
		if (sb.access_area > 0)
			sb.avg_turns /= sb.access_area;
	}

	return v;
}
