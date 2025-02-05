all: svr2 cli2

svr2: tcp_svr2.cc
	g++ -o $@ $^ -std=c++14 -pthread
cli2: tcp_cli2.cc
	g++ -o $@ $^ -std=c++14 -pthread

.PHONY:
clean:
	rm svr2 cli2