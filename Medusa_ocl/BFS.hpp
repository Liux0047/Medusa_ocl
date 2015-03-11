
#ifndef _BFS_HPP_
#define _BFS_HPP_

//Medusa data structures
#include "vertex_BFS.hpp"
#include "edge_BFS.hpp"
#include "cmdoptions.hpp"
#include "common.hpp"

#define INIT_LEVEL -1

using namespace std;

wstring clFileName = L"BFS.cl";

//construct the data from the file
void constructData(
	int vertexCount,
	int edgeCount,
	VertexArray &vertexArray,
	EdgeArray &edgeArray
	) {

	srand(time(NULL));

	cout << "Constructing data" << endl;

	// Initialize vertices
	int *vertex_level = new int [static_cast<int> (vertexCount)];
	vertexArray.level = vertex_level;

	int *edge_count = new int[static_cast<int> (vertexCount)];
	vertexArray.edge_count = edge_count;

	for (size_t i = 0; i < vertexCount; ++i)
	{
		vertexArray.edge_count[i] = 0;
		vertexArray.level[i] = INIT_LEVEL;
	}
	vertexArray.level[0] = 0;


	//initialize edges
	int *head_vertex = new int[static_cast<int> (edgeCount)];
	int *tail_vertex = new int[static_cast<int> (edgeCount)];
	int *offset = new int[static_cast<int> (edgeCount)];

	for (size_t i = 0; i < edgeCount; i++) {
		head_vertex[i] = 0;
		tail_vertex[i] = 0;
		offset[i] = 0;
	}


	edgeArray.head_vertex = head_vertex;
	edgeArray.tail_vertex = tail_vertex;
	edgeArray.offset = offset;

	int *last_edge_pos = new int[static_cast<int> (vertexCount)];	// the position of last out edge for each vertex
	for (size_t i = 0; i < vertexCount; i++) {
		last_edge_pos[i] = -1;
	}

	int *quota = generateQuotaWithZero<int>(vertexCount, edgeCount);
	unsigned long cellCount = 0;	//visualize in a matrix, number of cells that has been traversed
	
	int head = 0;
	for (size_t i = 0; i < edgeCount; ++i)
	{
		// Fill the edge with random values from range [0, vertex_count]
		head = cellCount % vertexCount;
		if (vertexArray.edge_count[head] < quota[head]){
			int tail = 0;

			do {
				// avoids self pointing edges
				tail = rand() % vertexCount;
			} while (tail == head);

			edgeArray.head_vertex[i] = head;
			edgeArray.tail_vertex[i] = tail;
			vertexArray.edge_count[head]++;

			//record the offset
			if (last_edge_pos[head] != -1) {
				edgeArray.offset[last_edge_pos[head]] = i - last_edge_pos[head];
			}
			last_edge_pos[head] = i;
		}
		else {
			i--;
		}
		cellCount++;

	}

	for (size_t i = 0; i < vertexCount; i++){
		offset[last_edge_pos[i]] = LAST_OUT_EDGE;
	}

	//cleaning up
	delete[] last_edge_pos;


	if (vertexCount <= 6){
		//output for test
		cout << "Vertex level:" << endl;
		for (int i = 0; i < vertexCount; i++) {
			cout << vertexArray.level[i] << " ";
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

}


// OpenCL host side code.
void medusa(
	CmdParserMedusa& cmdparser,
	OpenCLBasic& oclobjects,
	OpenCLProgramOneKernel& traverseKernel,
	size_t vertex_count,
	size_t edge_count,
	VertexArray &vertexArray,
	EdgeArray &edgeArray
	)
{
	// -----------------------------------------------------------------------
	// Calculating, allocating and initializing host-side memory
	// -----------------------------------------------------------------------

	cout
		<< "Running Medusa"
		<< " kernel with vertex count: " << vertex_count << "\n";


	size_t vertex_level_memory_size = vertex_count * sizeof(int);
	size_t vertex_edge_count_memory_size = vertex_count * sizeof(int);
	size_t edge_offset_memory_size = edge_count * sizeof(int);
	size_t tail_vertex_memory_size = edge_count * sizeof(int);
	size_t head_vertex_memory_size = edge_count * sizeof(int);
	cout << "Size of memory region for vertex rank: " << vertex_level_memory_size << " bytes\n";
	cout << "Size of memory region for edge tails/heads: " << tail_vertex_memory_size << " bytes\n";
	cout << "Size of memory region for edge offset: " << edge_offset_memory_size << " bytes\n";

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

	cl_mem vertex_level_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		vertex_level_memory_size,
		vertexArray.level,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	
	cl_mem edge_head_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		head_vertex_memory_size,
		edgeArray.head_vertex,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem edge_tail_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		tail_vertex_memory_size,
		edgeArray.tail_vertex,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);
	
	bool halt = false;
	cl_mem halt_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		sizeof(bool),
		&halt,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	clEnqueueMapBuffer(
		oclobjects.queue,
		halt_buffer,
		CL_TRUE,    // blocking map
		CL_MAP_READ,
		0,
		sizeof(bool),
		0, 0, 0,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	
	


	// -----------------------------------------------------------------------
	// Setting kernel arguments for traverseKernel
	// -----------------------------------------------------------------------
	
	err = clSetKernelArg(traverseKernel.kernel, 0, sizeof(cl_mem), &edge_head_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(traverseKernel.kernel, 1, sizeof(cl_mem), &edge_tail_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(traverseKernel.kernel, 2, sizeof(cl_mem), &vertex_level_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(traverseKernel.kernel, 4, sizeof(cl_mem), &halt_buffer);
	SAMPLE_CHECK_ERRORS(err);


	// -----------------------------------------------------------------------
	// Define ndrange iteration space: global and local sizes based on
	// parameters obtained from user.

	// Refer to the sample documentation for clarification about
	// how work is devided among work-groups and work-items.
	// -----------------------------------------------------------------------

	size_t global_size[1] = {
		cmdparser.edge_count.getValue()
	};


	// -----------------------------------------------------------------------
	// Loop with the kernel invocation
	// -----------------------------------------------------------------------
	cout << "Invoking kernel \n";
	double start = time_stamp();
	
	for (int i = 0; i < cmdparser.iterations.getValue(); i++)
	{
		int superStep = 0;

		while (!halt){
			halt = true;

			err = clSetKernelArg(traverseKernel.kernel, 3, sizeof(int), &superStep);
			SAMPLE_CHECK_ERRORS(err);
			
			// Here we start measuring host time for kernel execution
			err = clEnqueueNDRangeKernel(
				oclobjects.queue,
				traverseKernel.kernel,
				1,
				0,
				global_size,
				NULL,
				0, 0, 0
				);
			SAMPLE_CHECK_ERRORS(err);

			err = clFinish(oclobjects.queue);
			SAMPLE_CHECK_ERRORS(err);
			
			superStep++;

		} 
		

	}
	// It is important to measure end host time after clFinish call
	double end = time_stamp();

	double time = end - start;
	cout << "Host time: " << time << " sec.\n";

	err = clEnqueueUnmapMemObject(
		oclobjects.queue,
		halt_buffer,
		&halt,
		0, 0, 0
		);
	SAMPLE_CHECK_ERRORS(err);

	//read the output ranks on vertices
	clEnqueueReadBuffer(
		oclobjects.queue,
		vertex_level_buffer,
		CL_TRUE,
		0,
		vertex_level_memory_size,
		vertexArray.level,
		0, NULL, NULL
		);

	SAMPLE_CHECK_ERRORS(err);


	// deallocated resources
	clReleaseMemObject(vertex_level_buffer);
	clReleaseMemObject(edge_head_buffer);
	clReleaseMemObject(edge_tail_buffer);

	if (vertex_count <= 6) {
		cout << "Vertex level after Medusa:" << endl;
		for (int i = 0; i < static_cast<int> (vertex_count); i++) {
			cout << vertexArray.level[i] << " ";
		}
		cout << endl;
	}
	
	breakPoint();

}

void invokeMedusa(CmdParserMedusa cmdparser,
	int vertex_count,
	int edgeCount,
	OpenCLBasic& oclobjects) {


	// Form build options string from given parameters: macros definitions to pass into kernels
	string build_options =
		"-DT=" + cmdparser.arithmetic.getValue();
	/*  +
	(cmdparser.arithmetic_double.isSet() ? " -DSAMPLE_NEEDS_DOUBLE" : "") +
	" -DTILE_SIZE_M=" + to_str(cmdparser.tile_size_M.getValue()) +
	" -DTILE_GROUP_M=" + to_str(cmdparser.tile_group_M.getValue()) +
	" -DTILE_SIZE_N=" + to_str(cmdparser.tile_size_N.getValue()) +
	" -DTILE_GROUP_N=" + to_str(cmdparser.tile_group_N.getValue()) +
	" -DTILE_SIZE_K=" + to_str(cmdparser.tile_size_K.getValue());
	*/

	cout << "Build program options: " << inquotes(build_options) << "\n";

	// Build kernel
	cout << "build traverse kernel \n";
	OpenCLProgramOneKernel traverse(
		oclobjects,
		clFileName,
		"",
		"traverse",
		build_options
		);


	// Call medusa with required type of elements
	if (cmdparser.arithmetic_int.isSet())
	{
		VertexArray vertexArray;
		EdgeArray edgeArray;
		constructData(vertex_count, edgeCount, vertexArray, edgeArray);
		medusa(cmdparser, oclobjects, traverse, vertex_count, edgeCount, vertexArray, edgeArray);
	}
}

#endif
