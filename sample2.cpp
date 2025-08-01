// A C++ program for Dijkstra's single source shortest path
// algorithm. The program is for adjacency matrix
// representation of the graph

// original code snippet from https://www.geeksforgeeks.org/cpp/c-program-for-dijkstras-shortest-path-algorithm-greedy-algo-7/
#include "NetInt.h"
#include <limits.h>
#include <stdio.h>

// Number of vertices in the graph
#define V 9

// A utility function to find the vertex with minimum
// distance value, from the set of vertices not yet included
// in shortest path tree
NetInt minDistance(NetInt dist[], bool sptSet[]) {
	// Initialize min value
	NetInt min = INT_MAX, min_index;

	for (NetInt v = 0; v < V; v++)
		if (sptSet[v] == false && dist[v] <= min)
			min = dist[v], min_index = v;

	return min_index;
}

// A utility function to prNetInt the constructed distance
// array
void prNetIntSolution(NetInt dist[], NetInt n) {
	std::cout << "Vertex   Distance from Source\n";
	for (NetInt i = 0; i < V; i++)
		std::cout << "\t" << i << "\t\t\t\t" << dist[i] << std::endl;
}

// Function that implements Dijkstra's single source
// shortest path algorithm for a graph represented using
// adjacency matrix representation
void dijkstra(NetInt graph[V][V], NetInt src) {
	NetInt dist[V]; // The output array.  dist[i] will hold the
					// shortest
	// distance from src to i

	bool sptSet[V]; // sptSet[i] will be true if vertex i is
					// included in shortest
	// path tree or shortest distance from src to i is
	// finalized

	// Initialize all distances as INFINITE and stpSet[] as
	// false
	for (NetInt i = 0; i < V; i++)
		dist[i] = INT_MAX, sptSet[i] = false;

	// Distance of source vertex from itself is always 0
	dist[src] = 0;

	// Find shortest path for all vertices
	for (NetInt count = 0; count < V - 1; count++) {
		// Pick the minimum distance vertex from the set of
		// vertices not yet processed. u is always equal to
		// src in the first iteration.
		NetInt u = minDistance(dist, sptSet);

		// Mark the picked vertex as processed
		sptSet[u] = true;

		// Update dist value of the adjacent vertices of the
		// picked vertex.
		for (NetInt v = 0; v < V; v++)

			// Update dist[v] only if is not in sptSet,
			// there is an edge from u to v, and total
			// weight of path from src to  v through u is
			// smaller than current value of dist[v]
			if (!sptSet[v] && graph[u][v] && dist[u] != INT_MAX && dist[u] + graph[u][v] < dist[v])
				dist[v] = dist[u] + graph[u][v];
	}

	// prNetInt the constructed distance array
	prNetIntSolution(dist, V);
}

// driver program to test above function
int main() {
	hideMessages(true);
	setWhitelist({"127.0.0.1"});
	establishPort("8080");
	/* Let us create the example graph discussed above */
	NetInt graph[V][V] = {{0, 4, 0, 0, 0, 0, 0, 8, 0},
						  {4, 0, 8, 0, 0, 0, 0, 11, 0},
						  {0, 8, 0, 7, 0, 4, 0, 0, 2},
						  {0, 0, 7, 0, 9, 14, 0, 0, 0},
						  {0, 0, 0, 9, 0, 10, 0, 0, 0},
						  {0, 0, 4, 14, 10, 0, 2, 0, 0},
						  {0, 0, 0, 0, 0, 2, 0, 1, 6},
						  {8, 11, 0, 0, 0, 0, 1, 0, 7},
						  {0, 0, 2, 0, 0, 0, 6, 7, 0}};

	dijkstra(graph, 0);

	return 0;
}