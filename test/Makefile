all: svr3 cli3

svr3: tcp_svr3.cc
	g++ -o $@ $^ -std=c++14 -pthread
cli3: tcp_cli3.cc
	g++ -o $@ $^ -std=c++14 -pthread

.PHONY:
clean:
	rm svr3 cli3