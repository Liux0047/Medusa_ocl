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
		4
    ),
	edge_count(
		*this,
		'e',
		"edge_count",
		"<integer>",
		"Number of edges.",
		4
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
        1
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
