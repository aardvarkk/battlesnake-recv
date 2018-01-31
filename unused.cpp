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
