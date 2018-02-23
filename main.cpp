#include <algorithm>
#include <iomanip>
#include <iostream>
#include <deque>
#include <queue>
#include <random>
#include <set>

using namespace std;

#include "httplib.h"
using namespace httplib;

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "argh.h"

default_random_engine _rng;

enum class Move {
	Up, Down, Left, Right, None
};
typedef std::set<Move> Moves;

const std::set<Move> AllMoves = {
	Move::Up, Move::Down, Move::Left, Move::Right
};

inline std::string move_str(Move const& dir) {
	switch (dir) {
		case Move::Up:    return "up";
		case Move::Down:  return "down";
		case Move::Left:  return "left";
		case Move::Right: return "right";
		case Move::None:  return "none"; // only used for certain death
	}
}

Move random_move(Moves const& allowable = AllMoves) {
	vector<Move> rmoves;
	for (auto move : allowable) rmoves.push_back(move);
	shuffle(rmoves.begin(), rmoves.end(), _rng);
	return *rmoves.begin();
}

Move last_instructed = Move::None;

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

		last_instructed = Move::None;

		cout << "Responded in " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;
	});

	server.post("/set_move", [](Request const& req, Response& res) {
		auto j_in = json::parse(req.body);

		cout << "Received request to set move to " << j_in["move"] << endl;
		if (j_in["move"] == "up")    last_instructed = Move::Up;
		if (j_in["move"] == "down")  last_instructed = Move::Down;
		if (j_in["move"] == "left")  last_instructed = Move::Left;
		if (j_in["move"] == "right") last_instructed = Move::Right;

	});

	server.post("/move", [](Request const& req, Response& res) {
		auto start = chrono::high_resolution_clock::now();

		cout << "/move" << endl;

		auto j_in = json::parse(req.body);
//		cout << j_in.dump(2) << endl;

//		auto ccs = connected_areas(state);
//		draw_labels(ccs.first);

		json j_out;
		j_out["move"] = move_str(last_instructed != Move::None ? last_instructed : random_move(AllMoves));
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
    argh.addOption<int>(port, 5000, "-p", "Port to use");
    argh.parse(argc, argv);

    server(port);
	return 0;
}
