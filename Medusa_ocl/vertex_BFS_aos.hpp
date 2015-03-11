#ifndef VERTEX_BFS_AOS_H_
#define VERTEX_BFS_AOS_H_

#include <CL/cl.h>

struct VertexAOS {
	cl_int edge_count;
	cl_int level;
};

#endif  // VERTEX_H_