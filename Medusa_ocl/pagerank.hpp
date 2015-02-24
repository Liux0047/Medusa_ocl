
#ifndef _PAGERANK_HPP_
#define _PAGERANK_HPP_

//Medusa data structures
#include "vertex.hpp"
#include "edge.hpp"

//construct the data from the file
template <typename T>
void constructData(
	int vertexCount,
	int &edgeCount,
	VertexArray<T> &vertexArray,
	EdgeArray<T> &edgeArray
	) {

	ifstream inDataFile("data/small-sample.txt", ios::in);

	if (!inDataFile) {
		cerr << "File could not be opened" << endl;
		exit(1);
	}

	// first line is the vertex rank
	cout << "Initialize vertices \n";
	T *vertex_rank = new T[static_cast<int> (vertexCount)];
	for (int i = 0; i < vertexCount; i++) {
		inDataFile >> vertex_rank[i];
	}
	vertexArray.vertex_rank = vertex_rank;

	//determine the edge counts
	long edgeStartLocation = inDataFile.tellg();

	int hasEdge;
	while (!inDataFile.eof()) {
		inDataFile >> hasEdge;
		// 1 means edge exits; 0 otherwise
		if (hasEdge == 1) {
			edgeCount += hasEdge;
		}
	}

	//rewind to edge starting position
	inDataFile.clear();
	inDataFile.seekg(edgeStartLocation);

	//dynamic allocate a 2D array
	int** edge = new int*[vertexCount];
	for (int i = 0; i < vertexCount; ++i) {
		edge[i] = new int[vertexCount];
	}


	//read into the edge [][] array 
	int rowNum = 0;		//row number is the head vertex
	int colNum = 0;		//col number is the tail vertex
	while (inDataFile >> hasEdge && rowNum < vertexCount) {	//read till EOF and close the file
		edge[rowNum][colNum] = hasEdge;
		colNum++;
		if (colNum % vertexCount == 0) {	//next line
			colNum = 0;
			rowNum++;
		}
	}

	//initialize edges
	int *head_vertex = new int[static_cast<int> (edgeCount)];
	int *tail_vertex = new int[static_cast<int> (edgeCount)];
	int *offset = new int[static_cast<int> (edgeCount)];
	T *message = new T[static_cast<int> (edgeCount)];
	int *vertex_edge_count = new int[static_cast<int> (vertexCount)];

	for (size_t i = 0; i < edgeCount; i++) {
		head_vertex[i] = 0;
		tail_vertex[i] = 0;
		offset[i] = 0;
	}

	for (size_t i = 0; i < vertexCount; i++) {
		vertex_edge_count[i] = 0;
	}

	edgeArray.tail_vertex = tail_vertex;
	edgeArray.offset = offset;
	edgeArray.message = message;
	vertexArray.edge_count = vertex_edge_count;


	int *lastEdgePos = new int[static_cast<int> (vertexCount)];	// the position of last out edge for each vertex
	int *currentCol = new int[static_cast<int> (vertexCount)];	// the position of current col number for each vertex in the input matrix
	//initialize last edge position array
	for (size_t i = 0; i < vertexCount; i++) {
		lastEdgePos[i] = LAST_OUT_EDGE;
		currentCol[i] = 0;
	}

	// construct CAA in column major foramt
	int numEdgesAdded = 0;
	bool halt = false;
	while (!halt) {
		halt = true;
		for (int row = 0; row < vertexCount; row++){
			while (currentCol[row] < (static_cast<int> (vertexCount)) && edge[row][currentCol[row]] != 1) {
				currentCol[row]++;
			}

			if (currentCol[row] < (static_cast<int> (vertexCount))) {
				halt = false;
				vertexArray.edge_count[row]++;
				//record the tail vertex
				(edgeArray.tail_vertex)[numEdgesAdded] = currentCol[row];
				//record the offset
				if (lastEdgePos[row] != LAST_OUT_EDGE) {
					edgeArray.offset[lastEdgePos[row]] = numEdgesAdded - lastEdgePos[row];
				}
				lastEdgePos[row] = numEdgesAdded;
				numEdgesAdded++;
			}
			currentCol[row]++;
		}
	}

	for (int i = 0; i < vertexCount; i++){
		edgeArray.offset[lastEdgePos[i]] = LAST_OUT_EDGE;
	}

	//cleaning up
	delete[] lastEdgePos;
	for (int i = 0; i < vertexCount; ++i) {
		delete[] edge[i];
	}
	delete[] edge;

	//output for test
	cout << "Vertex rank:" << endl;
	for (int i = 0; i < vertexCount; i++) {
		cout << vertexArray.vertex_rank[i] << " ";
	}
	cout << endl;
	cout << "Tail vertex:" << endl;
	for (int i = 0; i < edgeCount; i++) {
		cout << edgeArray.tail_vertex[i] << " ";
	}
	cout << endl;
	cout << "number of edges on each vertex" << endl;
	for (int i = 0; i < vertexCount; i++) {
		cout << vertexArray.edge_count[i] << " ";
	}
	cout << endl;
	cout << "Offset:" << endl;
	for (int i = 0; i < edgeCount; i++) {
		cout << edgeArray.offset[i] << " ";
	}
	cout << endl;

}


#endif
