void add_snake(GameState& state, Coord const& c, int length, int food) {
//  int idx = state.heads.size();
//  state.heads.push_back(c);
//  state.food_left.push_back(food);
//  set_occupier(state.board, state.heads[idx], Snake);
//  set_counter(state.board, state.heads[idx], counter);
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

void get_articulation_points(
	Coords const& coords, // array-ordered
	int i, // index into helper arrays
	int d, // depth
	vector<bool>& visited,
	vector<int>& parent,
	vector<int>& depth,
	vector<int>& low,
	Coords& articulation_pts
) {
	cout << "Pointer to i = " << &i << endl;
	cout << "Visiting index " << i << " in coords size " << coords.size() << endl;
	cout << "Sizes: " << visited.size() << " " << parent.size() << " " << depth.size() << " " << low.size() << endl;

	visited[i] = true;
	depth[i] = d;
	low[i] = d;

	int child_count = 0;
	bool is_articulation = false;

	vector<int> adj;
	for (auto ni = 0; ni < coords.size(); ++ni) {
		if (are_neighbours(coords[i], coords[ni])) {
			cout << "Push " << ni << " neighbour to " << i << " from coord size " << coords.size() << " onto size " << adj.size() << endl;
			adj.push_back(ni);
		}
	}

	for (auto const& ni : adj) {
		cout << "a" << endl;
		if (!visited[ni]) {
			cout << "b" << endl;
			parent[ni] = i;
			cout << "b1" << endl;
			get_articulation_points(coords, ni, d+1, visited, parent, depth, low, articulation_pts);
			cout << "b2" << endl;
			child_count++;
			if (low[ni] >= depth[i]) {
				cout << "c" << endl;
				is_articulation = true;
			}
			cout << "d" << endl;
			low[i] = min(low[i], low[ni]);
		}
		else if (ni != parent[i]) {
			cout << "e" << endl;
			low[i] = min(low[i], depth[ni]);
		}
	}
	cout << "f" << endl;
	if ((parent[i] != -1 && is_articulation) || (parent[i] == -1 && child_count > 1))
		articulation_pts.push_back(coords[i]);
}

Coords articulation_points(Coords const& in) {
	int err = 0;
	pthread_attr_t 	stackSizeAttribute;
	size_t			stackSize = 0;
	err = pthread_attr_init (&stackSizeAttribute);
	err = pthread_attr_getstacksize(&stackSizeAttribute, &stackSize);
	err = pthread_attr_setstacksize (&stackSizeAttribute, 16 * 1024 * 1024);
	err = pthread_attr_getstacksize(&stackSizeAttribute, &stackSize);
	cout << "Stack size: " << stackSize << endl;

	Coords out;
	out.reserve(in.size());
	vector<bool> visited(in.size());
	vector<int> parent(in.size(), -1);
	vector<int> depth(in.size());
	vector<int> low(in.size());
	get_articulation_points(in, 0, 0, visited, parent, depth, low, out);
	return out;
}