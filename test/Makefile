all: svr2 cli

svr2: tcp_svr2.cc
	g++ -o $@ $^ -std=c++11 -pthread
cli: tcp_cli.cc
	g++ -o $@ $^ -std=c++11 -pthread

.PHONY:
clean:
	rm svr cli