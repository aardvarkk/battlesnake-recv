g++ -std=c++11 main.cpp && ./a.out

FAILED HTTP LIBRARIES
- proxygen (./deps.sh doesn't run on macOS)
- cpprestsdk (build fails to link to boost)
- pistache (relies on epoll -- can't find sys/timerfd.h)
