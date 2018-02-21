#pragma once

#include "types.h"
#include "voronoi.h"

std::string get_color(int idx);
void draw_colored(char c, int color_idx);
void draw_board(GameState const& state);
void draw(GameState const& state);
void draw_labels(LabelledBoard const& labels);
void draw_voronoi(Voronoi const& v);