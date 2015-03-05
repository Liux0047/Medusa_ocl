
#ifndef _APSP_HPP_
#define _APSP_HPP_

//Medusa data structures
#include "vertex.hpp"
#include "edge.hpp"
#include "cmdoptions.hpp"

//construct the data from the file
template <typename T>
void constructData(
	int vertexCount,
	int edgeCount,
	VertexArray<T> &vertexArray,
	EdgeArray<T> &edgeArray
	) {

	ifstream inDataFile("data/small-sample-APSP.txt", ios::in);

	if (!inDataFile) {
		cerr << "File could not be opened" << endl;
		exit(1);
	}
		
	//dynamic allocate a 2D array
	int** edge = new int*[vertexCount];
	for (int i = 0; i < vertexCount; ++i) {
		edge[i] = new int[vertexCount];
	}

	int edgeWeight;
	//read into the edge [][] array 
	int rowNum = 0;		//row number is the head vertex
	int colNum = 0;		//col number is the tail vertex
	while (inDataFile >> edgeWeight && rowNum < vertexCount) {	//read till EOF and close the file
		edge[rowNum][colNum] = edgeWeight;
		colNum++;
		if (colNum % vertexCount == 0) {	//next line
			colNum = 0;
			rowNum++;
		}
	}

	//initialize edges
	int *tail_vertex = new int[static_cast<int> (edgeCount)];
	int *offset = new int[static_cast<int> (edgeCount)];
	T *weight = new T[static_cast<int> (edgeCount)];
	int *vertex_edge_count = new int[static_cast<int> (vertexCount)];

	for (size_t i = 0; i < edgeCount; i++) {
		tail_vertex[i] = 0;
		offset[i] = 0;
	}

	for (size_t i = 0; i < vertexCount; i++) {
		vertex_edge_count[i] = 0;
	}

	edgeArray.tail_vertex = tail_vertex;
	edgeArray.offset = offset;
	edgeArray.weight = weight;
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
			
			if (currentCol[row] < (static_cast<int> (vertexCount))) {
				halt = false;
				edgeArray.weight[numEdgesAdded] = edge[row][currentCol[row]];
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
	cout << "edge wegihts:" << endl;
	for (int i = 0; i < edgeCount; i++) {
		cout << edgeArray.weight[i] << " ";
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


// OpenCL host side code.
template <typename T>
void medusa(
	CmdParserMedusa& cmdparser,
	OpenCLBasic& oclobjects,
	OpenCLProgramOneKernel& sendMsgKernel,
	OpenCLProgramOneKernel& combineKernel,
	size_t vertex_count,
	size_t edge_count,
	VertexArray<T> &vertexArray,
	EdgeArray<T> &edgeArray
	)
{
	// -----------------------------------------------------------------------
	// Calculating, allocating and initializing host-side memory
	// -----------------------------------------------------------------------

	cout
		<< "Running Medusa"
		<< " kernel with vertex count: " << vertex_count << "\n";
	
	size_t edge_weight_memory_size = edge_count * sizeof(T);
	size_t row_weight_memory_size = vertex_count * sizeof(T);
	size_t col_weight_memory_size = vertex_count * sizeof(T);
	cout << "Size of memory region for edge weights: " << edge_weight_memory_size << " bytes\n";
	cout << "Size of memory region for row/col weights: " << row_weight_memory_size << " bytes\n";

	// Allocate aligned memory for vertex ranks to use them in
	// buffers with CL_MEM_USE_HOST_PTR.
	// a pair of pointer and cl_mem object; cl_mem object is
	// be creater later.



	// -----------------------------------------------------------------------
	// Allocating device-side resources
	// -----------------------------------------------------------------------
	cout << "Allocating device-side resources \n";

	cl_int err = 0; // OpenCL error code

	// Create OpenCL buffers for the matrices based on allocated memory regions
	// Create buffers with CL_MEM_USE_HOST_PTR to minimize copying and
	// model situation when matrices are hosted by some native library that
	// uses OpenCL to accelerate calculations.

	
	cl_mem edge_weight_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		edge_weight_memory_size,
		edgeArray.weight,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem row_weight_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE,
		row_weight_memory_size,
		NULL,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem col_weight_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE,
		col_weight_memory_size,
		NULL,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);


	int vertex_count_int = static_cast<int> (vertex_count);

	// -----------------------------------------------------------------------
	// Setting kernel arguments for sendMsg
	// -----------------------------------------------------------------------

	err = clSetKernelArg(sendMsgKernel.kernel, 0, sizeof(cl_mem), &edge_weight_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(sendMsgKernel.kernel, 1, sizeof(cl_mem), &row_weight_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(sendMsgKernel.kernel, 2, sizeof(cl_mem), &col_weight_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(sendMsgKernel.kernel, 3, sizeof(int), &vertex_count_int);
	SAMPLE_CHECK_ERRORS(err);


	// -----------------------------------------------------------------------
	// Setting kernel arguments for combine
	// -----------------------------------------------------------------------

	err = clSetKernelArg(combineKernel.kernel, 0, sizeof(cl_mem), &edge_weight_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 1, sizeof(cl_mem), &row_weight_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 2, sizeof(cl_mem), &col_weight_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 3, sizeof(int), &vertex_count_int);
	SAMPLE_CHECK_ERRORS(err);

	// -----------------------------------------------------------------------
	// Define ndrange iteration space: global and local sizes based on
	// parameters obtained from user.

	// Refer to the sample documentation for clarification about
	// how work is devided among work-groups and work-items.
	// -----------------------------------------------------------------------
	
	size_t send_msg_global_size[1] = {
		cmdparser.vertex_count.getValue()
	};
	size_t combine_global_size[2] = {
		cmdparser.vertex_count.getValue(),
		cmdparser.vertex_count.getValue()
	};

	// -----------------------------------------------------------------------
	// Loop with the kernel invocation
	// -----------------------------------------------------------------------


	double start = time_stamp();
	for (int k = 0; k < static_cast<int>(vertex_count); k++)
	{
				
		// Here we start measuring host time for kernel execution
		err = clSetKernelArg(sendMsgKernel.kernel, 4, sizeof(int), &k);
		SAMPLE_CHECK_ERRORS(err);

		cout << "Invoking kernel k=" << k << " \n";
		err = clEnqueueNDRangeKernel(
			oclobjects.queue,
			sendMsgKernel.kernel,
			1,
			0,
			send_msg_global_size,
			NULL,
			0, 0, 0
			);
		
		err = clEnqueueNDRangeKernel(
			oclobjects.queue,
			combineKernel.kernel,
			2,
			0,
			combine_global_size,
			NULL,
			0, 0, 0
			);
			

		err = clFinish(oclobjects.queue);
		SAMPLE_CHECK_ERRORS(err);
		
	}

	
	// It is important to measure end host time after clFinish call
	double end = time_stamp();

	double time = end - start;
	cout << "Host time: " << time << " sec.\n";

	//read the output ranks on vertices
	clEnqueueMapBuffer(
		oclobjects.queue,
		edge_weight_buffer,
		CL_TRUE,    // blocking map
		CL_MAP_READ,
		0,
		edge_weight_memory_size,
		0, 0, 0,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);
	

	cout << "edge weight after Medusa:" << endl;
	for (int i = 0; i < static_cast<int> (edge_count); i++) {
		cout << edgeArray.weight[i] << " ";
	}

	err = clEnqueueUnmapMemObject(
		oclobjects.queue,
		edge_weight_buffer,
		edgeArray.weight,
		0, 0, 0
		);
	SAMPLE_CHECK_ERRORS(err);
	

	// deallocated resources
	clReleaseMemObject(edge_weight_buffer);
	clReleaseMemObject(row_weight_buffer);
	clReleaseMemObject(col_weight_buffer);

	
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
		VertexArray<int> vertexArray;
		EdgeArray<int> edgeArray;
		edgeCount = vertex_count * vertex_count;
		constructData(vertex_count, edgeCount, vertexArray, edgeArray);
		medusa<int>(cmdparser, oclobjects, sendMsgKernel, combineKernel, vertex_count, edgeCount, vertexArray, edgeArray);
	}
}

#endif
