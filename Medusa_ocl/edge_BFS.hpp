#ifndef EDGE_BFS_H_
#define EDGE_BFS_H_

#define LAST_OUT_EDGE -1
// Edge Array
// using column-major adjacency array
struct EdgeArray {
	int *head_vertex;
	int *tail_vertex;
	int *offset;	// the number of elements down the array with the same head vertex; -1 for end
};

#endif  // EDGE_H_