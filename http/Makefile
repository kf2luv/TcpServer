all: server client

server: main.cc
	g++ -o $@ $^ -std=c++17 -pthread -lstdc++fs
client: client.cc
	g++ -o $@ $^ -std=c++17 -pthread -lstdc++fs

.PHONY:
clean:
	rm server client