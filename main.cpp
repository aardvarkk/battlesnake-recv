#include <algorithm>
#include <cfloat>
#include <climits>
#include <iomanip>
#include <iostream>
#include <deque>
#include <queue>
#include <random>
#include <set>

// RULES:
// - If two snakes of same length hit each other they both die

// TODO:

// - If unable to access food, check HOW MANY TURNS until we can and then see where the newly "opened" square is
//  - Make it a goal to get to that square with exactly the right number of turns left?

// - If enemy snake is "committed" to a line we can cut off in several moves, cut them off!

// - If we can limit the opponent area severely, do it (i.e. lock them against a wall if possible)

// - If all players are too far away from food to live (and we have enough), let them starve and DON'T eat the food
// Just surround it until they die

// - If we can "race" all other snakes to food (guaranteed available path to reach it AND escape?) we should probably do it
// Check taxicab distance from all snakes to food and try to keep ourselves 1 turn ahead (keep their taxicab distances to food at ours + 1 at a minimum)

// - Never move such that the best-case (tails disappearing) NEW area we WOULD CREATE with our move is less than the amount we need for our length
// This is just pushing forward our area check by another turn

// - We'll surround food with our own body such that once we get it we have nowhere to go *after*
// It's OK to leave food until near-starvation, but we have to make damn sure we can get OUT after getting the food!

// - Subtract a value from the "counter" to see how many turns we have to "stall" until we'd get full access to different areas

// - Move to the space your opponent wants in TWO turns (unless you are longer, in which case move for the one turn and if he collides he dies!)

// - Don't leave "no escape" -- if we are long enough to fill the board top to bottom and food is on the wrong side of us then we're bound to die (in single player)

// - Concept of "ideal" area -- take square root of our length and that's the space we NEED

// - AVOID moves such that our food target gets pushed inside an area we can't fit in

// - Dying of starvation because we can't get to the next food instance in time
//  - Don't enter an area WITHOUT FOOD where the area doesn't give access to food in the next HEALTH POINT turns
//  - What maximizes our chances of getting to new food? Want most possible paths, which means our body should stay "tight" and occupy as little radius/w*h as possible

// - TODO: Don't collide with longer snake heads (but don't know where they're going!)

// - You can perfectly encircle if your length is even -- for instance, with length 8 you can perfectly surround food
// If odd, you can mostly encircle (but space between your tail and head)
// To encircle, just try to be a rectangle at first (until we get so big that we have to cover more area)

// - FEATURES TO CONSIDER:
//  - If we detect a snake in a situation where they're guaranteed to starve (can't exit an area without food and has no health points), then wait them out!
//  - Do we have the most/least health points of all remaining snakes? Where are we in the rankings?
//  - Are we the closest/farthest from food of all remaining snakes?
//  - Are we the shortest/longest of all remaining snakes?
//  - Do we have the most/least possible area to move to of all remaining snakes?
//  - Choke points -- squares to occupy that would drastically reduce our opponent's movable area
//   - How could we find these? One hint is REDUCING NUMBER OF OPTIONS -- look for snakes that only have a few moves available and occupy those lines
//  - Maximum branching -- go direction that continues to open up multiple options as we expand (lots of opportunity)

// - Really, we're trying to maximize our turns remaining while minimizing everybody else's
//  - Maximum of turns remaining is just health remaining -- aren't really guaranteed any more than that
//  - Minimum is 0 (no possible move left to not die on the next turn)

// - Don't enter areas where you're liable to be "cut off", even if you seem to have enough area to be contained
//  - Area with lots of "cut vertices" or "articulation points" would be problematic if your opponent can reach them first
//  - TRY to reach articulation points before your opponent if you can cut them off?

// - Random...
// Flood fill out for min travellable dist check (ignore any movement of own tail? and other players and make sure we can exit any halls we enter)
// Neural network for ourselves based on relative square around us
//	Connectivity of layers based on physical connections?
// Try to regress on number of turns remaining
// Model for ourself and use same network to model for each other player
//	Want move that has biggest delta change between our number of remaining turns and some function of all other players (min?)
// Find/check shortest navigable path to food — can be flood filled since we should never return to same location on the way to food. Do this instead of taxicab distance since it accounts for having to move around our own body. Assume our tail does move. Mark off places we’ve been so they don’t reflood.
// To and from JSON for game state

using namespace std;

#include "httplib.h"
using namespace httplib;

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "argh.h"
#include "draw.h"
#include "util.h"
#include "voronoi.h"

default_random_engine _rng;
bool draw_enabled;

Snake get_snake(GameState const& state, string const& id) {
	for (auto const& s : state.snakes) {
		if (s.id == id) return s;
	}
	return Snake();
}

void filter_wall_collisions(GameState const& state, Moves& moves) {
	// Don't go off edges
	if (state.me.head().row <= 0) {
		moves.erase(Move::Up);
		cout << "Can't move up because we'd go off the map" << endl;
	}
	if (state.me.head().col <= 0) {
		moves.erase(Move::Left);
		cout << "Can't move left because we'd go off the map" << endl;
	}
	if (state.me.head().row >= state.rows - 1) {
		moves.erase(Move::Down);
		cout << "Can't move down because we'd go off the map" << endl;
	}
	if (state.me.head().col >= state.cols - 1) {
		moves.erase(Move::Right);
		cout << "Can't move right because we'd go off the map" << endl;
	}
}

// Don't collide with "bodies"
// Tail is OK (counter of 1) because it will be gone next turn
// We don't know where heads are going to be
// But anything else with a counter > 1 is either our body or another snake's
void filter_body_collisions(GameState const& state, Moves& moves) {
	Moves filtered;
	for (auto const& m : moves) {
		if (get_counter(state.board, rel_coord(state.me.head(), m)) > 1) {
			cout << "Can't move " << move_str(m) << " because we'd collide with a body" << endl;
		} else {
			filtered.insert(m);
		}
	}
	moves = filtered;
}

void filter_possible_head_collisions(GameState const& state, Moves& moves) {
	Moves filtered;
	for (auto const& m : moves) {

		// Get the next location of our head given this move
		auto our_next_head = rel_coord(state.me.head(), m);
		bool possible_collision = false;

		// Go through every snake
		for (auto const& s : state.snakes) {

			// Don't compare to ourselves
			if (s.id == state.my_id) continue;

			// Only care about snakes >= our length
			if (s.length() < state.me.length()) continue;

			// If any of their head moves would collide with our head move, filter it out
			for (auto const& their_move : AllMoves) {
				auto their_next_head = rel_coord(s.head(), their_move);
				if (our_next_head == their_next_head) {
					cout << "Ignoring move " << move_str(m) << " because we might collide with longer snake head" << endl;
					possible_collision = true;
					break;
				}
			}

			if (possible_collision) {
				break;
			}
		}

		// No possible collisions
		if (!possible_collision) {
			filtered.insert(m);
		}
	}
	moves = filtered;
}

// Valid children are:
// 1. Not already visited
// 2. Not occupied by a non-tail player
void add_valid_children(Coord const& c, GameState& state, deque<Coord>& children) {
	for (auto const& m : AllMoves) {
		auto child = rel_coord(c, m);
		if (!in_bounds(child, state)) continue;
		if (get_flag(state.board, child, OccupierFlag::Visited)) continue;
		if (get_flag(state.board, child, OccupierFlag::Player)) continue;
		set_flag(state.board, child, OccupierFlag::Visited);
		children.push_back(child);
	}
}
// Measures turn distance from source to destination by marking sections of the game board as Visited
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

		// Every turn all tails will move so we can access more stuff
		stepdown_counter(state.board);

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
void filter_starvation(
	GameState const& state,
	map<Move, int> const& move_food_dists,
	Moves& moves
)
{
	Moves filtered;
	bool clear_path_exists = false;
	for (auto const& pr : move_food_dists) {
		if (pr.second < INT_MAX) {
			clear_path_exists = true;
		}

		if (pr.second >= state.me.health) {
			cout << "Unlikely we can go " << move_str(pr.first) << " to food at dist " << pr.second << " with only " << state.me.health << " health" << endl;
		} else {
			filtered.insert(pr.first);
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

// Measure the total area available to us as turns progress
int accessible_area(GameState const &const_state, Coord const& start) {
	int area = 0;

	if (!in_bounds(start, const_state)) return area;

	auto state = const_state;

	deque<Coord> unvisited;
	unvisited.push_back(start);
	set_flag(state.board, start, OccupierFlag::Visited);

	while (!unvisited.empty()) {

		// Holder for "next round"
		deque<Coord> children;

		// Go through all the coords at the current level and check them
		while (!unvisited.empty()) {

			auto c = unvisited.front();
			unvisited.pop_front();

			add_valid_children(c, state, children);

			// Food spot is a null op, because we'll grow by one to use it
			if (get_flag(state.board, c, OccupierFlag::Food)) --area;

			++area;
		}

		unvisited = children;
		stepdown_counter(state.board);
	}

	return area;
}

Move check_last_resorts(Moves const& moves) {
	if (moves.empty()) {
		cout << "No remaining moves!" << endl;
		return random_move(AllMoves);
	} else if (moves.size() == 1) {
		cout << "Only one non-death move, so we'll make it" << endl;
		return *moves.begin();
	}

	return Move::None;
}

bool is_longest_snake(Snake const& snake, GameState const& state) {
	for (auto const& other_snake : state.snakes) {
		if (other_snake.id != snake.id && other_snake.length() >= snake.length()) return false;
	}
	return true;
}

bool is_healthiest_snake(Snake const& snake, GameState const& state) {
	for (auto const& other_snake : state.snakes) {
		if (other_snake.id != snake.id && other_snake.health >= snake.health) return false;
	}
	return true;
}

int get_move_fatness(Snake const& snake, Move const& m, GameState const& const_state) {
	auto next_move = rel_coord(snake.head(), m);
	int min_r, max_r, min_c, max_c;
	min_r = max_r = next_move.row;
	min_c = max_c = next_move.col;

	for (auto const& c : snake.coords) {
		min_r = min(min_r, c.row);
		max_r = max(max_r, c.row);
		min_c = min(min_c, c.col);
		max_c = max(max_c, c.col);
	}

	return (max_r - min_r + 1) * (max_c - min_c + 1);
}

Move get_tight_body_move(
	GameState const& state,
	Moves const& moves,
	map<Move, int> const& move_food_dists
) {
	auto cur_fatness = get_move_fatness(state.me, Move::None, state);

	// Tighten the body...
	Move chosen = Move::None;
	int min_fatness = INT_MAX;
	int min_dist_to_food = INT_MAX; // Secondary criteria
	for (auto const& m : moves) {
		auto fatness = get_move_fatness(state.me, m, state);
		if (fatness < min_fatness) {
			min_fatness = fatness;
			chosen = m;
			cout << "Found new best fatness of " << min_fatness << " by going " << move_str(m) << endl;
		} else if (fatness == min_fatness && move_food_dists.at(m) < min_dist_to_food) {
			min_dist_to_food = move_food_dists.at(m);
			cout << "Found same fatness of " << min_fatness << " but with new best food dist of " << min_dist_to_food << endl;
			chosen = m;
		}
	}

	if (min_fatness >= cur_fatness) {
		return Move::None;
	} else {
		return chosen;
	}
}

// Return a labelled board along with labels of the different areas
// https://en.wikipedia.org/wiki/Connected-component_labeling
pair< LabelledBoard, vector<Coords> > connected_areas(Board const& board) {
	LabelledBoard lb = board;
	for (auto i = 0; i < lb.size(); ++i) {
		for (auto j = 0; j < lb[i].size(); ++j) {
			lb[i][j] = INT_MAX;
		}
	}

	// First pass -- assign temporary labels
	int label = 0;
	map<int, set<int> > label_equiv;
	for (auto i = 0; i < lb.size(); ++i) {
		for (auto j = 0; j < lb[i].size(); ++j) {
			if (!get_is_enterable(board, i, j)) continue;

			int north_neighbour_label = INT_MAX;
			int west_neighbour_label = INT_MAX;

			if (i > 0) {
				north_neighbour_label = lb[i - 1][j];
			}
			if (j > 0) {
				west_neighbour_label = lb[i][j - 1];
			}
			int min_neighbour_label = min(north_neighbour_label, west_neighbour_label);

			if (min_neighbour_label < INT_MAX) {
				if (north_neighbour_label < INT_MAX && north_neighbour_label != min_neighbour_label) {
					label_equiv[north_neighbour_label].insert(min_neighbour_label);
				}
				if (west_neighbour_label < INT_MAX && west_neighbour_label != min_neighbour_label) {
					label_equiv[west_neighbour_label].insert(min_neighbour_label);
				}
				lb[i][j] = min_neighbour_label;
			} else {
				lb[i][j] = label;
				++label;
			}
		}
	}

//	cout << "Initial Equivalents" << endl;
//	for (auto const& src_label : label_equiv) {
//		for (auto const& equiv : src_label.second) {
//			cout << src_label.first << " -> " << equiv << endl;
//		}
//	}

	// Reduce the label equivalency to the minimum
	map<int, int> final_equiv;
	for (int i = 0; i < label; ++i) {
		final_equiv[i] = i;
	}
	for (auto const& equiv : label_equiv) {
		int this_equiv = equiv.first;
		while (!label_equiv[this_equiv].empty()) {
			this_equiv = *label_equiv[this_equiv].begin();
		}
		final_equiv[equiv.first] = this_equiv;
	}

//	cout << "Finished Equivalents" << endl;
//	for (auto const& eq : final_equiv) {
//		cout << eq.first << " -> " << eq.second << endl;
//	}

	// Generate label names that are in numerical without skipping values
	// We have determined which labels are equivalent, but we want to re-map so we don't skip values
	set<int> equiv_idxs;
	for (auto const& eq : final_equiv) {
		equiv_idxs.insert(eq.second);
	}

	label = 0;
	map<int, int> final_labels;
	for (auto const& final_equiv : equiv_idxs) {
		final_labels[final_equiv] = label++;
	}

//	cout << "Label Mapping" << endl;
//	for (auto const& lbs: final_labels) {
//		cout << lbs.first << " -> " << lbs.second << endl;
//	}

	// Second pass -- replace labels with equivalents and generate coord lists
	vector<Coords> areas;
	for (auto i = 0; i < lb.size(); ++i) {
		for (auto j = 0; j < lb[i].size(); ++j) {
			if (lb[i][j] == INT_MAX) {
				lb[i][j] = 0;
				set_flag(lb, i, j, OccupierFlag::Inaccessible);
				continue;
			}

			int final_label = final_labels[final_equiv[lb[i][j]]];
			set_flag(lb, i, j, OccupierFlag::Accessible);
			set_owner(lb, i, j, final_label);

			if (final_label >= areas.size()) areas.resize(final_label+1);
			areas[final_label].push_back(Coord(i,j));
		}
	}

	return make_pair(lb, areas);
}

// Try to calculate articulation points
// They may not be accurate until we refine them
// Two possibilities:
// --- -o-
// o!o -!-
// --- -o-
// where ! is the articulation point
void possible_articulation_points(GameState const& state, LabelledBoard& lb) {
	auto const& board = state.board;
	for (auto i = 0; i < board.size(); ++i) {
		for (auto j = 0; j < board[i].size(); ++j) {
			bool ok = in_bounds(i, j, state) && get_is_enterable(board, i, j);
			if (!ok) continue;

			bool up_ok    = in_bounds(i-1, j,   state) && get_is_enterable(board, i-1, j);
			bool down_ok  = in_bounds(i+1, j,   state) && get_is_enterable(board, i+1, j);
			bool left_ok  = in_bounds(i,   j-1, state) && get_is_enterable(board, i,   j-1);
			bool right_ok = in_bounds(i,   j+1, state) && get_is_enterable(board, i,   j+1);

			bool articulation = (ok && left_ok && right_ok && !up_ok && !down_ok) ||
				(ok && down_ok && up_ok && !left_ok && !right_ok);

			if (articulation)
				set_flag(lb, i, j, OccupierFlag::CutVert);
		}
	}
}

struct NeighbourResult {
	NeighbourResult() : articulation(false), containing_area(0), open_neighbours(0) {}
	Move move;
	bool articulation;
	int containing_area;
	int open_neighbours;
};

bool is_articulation(Board const& board, vector<Coords> const& base, Coord const& c) {
	// Copy the board
	Board board_copy = board;

	// Fake that the position in question is occupied
	set_flag(board_copy, c, OccupierFlag::Inaccessible);

	// Calculate connected components
	auto cas = connected_areas(board_copy);
//	draw_labels(cas.first);

	// Has the number of connected components changed? If so, it's an articulation point!
	return cas.second.size() != base.size();
}

// Take the move whose destination has the fewest number of neighbours
// But make sure we have at least ONE neighbour to continue into!
Move space_fill_by_heuristic(GameState const& state, Moves const& moves) {
	// Go through each connected area (graph) and find articulation points
	auto ccs = connected_areas(state.board);
//	possible_articulation_points(state, ccs.first);
	if (draw_enabled) draw_labels(ccs.first);

	// Accessible areas for different moves
	vector<NeighbourResult> neighbour_results;

	for (auto const& m : moves) {
		auto target = rel_coord(state.me.head(), m);

		NeighbourResult res;
		for (auto const& nm : AllMoves) {
			auto n = rel_coord(target, nm);
			if (in_bounds(n, state) && get_is_enterable(state.board, n))
				res.open_neighbours++;
		}

		res.move = m;
		res.articulation = is_articulation(state.board, ccs.second, target);
		res.containing_area = static_cast<int>(ccs.second[get_owner(ccs.first, target)].size());

		neighbour_results.push_back(res);
	}

	// Sort first by articulation points (don't want to take them!), and then by resulting area (want max!)
	sort(neighbour_results.begin(), neighbour_results.end(),
			 [](NeighbourResult const& a, NeighbourResult const& b) {
				 if (!a.articulation && b.articulation) return true;
				 else if (a.articulation && !b.articulation) return false;
				 else if (a.containing_area != b.containing_area) return a.containing_area > b.containing_area;
				 else return a.open_neighbours < b.open_neighbours;
	});

	cout << "Sorted space fill results" << endl;
	for (auto const& r : neighbour_results) {
		cout << move_str(r.move) << " articulation " << r.articulation << " in area " << r.containing_area << " with " << r.open_neighbours << " neighbours" << endl;
	}

	return neighbour_results.begin()->move;
}

map<Move, int> calculate_move_areas(GameState const& state, Moves const& moves) {
	map<Move, int> move_areas;
	for (auto const& m : moves) {
		auto area = accessible_area(state, rel_coord(state.me.head(), m));
		move_areas[m] = area;
	}
	return move_areas;
}

// For each of the moves, flood fill and count the area available in that direction
// If the area isn't big enough to hold our length, don't go into it
void filter_inadequate_areas(GameState const& state, map<Move, int> const& move_areas, Moves& moves) {
	Moves filtered;
	for (auto const& pr : move_areas) {
		if (pr.second < state.me.length()) {
			cout << "Can't move " << move_str(pr.first) << " since we can't fit length " << state.me.length() << " in area " << pr.second << endl;
		} else {
			filtered.insert(pr.first);
		}
	}
	moves = filtered;
}

bool alone(GameState const& state) {
	// If any of our accessible neighbours are accessible by another snake, we're not alone
	set<int> our_areas, their_areas;

	auto cca = connected_areas(state.board);

	for (auto const& snake : state.snakes) {
		auto& accessible_areas = snake.id == state.my_id ? our_areas : their_areas;
		for (auto const& m : AllMoves) {
			auto n = rel_coord(snake.head(), m);
			if (in_bounds(n, state) && get_flag(cca.first, n, OccupierFlag::Accessible))
				accessible_areas.insert(get_owner(cca.first, n));
		}
	}

	set<int> intersect_areas;
	set_intersection(our_areas.begin(), our_areas.end(),
									 their_areas.begin(), their_areas.end(),
									 inserter(intersect_areas, intersect_areas.begin())
	);

	return intersect_areas.empty();
}

// Only look one move ahead to see which move gives us best area access
Move greedy_max_control_area(GameState const& state, Moves const& moves) {
	int best_access_area = 0;
	int best_first_area = 0;
	float best_avg_turns = FLT_MAX;
	set<Move> best_moves;

	cout << "Greedily chosing move based on control area" << endl;

	for (auto const& m : moves) {

		// Simulate moving to the point
		auto state_copy = state;
		auto rel = rel_coord(state_copy.me.head(), m);
		state_copy.snakes[state_copy.me.idx].coords.push_front(rel);
		set_owner(state_copy.board, rel, state_copy.me.idx);
		set_flag(state_copy.board, rel, OccupierFlag::Player);

		// Calculate Voronoi result
		auto v = voronoi(state_copy);

		int access_area = v.snake_boards[state_copy.me.idx].access_area;
		int first_area  = v.snake_boards[state_copy.me.idx].first_area;
		float avg_turns = v.snake_boards[state_copy.me.idx].avg_turns;

		// TODO: Can probably be more clever about this...

		// Priority 1 -- keep access to as much area as possible (whether first or not)
		if (access_area > best_access_area) {
			best_moves.clear();
			best_moves.insert(m);
			best_access_area = access_area;
			best_first_area = first_area;
			best_avg_turns = avg_turns;

			cout << "Found best access area of " << best_access_area << " when moving " << move_str(m) << endl;
		} else if (access_area == best_access_area) {

			// Priority 2 -- keep access to as much first area as possible
			if (first_area > best_first_area) {
				best_moves.clear();
				best_moves.insert(m);
				best_first_area = first_area;
				best_avg_turns = avg_turns;

				cout << "Found better first area of " << best_first_area << " when moving " << move_str(m) << endl;
			} else if (first_area == best_first_area) {

				// Priority 3 -- keep average number of turns as low as possible
				if (avg_turns < best_avg_turns) {
					best_moves.clear();
					best_moves.insert(m);
					best_avg_turns = avg_turns;

					cout << "Found better avg turns of " << best_avg_turns << " when moving " << move_str(m) << endl;
				}
			}
		}
	}

	Move chosen;
	if (best_moves.size() == 1) {
		chosen = *best_moves.begin();
		cout << "One best option of " << move_str(chosen) << endl;
	} else {
		chosen = random_move(best_moves);
		cout << "Moving " << move_str(chosen) << " as chosen randomly from " << best_moves.size() << " best options" << endl;
	}
	return chosen;
}

Move get_move(GameState const& state) {
	// Start by considering all possible moves
	Moves moves = AllMoves;

	// Convenience variable
	Moves pre_filtered;

	// region Hard filter remove collisions with walls (guaranteed loss)
	filter_wall_collisions(state, moves);
	{
		auto chosen = check_last_resorts(moves);
		if (chosen != Move::None) return chosen;
	}
	// endregion

	// region Hard filter collisions with snake bodies (guaranteed loss)
	filter_body_collisions(state, moves);
	{
		auto chosen = check_last_resorts(moves);
		if (chosen != Move::None) return chosen;
	}
	// endregion

	// There's no food, so position strategically (Tron snake variant)
	if (state.food.empty()) {
		cout << "There's no food, so we're behaving like a TRON snake" << endl;

		auto v = voronoi(state);
		if (draw_enabled) draw_voronoi(v);

		// PHASE 1 - We're in the same connected area as our opponent, so play for position
		if (!alone(state)) {
			cout << "We're not alone! Strategize!" << endl;

			// Filter to battlefront points (farthest we can get, guaranteed, before enemies)

			// Check battlefront points to see if any *conclusively* limit opponent area to less than ours
			// If so, go there!

			// If not, greedily move to point that gives us largest resulting max Voronoi control area

			return greedy_max_control_area(state, moves);
		}
		// PHASE 2 - We're in a separate area from our opponent, so just space fill...
		else {
			cout << "We're alone, so we'll space fill by heuristic (avoiding cut vertices)" << endl;
			return space_fill_by_heuristic(state, moves);
		}
	}
	// There's food available, so strategize (Normal variant)
	else {
		// region Soft filter head collisions - if all we have remaining are head collisions, though, just ignore them!
		// TODO: Probabilistically choose the most likely to be empty?
		pre_filtered = moves;
		filter_possible_head_collisions(state, moves);
		if (moves.empty()) {
			cout << "Filtered out all moves with possible head collisions, so ignoring them..." << endl;
			moves = pre_filtered;
		}
		// endregion

		// region Soft filter entering areas we can't fit into
		pre_filtered = moves;
		auto move_areas = calculate_move_areas(state, moves);
		filter_inadequate_areas(state, move_areas, moves);
		if (moves.empty()) {
			cout << "Filtered out all moves with inadequate move areas, so ignoring them..." << endl;
			moves = pre_filtered;
		}
		// endregion

		// region Calculate best-case distance to food for each move remaining
		// We'll use this for several calculations
		map<Move, int> move_food_dists;
		for (auto const& m : moves) {
			move_food_dists[m] = min_dist_to_food(rel_coord(state.me.head(), m), state);
		}
		// endregion

		// region Strongly prefer to not move farther away from food than our health allows,
		// but we can hope that another snake eats food and the new food regenerates closer to us
		{
			Moves pre = moves;
			filter_starvation(state, move_food_dists, moves);

			// We have no good options (very likely to die, but tiny chance of something else...)
			if (moves.empty()) {
				cout << "We'll starve no matter which move we make, so picking a random one..." << endl;
				return random_move(pre);
			}
				// We only have one possible move, so do it
			else if (moves.size() == 1) {
				cout << "Only one move to avoid starvation, so using it!" << endl;
				return *moves.begin();
			}
		}
		// endregion

		// We have a list of moves to choose from...

		// If we're the longest snake with the most health...
		if (is_longest_snake(state.me, state) && is_healthiest_snake(state.me, state)) {
			cout << "We are in great shape... longest and healthiest" << endl;

			cout << "Targeting keeping a tight body" << endl;

			// region Target tight body
			auto tight_body_move = get_tight_body_move(state, moves, move_food_dists);
			if (tight_body_move != Move::None) {
				cout << "Using a tight body move we found!" << endl;
				return tight_body_move;
			} else {
				cout << "No improvement in fatness move found, so choosing random move!" << endl;
				return random_move(moves);
			}
			// endregion
		} else {
			cout << "We are not longest and healthiest, so targeting food!" << endl;

			// region Target food
			int min_dist = INT_MAX;
			Moves food_moves;
			for (auto const& pr : move_food_dists) {
				if (pr.second < min_dist) {
					food_moves.clear();
					food_moves.insert(pr.first);
					min_dist = pr.second;
				} else if (pr.second == min_dist) {
					food_moves.insert(pr.first);
				}
			}

			if (food_moves.size() == 1) {
				cout << "Chose single best food move" << endl;
				return *food_moves.begin();
			} else {
				cout << "Chose random best food move" << endl;
				return random_move(food_moves);
			}
			// endregion
		}
	}
}

void process_snake(json const& j, uint8_t owner, GameState& state) {
	Snake s;

	s.idx = owner;

	for (auto jc : j.at("body").at("data")) {
		Coord c(jc.at("y"), jc.at("x"));
		s.coords.push_back(c);
		set_flag(state.board, c, OccupierFlag::Player);
		set_owner(state.board, c, owner);
	}

	// Set counter in reverse so head has max value
	int ctr = 0;
	for (auto it = s.coords.rbegin(); it != s.coords.rend(); ++it) {
		set_counter(state.board, *it, static_cast<Counter>(++ctr));
	}

	s.health = j.at("health");
	s.id = j.at("id");
	state.snakes.push_back(s);
}

GameState process_state(json const& j) {
	GameState state(j.at("height"), j.at("width"));

	for (auto const& f : j.at("food").at("data")) {
		Coord c(f.at("y"), f.at("x"));
		state.food.push_back(c);
		set_flag(state.board, c, OccupierFlag::Food);
	}

	state.snakes.clear();
	state.my_id = j.at("you").at("id");

	uint8_t owner = 0;
	for (auto const& s : j.at("snakes").at("data")) {
		process_snake(s, owner++, state);
	}

	state.me = get_snake(state, state.my_id);

	return state;
}

void server(int port) {
	Server server;

	server.post("/start", [](Request const& req, Response& res) {
		auto start = chrono::high_resolution_clock::now();

		cout << "/start" << endl;

		auto j_in = json::parse(req.body);
//		cout << j_in.dump(2) << endl;

		json j_out;
		j_out["color"] = "#00ff00";
		j_out["secondary_color"] = "#000000";
		j_out["head_url"] = "http://placecage.com/c/100/100";
		j_out["name"] = "clifford";
		j_out["taunt"] = "the big red dog";
		j_out["head_type"] = "pixel";
		j_out["tail_type"] = "pixel";
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
		if (draw_enabled) draw(state);

//		auto ccs = connected_areas(state);
//		draw_labels(ccs.first);

		json j_out;
		j_out["move"] = move_str(get_move(state));
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
    int port;

    Argh argh;
    argh.addFlag(draw_enabled, "--draw", "Enable drawing");
    argh.addOption<int>(port, 5000, "-p", "Port to use");
    argh.parse(argc, argv);

    server(port);
	return 0;
}
