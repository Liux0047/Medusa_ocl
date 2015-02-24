#ifndef EDGE_AA_H_
#define EDGE_AA_H_

#define LAST_OUT_EDGE -1
// Edge Array
// using column-major adjacency array
template <typename T>
struct EdgeArrayAA {
	int *tail_vertex;
	
	T *message;
};

#endif  // EDGE_AA_H_