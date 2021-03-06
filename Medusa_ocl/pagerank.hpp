
#ifndef _PAGERANK_HPP_
#define _PAGERANK_HPP_

//Medusa data structures
#include "vertex.hpp"
#include "edge.hpp"
#include "cmdoptions.hpp"
#include "common.hpp"

using namespace std;
wstring clFileName = L"pagerank.cl";

//construct the data from the file
template <typename T>
void constructData(
	int vertexCount,
	int edgeCount,
	VertexArray<T> &vertexArray,
	EdgeArray<T> &edgeArray
	) {

	cout << "Constructing data" <<endl;

	// Initialize vertices
	T *vertex_rank = new T[static_cast<int> (vertexCount)];
	vertexArray.vertex_rank = vertex_rank;

	T rank;
	srand(time(NULL));
	for (size_t i = 0; i < vertexCount; ++i)
	{
		// Fill the vertex with random values from range [1, 100]
		rank = (rand() % 100) + 1;
		vertexArray.vertex_rank[i] = rank;
	}


	//initialize edges
	int *tail_vertex = new int[static_cast<int> (edgeCount)];
	int *offset = new int[static_cast<int> (edgeCount)];
	T *message = new T[static_cast<int> (edgeCount)];
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
	edgeArray.message = message;
	vertexArray.edge_count = vertex_edge_count;

	int *last_edge_pos = new int[static_cast<int> (vertexCount)];	// the position of last out edge for each vertex
	for (size_t i = 0; i < vertexCount; i++) {
		last_edge_pos[i] = -1;
	}

	int *quota = generateQuota<T>(vertexCount, edgeCount);
	unsigned long cellCount = 0;	//visualize in a matrix, number of cells that has been traversed
	int head = 0;
	int numEdgeGenerated = 0;
	int maxQuota = 0, maxQuotaId = 0;
	while (numEdgeGenerated < edgeCount)
	{
		// Fill the edge with random values from range [0, vertex_count]
		head = cellCount % vertexCount;
		if (vertexArray.edge_count[head] < quota[head]){
			
			if (quota[head] > maxQuota) {
				maxQuota = quota[head];
				maxQuotaId = head;
			}
			
			int tail = 0;

			do {
				// avoids self pointing edges
				tail = rand() % vertexCount;
			} while (tail == head);

			edgeArray.tail_vertex[numEdgeGenerated] = tail;
			vertexArray.edge_count[head]++;

			//record the offset
			if (last_edge_pos[head] != -1) {
				edgeArray.offset[last_edge_pos[head]] = numEdgeGenerated - last_edge_pos[head];
			}
			last_edge_pos[head] = numEdgeGenerated;

			numEdgeGenerated++;
		}

		cellCount++;
	}
	
	for (size_t i = 0; i < vertexCount; i++){
		offset[last_edge_pos[i]] = LAST_OUT_EDGE;
	}
		
	//cleaning up
	delete[] last_edge_pos;

	cout << "Max quota is " << maxQuota << " at position " << maxQuotaId << endl;
	
	if (vertexCount <= 4){
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
		<< "Running Medusa PageRank"
		<< " kernel with edge count: " << edge_count << "\n";


	size_t vertex_rank_memory_size = vertex_count * sizeof(T);
	size_t vertex_edge_count_memory_size = vertex_count * sizeof(int);
	size_t edge_msg_memory_size = edge_count * sizeof(T);
	size_t edge_offset_memory_size = edge_count * sizeof(int);
	size_t tail_vertex_memory_size = edge_count * sizeof(int);
	cout << "Size of memory region for vertex rank: " << vertex_rank_memory_size << " bytes\n";
	cout << "Size of memory region for edge message: " << edge_msg_memory_size << " bytes\n";
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

	cl_mem vertex_rank_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		vertex_rank_memory_size,
		vertexArray.vertex_rank,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem vertex_edge_count_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		vertex_edge_count_memory_size,
		vertexArray.edge_count,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem edge_msg_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_WRITE_ONLY,
		edge_msg_memory_size,
		NULL,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem edge_offset_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		edge_offset_memory_size,
		edgeArray.offset,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem tail_vertex_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		tail_vertex_memory_size,
		edgeArray.tail_vertex,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);




	// -----------------------------------------------------------------------
	// Setting kernel arguments for sendMsg
	// -----------------------------------------------------------------------

	err = clSetKernelArg(sendMsgKernel.kernel, 0, sizeof(cl_mem), &vertex_rank_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(sendMsgKernel.kernel, 1, sizeof(cl_mem), &vertex_edge_count_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(sendMsgKernel.kernel, 2, sizeof(cl_mem), &edge_offset_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(sendMsgKernel.kernel, 3, sizeof(cl_mem), &edge_msg_buffer);
	SAMPLE_CHECK_ERRORS(err);


	// -----------------------------------------------------------------------
	// Setting kernel arguments for combine
	// -----------------------------------------------------------------------

	err = clSetKernelArg(combineKernel.kernel, 0, sizeof(cl_mem), &tail_vertex_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 1, sizeof(cl_mem), &edge_msg_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 2, sizeof(cl_mem), &edge_offset_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 3, sizeof(cl_mem), &vertex_rank_buffer);
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
	cout << "Invoking kernel \n";

	double start = time_stamp();
	for (int i = 0; i < cmdparser.iterations.getValue(); i++)
	{
		// Here we start measuring host time for kernel execution
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
		err = clFinish(oclobjects.queue);
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

	//read the output ranks on vertices
	clEnqueueReadBuffer(
		oclobjects.queue,
		vertex_rank_buffer,
		CL_TRUE,
		0,
		vertex_rank_memory_size,
		vertexArray.vertex_rank,
		0, NULL, NULL
		);

	SAMPLE_CHECK_ERRORS(err);

	// deallocated resources
	clReleaseMemObject(vertex_rank_buffer);
	clReleaseMemObject(vertex_edge_count_buffer);
	clReleaseMemObject(edge_msg_buffer);
	clReleaseMemObject(edge_offset_buffer);

	if (vertex_count <= 4) {
		cout << "Vertex rank after Medusa:" << endl;
		for (int i = 0; i < static_cast<int> (vertex_count); i++) {
			cout << vertexArray.vertex_rank[i] << " ";
		}
		cout << endl;
	}
	

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

		VertexArray<int> vertexArray;
		EdgeArray<int> edgeArray;
		constructData(vertex_count, edgeCount, vertexArray, edgeArray);
		medusa<int>(cmdparser, oclobjects, sendMsgKernel, combineKernel, vertex_count, edgeCount, vertexArray, edgeArray);
	}
}

#endif
