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


// Check validity for multiplication of square matrices: Cresult == alpha*A*Btransposed(B) + beta*Cinit.
// Samll simplification here: the procedure assumes that initial C values are all zeros,
// so beta is not actually used.
template <class T>
bool checkValidity (
    const T* A,     // left input matrix, column-major
    const T* B,     // right input matrix, column-major or row-major depending on Btransposed argument
    const T* C,     // output matrix, column-major
    size_t size,    // number of column in each of the matrices
    size_t ldabc,   // row stride: the number of T elements in one row (including optional padding)
    bool Btransposed,
    T alpha, T beta // coefficient to compute
)
{
    cout << "Validate output..." << flush;

    // Btransposed == false, lstride = 1
    size_t lstride = Btransposed ? ldabc : 1;
    size_t jstride = Btransposed ? 1 : ldabc;

    // Estimate error tolerance for a given type T and relying on the fact
    // that initial matrix values are from [0, 1]
    T max_value = 1;
    T error_tol = T(2) * alpha * max_value * max_value * T(2) * size * numeric_limits<T>::epsilon();

    for(size_t i = 0; i < size; ++i)
    {
        for(size_t j = 0; j < size; ++j)
        {
            // compute golden value for c[i][j] element
            T accum = 0;
            for(size_t l = 0; l < size; ++l)
            {
                accum += A[l*ldabc + i] * B[l*lstride + j*jstride];
            }

            T golden = alpha*accum;

            T absdiff = abs(C[j*ldabc+i] - golden);
            if(absdiff > error_tol)
            {
                cout << " FAILED\n";
                cerr.precision(std::numeric_limits<T>::digits10);
                cerr << "\nVALIDATION FAILED!!!\n    reference" << "[" << i << ", " << j << "] = "
                     << golden << ",\n    calculated" << "[" << i << ", " << j << "] = "
                     << C[j*ldabc+i]
                     << ",\n    absolute difference" << "[" << i << ", " << j << "] = " << absdiff << "\n"
                     << "Further validation was stopped\n\n";
                return false;
            }
        }
    }

    std::cout << " PASSED\n";
    return true;
}


// The main medusa function with all application specific
// OpenCL host side code.
template <typename T>
void medusa(
    CmdParserMedusa& cmdparser,
    OpenCLBasic& oclobjects,
    OpenCLProgramOneKernel& executable
)
{
    // -----------------------------------------------------------------------
    // Calculating, allocating and initializing host-side memory
    // -----------------------------------------------------------------------
	
	size_t vertex_count = cmdparser.vertex_count.getValue();
	size_t edge_count = cmdparser.edge_count.getValue();

    cout
        << "Running Medusa" 
		<< " kernel with vertex count: " << vertex_count << "\n";


	size_t vertex_rank_memory_size = vertex_count * sizeof(T);
	size_t edge_msg_memory_size = edge_count * sizeof(T);
	size_t edge_offset_memory_size = edge_count * sizeof(int);
	cout << "Size of memory region for vertex rank: " << vertex_rank_memory_size << " bytes\n";
	cout << "Size of memory region for edge message: " << edge_msg_memory_size << " bytes\n";
	cout << "Size of memory region for edge offset: " << edge_offset_memory_size << " bytes\n";

    // Allocate aligned memory for vertex ranks to use them in
    // buffers with CL_MEM_USE_HOST_PTR.
    // a pair of pointer and cl_mem object; cl_mem object is
    // be creater later.
	

	// Initialize vertices
	T *vertex_rank = new T [ static_cast<int> (vertex_count)];

	VertexArray<T> vertexAarry;
	vertexAarry.vertex_rank = vertex_rank;
	
	for (size_t i = 0; i < vertex_count; ++i)
    {
        // Fill the vertex with random values from range [0, 1]
		srand(time(NULL));
		T rank = (rand() % 100) / 100.0;
		vertex_rank[i] = rank;
		vertexAarry.vertex_index[i] = i;
    }


	//initialize edges
	int *head_vertex = new int [ static_cast<int> (edge_count) ];
	int *tail_vertex = new int [ static_cast<int> (edge_count) ];
	int *offset = new int [ static_cast<int> (edge_count)];
	T *message = new T [ static_cast<int> (edge_count) ];

	EdgeArray<T> edgeArray;
	edgeArray.tail_vertex = tail_vertex;
	edgeArray.offset = offset;
	edgeArray.message = message;

	int *last_edge_pos = new int [static_cast<int> (vertex_count) ];	// the position of last out edge for each vertex
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


    // -----------------------------------------------------------------------
    // Allocating device-side resources for matrices
    // -----------------------------------------------------------------------

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


    // -----------------------------------------------------------------------
    // Setting kernel arguments
    // -----------------------------------------------------------------------

	err = clSetKernelArg(executable.kernel, 0, sizeof(cl_mem), &vertex_rank_buffer);
    SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(executable.kernel, 1, sizeof(cl_int), &edge_msg_buffer);
    SAMPLE_CHECK_ERRORS(err);
	err = clSetKernelArg(executable.kernel, 2, sizeof(cl_mem), &edge_offset_buffer);
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

	/*
    size_t local_size[2] = {
        cmdparser.tile_group_M.getValue(),
        cmdparser.tile_group_N.getValue()
    };
	*/


    // -----------------------------------------------------------------------
    // Loop with the kernel invocation
    // -----------------------------------------------------------------------

    for(int i = 0; i < cmdparser.iterations.getValue(); i++)
    {
        // Here we start measuring host time for kernel execution
        double start = time_stamp();

        err = clEnqueueNDRangeKernel(
            oclobjects.queue,
            executable.kernel,
            1,
            0,
            global_size,
            NULL,
            0, 0, 0
        );

		/* original 
		err = clEnqueueNDRangeKernel(
			oclobjects.queue,
			executable.kernel,
			2,
			0,
			global_size,
			local_size,
			0, 0, 0
			);
		*/
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
	clReleaseMemObject(edge_msg_buffer);
	clReleaseMemObject(edge_offset_buffer);

	delete[] vertex_rank;
	delete[] head_vertex;
	delete[] tail_vertex;
	delete[] offset;
	delete[] message;

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
			"-DT=" + cmdparser.arithmetic.getValue() +
			(cmdparser.arithmetic_double.isSet() ? " -DSAMPLE_NEEDS_DOUBLE" : "");
			/* +
            " -DTILE_SIZE_M=" + to_str(cmdparser.tile_size_M.getValue()) +
            " -DTILE_GROUP_M=" + to_str(cmdparser.tile_group_M.getValue()) +
            " -DTILE_SIZE_N=" + to_str(cmdparser.tile_size_N.getValue()) +
            " -DTILE_GROUP_N=" + to_str(cmdparser.tile_group_N.getValue()) +
            " -DTILE_SIZE_K=" + to_str(cmdparser.tile_size_K.getValue());
			*/

        cout << "Build program options: " << inquotes(build_options) << "\n";

        // Build kernel
        OpenCLProgramOneKernel executable(
            oclobjects,
            L"medusa.cl",
            "",
            "vertex_func",
            build_options
        );

        // Call medusa with required type of elements
        if(cmdparser.arithmetic_float.isSet())
        {
			medusa<float>(cmdparser, oclobjects, executable);
        }
        else if(cmdparser.arithmetic_double.isSet())
        {
			medusa<double>(cmdparser, oclobjects, executable);
        }

		int exit;
		cout << "Press any key to exit";
		cin >> exit;

        // All resource deallocations happen in destructors of helper objects.

        return 0;
    }
    catch(const CmdParser::Error& error)
    {
        cerr
            << "[ ERROR ] In command line: " << error.what() << "\n"
            << "Run " << argv[0] << " -h for usage info.\n";
        return 1;
    }
    catch(const Error& error)
    {
        cerr << "[ ERROR ] Sample application specific error: " << error.what() << "\n";
        return 1;
    }
    catch(const exception& error)
    {
        cerr << "[ ERROR ] " << error.what() << "\n";
        return 1;
    }
    catch(...)
    {
        cerr << "[ ERROR ] Unknown/internal error happened.\n";
        return 1;
    }
}
