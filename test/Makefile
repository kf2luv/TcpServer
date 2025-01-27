all: svr cli

svr: tcp_svr.cc
	g++ -o $@ $^ -std=c++11 -pthread
cli: tcp_cli.cc
	g++ -o $@ $^ -std=c++11 -pthread

.PHONY:
clean:
	rm svr cli