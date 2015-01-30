// Copyright (c) 2009-2013 Intel Corporation
// All rights reserved.
//
// WARRANTY DISCLAIMER
//
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly


#include <iostream>
#include <ctime>
#include <limits>
#include <cmath>
#include <time.h>
#include <stdlib.h>

#include <CL/cl.h>

#include "basic.hpp"
#include "cmdoptions.hpp"
#include "oclobject.hpp"

//Medusa data structures
#include "vertex.hpp"
#include "edge.hpp"

using namespace std;

void breakPoint () {
	int continue_key;
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	cout << "Press any key to continue...";
	cin >> continue_key;
}

//initialize test data
template <typename T>
void initData(
	size_t vertex_count,
	size_t edge_count,
	VertexArray<T> &vertexAarry,
	EdgeArray<T> &edgeArray
) {
	// Initialize vertices
	cout << "Initialize vertices \n";
	T *vertex_rank = new T[static_cast<int> (vertex_count)];
	int *vertex_edge_count = new int[static_cast<int> (vertex_count)];
		
	vertexAarry.vertex_rank = vertex_rank;
	vertexAarry.edge_count = vertex_edge_count;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		// Fill the vertex with random values from range [0, 1]
		srand(time(NULL));
		T rank = (rand() % 100) / 100.0;
		vertex_rank[i] = rank;
	}


	//initialize edges
	int *head_vertex = new int[static_cast<int> (edge_count)];
	int *tail_vertex = new int[static_cast<int> (edge_count)];
	int *offset = new int[static_cast<int> (edge_count)];
	T *message = new T[static_cast<int> (edge_count)];

	
	edgeArray.tail_vertex = tail_vertex;
	edgeArray.offset = offset;
	edgeArray.message = message;

	int *last_edge_pos = new int[static_cast<int> (vertex_count)];	// the position of last out edge for each vertex
	//initialize last edge position array
	for (size_t i = 0; i < vertex_count; i++) {
		last_edge_pos[i] = 0;
	}

	// initialize edge data
	for (size_t i = 0; i < edge_count; ++i)
	{
		// Fill the edge with random values from range [0, vertex_count]
		srand(time(NULL));
		int head = rand() % vertex_count;
		int tail;
		// avoids self pointing edges
		do {
			tail = rand() % vertex_count;
		} while (tail == head);

		//update edge_count
		vertexAarry.edge_count[head]++;

		//record the offset
		if (last_edge_pos[head] != 0) {
			offset[last_edge_pos[head]] = i - last_edge_pos[head];
		}
		last_edge_pos[head] = i;

	}

	for (size_t i = 0; i < vertex_count; i++){
		offset[last_edge_pos[i]] = LAST_OUT_EDGE;
	}

	delete[] last_edge_pos;

}

// The main medusa function with all application specific
// OpenCL host side code.
template <typename T>
void medusa(
    CmdParserMedusa& cmdparser,
    OpenCLBasic& oclobjects,
    OpenCLProgramOneKernel& sendMsgKernel,
	OpenCLProgramOneKernel& combineKernel,
	size_t vertex_count,
	size_t edge_count,
	VertexArray<T> &vertexAarry,
	EdgeArray<T> &edgeArray
)
{
    // -----------------------------------------------------------------------
    // Calculating, allocating and initializing host-side memory
    // -----------------------------------------------------------------------
	
    cout
        << "Running Medusa" 
		<< " kernel with vertex count: " << vertex_count << "\n";


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
		vertexAarry.vertex_rank,
        &err
    );
    SAMPLE_CHECK_ERRORS(err);

	cl_mem vertex_edge_count_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		vertex_edge_count_memory_size,
		vertexAarry.edge_count,
		&err
		);
	SAMPLE_CHECK_ERRORS(err);

	cl_mem edge_msg_buffer = clCreateBuffer(
		oclobjects.context,
		CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		edge_msg_memory_size,
		edgeArray.message,
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
	err = clSetKernelArg(sendMsgKernel.kernel, 2, sizeof(cl_mem), &edge_msg_buffer);
    SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(sendMsgKernel.kernel, 3, sizeof(cl_mem), &edge_offset_buffer);
    SAMPLE_CHECK_ERRORS(err);


	// -----------------------------------------------------------------------
	// Setting kernel arguments for combine
	// -----------------------------------------------------------------------

	err = clSetKernelArg(combineKernel.kernel, 0, sizeof(cl_mem), &vertex_rank_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 1, sizeof(cl_mem), &tail_vertex_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 2, sizeof(cl_mem), &edge_msg_buffer);
	SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(combineKernel.kernel, 3, sizeof(cl_mem), &edge_offset_buffer);
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

    for(int i = 0; i < cmdparser.iterations.getValue(); i++)
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


        // It is important to measure end host time after clFinish call
        double end = time_stamp();

        double time = end - start;
        cout << "Host time: " << time << " sec.\n";

    }

    // deallocated resources
	clReleaseMemObject(vertex_rank_buffer);
	clReleaseMemObject(vertex_edge_count_buffer);	
	clReleaseMemObject(edge_msg_buffer);
	clReleaseMemObject(edge_offset_buffer);
	
}


// Entry point for sample application, command-line parsing,
// generic OpenCL resources allocation and deallocation.
int main (int argc, const char** argv)
{
    try
    {
        // Define and parse command-line arguments.
        CmdParserMedusa cmdparser(argc, argv);
        cmdparser.parse();

        // Immediatly exit if user wanted to see the usage information only.
        if(cmdparser.help.isSet())
        {
            return 0;
        }

        // Create the necessary OpenCL objects up to device queue.
        OpenCLBasic oclobjects(
            cmdparser.platform.getValue(),
            cmdparser.device_type.getValue(),
            cmdparser.device.getValue()
        );


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
            L"medusa.cl",
            "",
            "send_msg",
            build_options
        );

		cout << "build combine message kernel \n";
		OpenCLProgramOneKernel combineKernel(
			oclobjects,
			L"medusa.cl",
			"",
			"combine",
			build_options
			);


		size_t vertex_count = cmdparser.vertex_count.getValue();
		size_t edge_count = cmdparser.edge_count.getValue();

	
        // Call medusa with required type of elements
        if(cmdparser.arithmetic_int.isSet())
        {
			VertexArray<int> vertexAarry;
			EdgeArray<int> edgeArray;
			initData(vertex_count, edge_count, vertexAarry, edgeArray);
			medusa<int>(cmdparser, oclobjects, sendMsgKernel, combineKernel, vertex_count, edge_count, vertexAarry, edgeArray);
        }

		breakPoint();

        // All resource deallocations happen in destructors of helper objects.

        return 0;
    }
    catch(const CmdParser::Error& error)
    {
        cerr
            << "[ ERROR ] In command line: " << error.what() << "\n"
            << "Run " << argv[0] << " -h for usage info.\n";
		breakPoint();
        return 1;
    }
    catch(const Error& error)
    {
        cerr << "[ ERROR ] Sample application specific error: " << error.what() << "\n";
		breakPoint();
        return 1;
    }
    catch(const exception& error)
    {
        cerr << "[ ERROR ] " << error.what() << "\n";
		breakPoint();
        return 1;
    }
    catch(...)
    {
        cerr << "[ ERROR ] Unknown/internal error happened.\n";
		breakPoint();
        return 1;
    }
}
