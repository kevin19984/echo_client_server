all : echo_client echo_server

echo_client: echo_client.o
	g++ -g -o echo_client echo_client.o -std=c++11 -pthread

echo_server: echo_server.o
	g++ -g -o echo_server echo_server.o -std=c++11 -pthread

echo_client.o: echo_client.cpp
	g++ -g -c -o echo_client.o echo_client.cpp -std=c++11 -pthread

echo_server.o: echo_server.cpp
	g++ -g -c -o echo_server.o echo_server.cpp -std=c++11 -pthread

clean:
	rm -f echo_client
	rm -f echo_server
	rm -f *.o

