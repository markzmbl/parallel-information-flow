all:
	# g++ -O2 -fopenmp -lpthread -std=c++2a -o main main.cpp
	g++ -O0 -g -pthread -std=c++2a -o main main.cpp