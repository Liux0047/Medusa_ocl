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
#include <fstream>
#include <ctime>
#include <vector>
#include <limits>
#include <cmath>
#include <time.h>
#include <stdlib.h>


#include <CL/cl.h>

#include "basic.hpp"
#include "cmdoptions.hpp"
#include "oclobject.hpp"
#include "APSP.hpp"

#define AOS true

using namespace std;


//count the number of edges
int edgeCount = 0;
wstring clFileName = L"APSP.cl";

void breakPoint () {
	int continue_key;
	cout << "Press any key to continue Medusa...";
	cin >> continue_key;
}

// The main medusa function with all application specific


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


		size_t vertex_count = cmdparser.vertex_count.getValue();

		invokeMedusa(cmdparser, vertex_count, edgeCount, oclobjects, sendMsgKernel, combineKernel);
		
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
