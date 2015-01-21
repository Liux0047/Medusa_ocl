#ifndef VERTEX_H_
#define VERTEX_H_


template <typename T>
struct VertexArray {
	int *edge_count;
	T *vertex_rank;
};

#endif  // VERTEX_H_