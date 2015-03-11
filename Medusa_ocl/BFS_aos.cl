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


/*  The following macros should be defined in the build options:

    - T -- type of matrix elements and all calculations

    Tiles sizes and group sizes:

    - TILE_SIZE_M -- the number of elements of A processed in one WI
    - TILE_GROUP_M -- group/tile size along matrix A (NDRange dimension 0)
    - TILE_SIZE_N -- the number of elements of B processed in one WI
    - TILE_GROUP_N -- group/tile size along matrix B (NDRange dimension 1)
    - TILE_SIZE_K -- size of a tile along dot-product dimension
*/

#ifdef SAMPLE_NEEDS_DOUBLE
    #pragma OPENCL EXTENSION cl_khr_fp64: enable
#endif

#define INIT_LEVEL -1

typedef struct tag_VertexAOS  {
	int edge_count;
	int level;
} VertexAOS;


typedef struct tag_EdgeAOS  {
	int head_vertex;
	int tail_vertex;
	int offset;	// the number of elements down the array with the same head vertex; -1 for end
} EdgeAOS;

/*
 * Traverse the edges to update level
 */
__kernel void traverse (
	global VertexAOS *vertex_list,
	global EdgeAOS *edge_list,
	int super_step,
	global bool *halt
)
{	
	
	int id = get_global_id(0);

	int head_vertex_id = edge_list[id].head_vertex;
	int tail_vertex_id = edge_list[id].tail_vertex;

	if (vertex_list[head_vertex_id].level == super_step)
	{
		if(vertex_list[tail_vertex_id].level == INIT_LEVEL)
		{
			vertex_list[tail_vertex_id].level = super_step + 1;
			*halt = false;
		}
	}

	
}
