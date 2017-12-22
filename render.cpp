#include <chrono>
#include <cstdio>
#include <thread>

using namespace std;

int grid = 8;

void draw(char c) {
  static bool first = true;
  if (!first) printf("\033[%dA", grid);
  if (first)  first = !first;

	for (int i = 0; i < grid; ++i) {
		for (int j = 0; j < grid; ++j) {
      putchar(c);
    }
    printf("\n");
	}
}

int main(int argc, char* argv[]) {
  draw('*');
  this_thread::sleep_for(chrono::seconds(1));
  draw(176u);
  this_thread::sleep_for(chrono::seconds(1));
  draw(177u);
  this_thread::sleep_for(chrono::seconds(1));
  draw(178u);
}
