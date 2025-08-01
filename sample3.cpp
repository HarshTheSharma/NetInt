// A C++ program for Dijkstra's single source shortest path
// algorithm. The program is for adjacency matrix
// representation of the graph

// original code snippet from https://www.geeksforgeeks.org/cpp/c-program-for-dijkstras-shortest-path-algorithm-greedy-algo-7/
#include "NetInt.h"
#include <iostream>

int main() {
	// Initialize the NetInt context
	hideMessages(true);
	setWhitelist({"127.0.0.1"});
	establishPort("8080");
	NetInt temp = -1;
	std::cout << "Temporary NetInt value: " << temp << std::endl;
	NetInt temp2 = temp + -5;
	std::cout << "Temporary NetInt value after addition: " << temp2 << std::endl;

	return 0;
}