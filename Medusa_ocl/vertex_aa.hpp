#ifndef VERTEX_AA_H_
#define VERTEX_AA_H_


template <typename T>
struct VertexArrayAA {
	int *edge_count;
	unsigned long *start;		//the starting position of the vertex's edges in the edgeArray
	T *vertex_rank;
};

#endif  // VERTEX_H_