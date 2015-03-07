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


#include <limits>
#include <cmath>

#include "cmdoptions.hpp"

using namespace std;


#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4355)    // 'this': used in base member initializer list
#endif

CmdParserMedusa::CmdParserMedusa (int argc, const char** argv) :
    CmdParserCommon(argc, argv),
    vertex_count(
        *this,
        'v',
        "vertex_count",
        "<integer>",
        "Number of vertices.",
		100000
    ),
	edge_count(
		*this,
		'e',
		"edge_count",
		"<integer>",
		"Number of edges.",
		1600000
	),
    iterations(
        *this,
        'i',
        "iterations",
        "<integer>",
        "Number of kernel invocations. For each invoction, "
            "performance information will be printed. "
            "Zero is allowed: in this case no kernel invocation "
            " is performed but all other host stuff is created.",
        10
    ),
    arithmetic(
        *this,
        'a',
        "arithmetic",
        "",
        "Type of elements and all calculations.",
        "int"
    ),
	arithmetic_int(arithmetic, "int")
    //arithmetic_float(arithmetic, "float"),
    //arithmetic_double(arithmetic, "double")
{
}

#ifdef _MSC_VER
#pragma warning (pop)
#endif


void CmdParserMedusa::parse ()
{
    CmdParserCommon::parse();

    // Test a small part of parameters for consistency
    // in this function. The major part of checks is placed in
    // validateParameters function. But to call it you need
    // further specialization on what OpenCL objects and their
    // capabilities are.

	if (!arithmetic_int.isSet()){
		throw CmdParser::Error(
			"int datatype not chosen. "
			);
	}
    
}

/*
size_t CmdParserMedusa::estimateMaxMatrixSize (
    OpenCLBasic& oclobjects,
    size_t size_of_element,
    size_t alignment
)
{
    cl_ulong max_alloc_size = 0;
    cl_int err = clGetDeviceInfo(
        oclobjects.device,
        CL_DEVICE_MAX_MEM_ALLOC_SIZE,
        sizeof(max_alloc_size),
        &max_alloc_size,
        0
    );
    SAMPLE_CHECK_ERRORS(err);

    cl_ulong max_global_mem_size = 0;
    err = clGetDeviceInfo(
        oclobjects.device,
        CL_DEVICE_GLOBAL_MEM_SIZE,
        sizeof(max_global_mem_size),
        &max_global_mem_size,
        0
    );
    SAMPLE_CHECK_ERRORS(err);

    double max_matrix_size = sqrt(
        min(
            double(numeric_limits<size_t>::max()),
            min(double(max_alloc_size), double(max_global_mem_size)/3)
        ) / size_of_element
    );

    assert(alignment%size_of_element == 0);

    // the following is effect of a bit conservative
    // estimation of the overhead on a row alignment
    max_matrix_size -= alignment/size_of_element;

    assert(max_matrix_size < double(numeric_limits<size_t>::max()));

    return static_cast<size_t>(max_matrix_size);
}


void CmdParserMedusa::validateTile (
    const CmdOption<size_t>& tile_group,
    const CmdOption<size_t>& tile_size,
    size_t max_group_value
)
{
    validatePositiveness(tile_group);
    validatePositiveness(tile_size);

    tile_group.validate(
        size.getValue() % tile_group.getValue() == 0,
        "should divide matrix size without a remainder"
    );

    tile_size.validate(
        size.getValue() % tile_size.getValue() == 0,
        "should divide matrix size without a remainder"
    );

    tile_group.validate(
        tile_group.getValue() <= max_group_value,
        "too big value; should be <= " + to_str(max_group_value)
    );

    if(
        size.getValue() %
        (tile_group.getValue() * tile_size.getValue()) != 0
    )
    {
        throw CmdParser::Error(
            "Multiplication of " + tile_group.name() + " and " + tile_size.name() +
            " parameters should divide matrix size without a remainder."
        );
    }
}


void CmdParserMedusa::validateParameters (
    OpenCLBasic& oclobjects,
    OpenCLProgramOneKernel& executable,
    size_t size_of_element,
    size_t alignment
)
{
    validatePositiveness(size);

    size_t max_matrix_size =
        estimateMaxMatrixSize(oclobjects, size_of_element, alignment);

    size.validate(
        size.getValue() <= max_matrix_size,
        "requested value is too big; should be <= " + to_str(max_matrix_size)
    );

    iterations.validate(
        iterations.getValue() >= 0,
        "negative value is provided; should be positive or zero"
    );

    size_t max_work_item_sizes[3] = {0};
    deviceMaxWorkItemSizes(oclobjects.device, max_work_item_sizes);

    validateTile(tile_group_M, tile_size_M, max_work_item_sizes[0]);
    validateTile(tile_group_N, tile_size_N, max_work_item_sizes[1]);

    size_t work_group_size =
        tile_group_M.getValue() * tile_group_N.getValue();

    size_t max_device_work_group_size =
        deviceMaxWorkGroupSize(oclobjects.device);

    size_t max_kernel_work_group_size =
        kernelMaxWorkGroupSize(executable.kernel, oclobjects.device);

    size_t max_work_group_size =
        min(max_device_work_group_size, max_kernel_work_group_size);

    if(work_group_size > max_kernel_work_group_size)
    {
        throw CmdParser::Error(
            "Work group size required based on " +
            tile_group_M.name() + " and " + tile_group_N.name() +
            " is greater than allowed for this kernel and/or device. " +
            "Maximum possible value is " +
            to_str(max_kernel_work_group_size) + "."
        );
    }

    validatePositiveness(tile_size_K);

    tile_size_K.validate(
        size.getValue() % tile_size_K.getValue() == 0,
        "should divide matrix size without a remainder"
    );
}
*/
