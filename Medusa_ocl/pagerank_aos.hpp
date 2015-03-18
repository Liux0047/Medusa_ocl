
#ifndef _PAGERANK_AOS_HPP_
#define _PAGERANK_AOS_HPP_

#include <CL/cl.h>

//Medusa data structures
#include "vertex_aos.hpp"
#include "edge_aos.hpp"
#include "cmdoptions.hpp"
#include "common.hpp"

using namespace std;
wstring clFileName = L"pagerank_aos.cl";

//construct the data from the file
template <typename T>
void constructData(
	int vertexCount,
	int edgeCount,
	VertexAOS<T> *vertexArray,
	EdgeAOS<T> *edgeArray	
	) {

	cout << "constructing data" << endl;
	T rank;
	srand(time(NULL));
	for (size_t i = 0; i < vertexCount; ++i)
	{
		// Fill the vertex with random values from range [1, 100]
		rank = (rand() % 100) + 1;
		vertexArray[i].vertex_rank = rank;
	}

	for (size_t i = 0; i < vertexCount; i++) {
		vertexArray[i].edge_count = 0;
	}

	for (size_t i = 0; i < edgeCount; i++) {
		edgeArray[i].offset = 0;
	}

	int *last_edge_pos = new int[static_cast<int> (vertexCount)];	// the position of last out edge for each vertex
	for (size_t i = 0; i < vertexCount; i++) {
		last_edge_pos[i] = -1;
	}

	int *quota = generateQuota<T>(vertexCount, edgeCount);

	int maxQuota = 0, maxQuotaId = 0;
	unsigned long cellCount = 0;	//visualize in a matrix, number of cells that has been traversed
	
	int head = 0;
	int numEdgeGenerated = 0;
	while (numEdgeGenerated < edgeCount)
	{
		// Fill the edge with random values from range [0, vertex_count]
		head = cellCount % vertexCount;
		if (vertexArray[head].edge_count < quota[head]){

			if (quota[head] > maxQuota) {
				maxQuota = quota[head];
				maxQuotaId = head;
			}

			int tail = 0;

			do {
				// avoids self pointing edges
				tail = rand() % vertexCount;
			} while (tail == head);

			edgeArray[numEdgeGenerated].tail_vertex = tail;
			vertexArray[head].edge_count++;

			//record the offset
			if (last_edge_pos[head] != -1) {
				edgeArray[last_edge_pos[head]].offset = numEdgeGenerated - last_edge_pos[head];
			}
			last_edge_pos[head] = numEdgeGenerated;
			numEdgeGenerated++;
		}
		cellCount++;

	}

	cout << "Max quota is " << maxQuota << " at position " << maxQuotaId << endl;

	for (size_t i = 0; i < vertexCount; i++){
		edgeArray[last_edge_pos[i]].offset = LAST_OUT_EDGE;
	}

	//cleaning up
	delete[] last_edge_pos;


	if (vertexCount <= 4){
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
	}
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
		<< "Running Medusa PageRank AOS"
		<< " kernel with edge count: " << edge_count << "\n";


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
	// Here we start measuring host time for kernel execution
	double start = time_stamp();
	cout << "Invoking kernel \n";
	for (int i = 0; i < cmdparser.iterations.getValue(); i++)
	{
		
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

		err = clFinish(oclobjects.queue);
		SAMPLE_CHECK_ERRORS(err);
		
	}
	// It is important to measure end host time after clFinish call
	double end = time_stamp();
	double time = end - start;
	cout << "Host time: " << time << " sec.\n";

	if (vertex_count <= 4){
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
	}

	// deallocated resources
	clReleaseMemObject(vertex_buffer);
	clReleaseMemObject(edge_buffer);	

	breakPoint();

}

void invokeMedusa(CmdParserMedusa cmdparser,
	int vertex_count,
	int edgeCount,
	OpenCLBasic& oclobjects) {
	// Call medusa with required type of elements
	if (cmdparser.arithmetic_int.isSet())
	{
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
		cout << "build send message kernel \n";
		OpenCLProgramOneKernel sendMsgKernel(
			oclobjects,
			clFileName,
			"",
			"send_msg",
			build_options
			);

		cout << "build combine message kernel \n";
		OpenCLProgramOneKernel combineKernel(
			oclobjects,
			clFileName,
			"",
			"combine",
			build_options
			);

		VertexAOS<int> *vertexArray = new VertexAOS<int>[(vertex_count)];
		EdgeAOS<int> *edgeArray = new EdgeAOS<int>[(edgeCount)];

		constructData(vertex_count, edgeCount, vertexArray, edgeArray);		
		medusa<int>(cmdparser, oclobjects, sendMsgKernel, combineKernel, vertex_count, edgeCount, vertexArray, edgeArray);
	}
}

#endif
