#ifndef EDGE_AOS_H_
#define EDGE_AOS_H_

#define LAST_OUT_EDGE -1

#include <CL/cl.h>
// Edge Array
// using column-major adjacency array
template <typename T>
struct EdgeAOS {
	cl_int tail_vertex;

	cl_int offset;	// the number of elements down the array with the same head vertex; -1 for end

	cl_int message;
};

#endif  // EDGE_AA_H_