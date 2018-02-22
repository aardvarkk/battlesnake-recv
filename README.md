g++ -std=c++11 main.cpp && ./a.out

SERVER

brew install elm
mix deps.get
mix ecto.create
mix ecto.migrate
cd assets && npm install
cd ..
PORT=4000 mix phx.server

FAILED HTTP LIBRARIES
- proxygen (./deps.sh doesn't run on macOS)
- cpprestsdk (build fails to link to boost)
- pistache (relies on epoll -- can't find sys/timerfd.h)
