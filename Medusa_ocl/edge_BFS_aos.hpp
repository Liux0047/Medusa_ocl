#ifndef EDGE_BFS_AOS_H_
#define EDGE_BFS_AOS_H_

#define LAST_OUT_EDGE -1

#include <CL/cl.h>
// Edge Array
// using column-major adjacency array
struct EdgeAOS {
	cl_int head_vertex;

	cl_int tail_vertex;

	cl_int offset;	// the number of elements down the array with the same head vertex; -1 for end
};

#endif  // EDGE_AA_H_