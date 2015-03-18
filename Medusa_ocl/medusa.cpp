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
#include "pagerank.hpp"


using namespace std;



int *makePageRankVertices(int vertexCount) {
	int *ranks = new int[vertexCount];
	for (int i = 0; i < vertexCount; i++) {
		ranks[i] = rand() % 100 + 1;
	}

	return ranks;
}

int **makePageRankEdges(int vertexCount, int &edgeCount) {
	int** edges = new int*[vertexCount];
	for (int i = 0; i < vertexCount; ++i) {
		edges[i] = new int[vertexCount];
	}

	int value;

	for (int i = 0; i < vertexCount; i++) {
		for (int j = 0; j < vertexCount; j++) {
			if (i != j) {
				value = (rand() % vertexCount) / (vertexCount -16);
				if (value == 1){
					edgeCount++;
				}
			}
			else {
				value = 0;
			}
			edges[i][j] = value;
		}
	}

	return edges;
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


		size_t vertexCount = cmdparser.vertex_count.getValue();
		size_t edgeCount = cmdparser.edge_count.getValue();

		invokeMedusa(cmdparser, vertexCount, edgeCount, oclobjects);
		
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
