#ifndef EDGE_H_
#define EDGE_H_

#define LAST_OUT_EDGE -1
// Edge Array
// using column-major adjacency array
template <typename T>
struct EdgeArray {
	int *tail_vertex;

	int *offset;	// the number of elements down the array with the same head vertex; -1 for end

	T *message;

	T *weight;
};

#endif  // EDGE_H_