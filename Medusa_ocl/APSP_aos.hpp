
#ifndef _PAGERANK_AOS_HPP_
#define _PAGERANK_AOS_HPP_

#include <CL/cl.h>

//Medusa data structures
#include "vertex_aos.hpp"
#include "edge_aos.hpp"
#include "cmdoptions.hpp"

//construct the data from the file
template <typename T>
EdgeAOS<T> * constructData(
	int vertexCount,
	int &edgeCount,
	VertexAOS<T> *vertexArray

	) {

	ifstream inDataFile("data/small-sample.txt", ios::in);

	if (!inDataFile) {
		cerr << "File could not be opened" << endl;
		exit(1);
	}

	// first line is the vertex rank
	cout << "Initialize vertices \n";
	for (int i = 0; i < vertexCount; i++) {
		inDataFile >> vertexArray[i].vertex_rank;
	}

	//determine the total edge counts
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

	//initialize the vertex and edge array
	EdgeAOS<T> *edgeArray = new EdgeAOS<int>[(edgeCount)];


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
	for (size_t i = 0; i < vertexCount; i++) {
		vertexArray[i].edge_count = 0;
	}



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
				vertexArray[row].edge_count++;
				//record the tail vertex
				edgeArray[numEdgesAdded].tail_vertex = currentCol[row];
				//record the offset
				if (lastEdgePos[row] != LAST_OUT_EDGE) {
					edgeArray[lastEdgePos[row]].offset = numEdgesAdded - lastEdgePos[row];
				}
				lastEdgePos[row] = numEdgesAdded;
				numEdgesAdded++;
			}
			currentCol[row]++;
		}
	}

	for (int i = 0; i < vertexCount; i++){
		edgeArray[lastEdgePos[i]].offset = LAST_OUT_EDGE;
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
		cout << vertexArray[i].vertex_rank << " ";
	}
	cout << endl;
	cout << "Tail vertex:" << endl;
	for (int i = 0; i < edgeCount; i++) {
		cout << edgeArray[i].tail_vertex << " ";
	}
	cout << endl;
	cout << "number of edges on each vertex" << endl;
	for (int i = 0; i < vertexCount; i++) {
		cout << vertexArray[i].edge_count << " ";
	}
	cout << endl;
	cout << "Offset:" << endl;
	for (int i = 0; i < edgeCount; i++) {
		cout << edgeArray[i].offset << " ";
	}
	cout << endl;

	return edgeArray;
}


// OpenCL host side code.
template <typename T>
void medusa(
	CmdParserMedusa& cmdparser,
	OpenCLBasic& oclobjects,
	OpenCLProgramOneKernel& sendMsgKernel,
	OpenCLProgramOneKernel& combineKernel,
	size_t vertex_count,
	size_t edge_count,
	VertexAOS<T> *vertexArray,
	EdgeAOS<T> *edgeArray
	)
{
	// -----------------------------------------------------------------------
	// Calculating, allocating and initializing host-side memory
	// -----------------------------------------------------------------------

	cout
		<< "Running Medusa"
		<< " kernel with vertex count: " << vertex_count << "\n";


	size_t vertex_memory_size = vertex_count * sizeof(VertexAOS<T>);
	size_t edge_memory_size = edge_count * sizeof(EdgeAOS<T>);
	cout << "Size of memory region for vertex: " << vertex_memory_size << " bytes\n";
	cout << "Size of memory region for edge: " << edge_memory_size << " bytes\n";


	// -----------------------------------------------------------------------
	// Allocating device-side resources
	// -----------------------------------------------------------------------
	cout << "Allocating device-side resources \n";

	cl_int err = 0; // OpenCL error code

	// Create OpenCL buffers for the matrices based on allocated memory regions
	// Create buffers with CL_MEM_USE_HOST_PTR to minimize copying and
	// model situation when matrices are hosted by some native library that
	// uses OpenCL to accelerate calculations.

	cl_mem vertex_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		vertex_memory_size,
		vertexArray,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem edge_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		edge_memory_size,
		edgeArray,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);


	// -----------------------------------------------------------------------
	// Setting kernel arguments for sendMsg
	// -----------------------------------------------------------------------

	err = clSetKernelArg(sendMsgKernel.kernel, 0, sizeof(cl_mem), &vertex_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(sendMsgKernel.kernel, 1, sizeof(cl_mem), &edge_buffer);
	SAMPLE_CHECK_ERRORS(err);

	// -----------------------------------------------------------------------
	// Setting kernel arguments for combine
	// -----------------------------------------------------------------------

	err = clSetKernelArg(combineKernel.kernel, 0, sizeof(cl_mem), &vertex_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 1, sizeof(cl_mem), &edge_buffer);
	SAMPLE_CHECK_ERRORS(err);

	// -----------------------------------------------------------------------
	// Define ndrange iteration space: global and local sizes based on
	// parameters obtained from user.

	// Refer to the sample documentation for clarification about
	// how work is devided among work-groups and work-items.
	// -----------------------------------------------------------------------

	size_t global_size[1] = {
		cmdparser.vertex_count.getValue()
	};


	// -----------------------------------------------------------------------
	// Loop with the kernel invocation
	// -----------------------------------------------------------------------

	for (int i = 0; i < cmdparser.iterations.getValue(); i++)
	{
		// Here we start measuring host time for kernel execution
		double start = time_stamp();

		cout << "Invoking kernel \n";
		err = clEnqueueNDRangeKernel(
			oclobjects.queue,
			sendMsgKernel.kernel,
			1,
			0,
			global_size,
			NULL,
			0, 0, 0
			);
		SAMPLE_CHECK_ERRORS(err);


		err = clEnqueueNDRangeKernel(
			oclobjects.queue,
			combineKernel.kernel,
			1,
			0,
			global_size,
			NULL,
			0, 0, 0
			);

		SAMPLE_CHECK_ERRORS(err);

		//read the output ranks on vertices
		clEnqueueMapBuffer(
			oclobjects.queue,
			vertex_buffer,
			CL_TRUE,    // blocking map
			CL_MAP_READ,
			0,
			vertex_memory_size,
			0, 0, 0,
			&err
			);
		SAMPLE_CHECK_ERRORS(err);

		cout << "Vertex rank after Medusa:" << endl;
		for (int i = 0; i < static_cast<int> (vertex_count); i++) {
			cout << vertexArray[i].vertex_rank << " ";
		}
		cout << endl;

		err = clEnqueueUnmapMemObject(
			oclobjects.queue,
			vertex_buffer,
			vertexArray,
			0, 0, 0
			);
		SAMPLE_CHECK_ERRORS(err);

		err = clFinish(oclobjects.queue);
		SAMPLE_CHECK_ERRORS(err);


		// It is important to measure end host time after clFinish call
		double end = time_stamp();

		double time = end - start;
		cout << "Host time: " << time << " sec.\n";

	}

	// deallocated resources
	clReleaseMemObject(vertex_buffer);
	clReleaseMemObject(edge_buffer);

	breakPoint();

}

void invokeMedusa(CmdParserMedusa cmdparser,
	int vertex_count,
	int edgeCount,
	OpenCLBasic& oclobjects,
	OpenCLProgramOneKernel& sendMsgKernel,
	OpenCLProgramOneKernel& combineKernel) {
	// Call medusa with required type of elements
	if (cmdparser.arithmetic_int.isSet())
	{
		VertexAOS<int> *vertexArray = new VertexAOS<int>[(vertex_count)];
		EdgeAOS<int> *edgeArray = constructData(vertex_count, edgeCount, vertexArray);

		medusa<int>(cmdparser, oclobjects, sendMsgKernel, combineKernel, vertex_count, edgeCount, vertexArray, edgeArray);
	}
}

#endif
