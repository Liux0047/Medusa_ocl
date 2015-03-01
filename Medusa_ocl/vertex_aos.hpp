#ifndef VERTEX_AOS_H_
#define VERTEX_AOS_H_

#include <CL/cl.h>

template <typename T>
struct VertexAOS {
	cl_int edge_count;
	cl_int vertex_rank;
};

#endif  // VERTEX_H_