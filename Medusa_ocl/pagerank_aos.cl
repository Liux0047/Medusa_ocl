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


typedef struct tag_VertexAOS  {
	int edge_count;
	int vertex_rank;
} VertexAOS;


typedef struct tag_EdgeAOS  {
	int tail_vertex;
	int offset;	// the number of elements down the array with the same head vertex; -1 for end
	int message;
} EdgeAOS;


// sendMsg function
// edge_count_list is the list of edge counts in VertexArray
// rank_list is the list of vertex ranks in VertexArray
// edge_msg_list is the pointer to a list of edge messages
// edge_offset_list is the pointer to a list of edge offsets as in CAA layout
__kernel void send_msg (
	global VertexAOS *vertexArray,
	global EdgeAOS *edgeArray
)
{

	int id = get_global_id(0);
		
	T msg = (vertexArray[id].vertex_rank + vertexArray[id].edge_count/2) / vertexArray[id].edge_count;	//round to nearest integer

	//broadcast to all out edges
	//edge list stored in CAA format	
	int edge_id = id;
	
	while (edgeArray[edge_id].offset != -1) { 
		edgeArray[edge_id].message = msg;
		edge_id += edgeArray[edge_id].offset;
	} 	
	edgeArray[edge_id].message = msg;
	
	
	
}


/*
 * combine messages from edges
 */
__kernel void combine (
	global VertexAOS *vertexArray,
	global EdgeAOS *edgeArray
)
{	
	int id = get_global_id(0);
	
	int edge_id = id;
	int offset = edgeArray[id].offset;
	vertexArray[id].vertex_rank = 0;

	 while (edgeArray[edge_id].offset != -1) { 

		// atomic add floats
		//while ( atom_cmpxchg( mutex[ tail_vertex[edge_id] ], 0 ,1 ) );
		atomic_add(&vertexArray[ edgeArray[edge_id].tail_vertex ].vertex_rank, edgeArray[edge_id].message);
		//atom_xchg ( mutex[ tail_vertex[edge_id] ], 0 );
		
		edge_id += edgeArray[edge_id].offset;
	};
	atomic_add(&vertexArray[ edgeArray[edge_id].tail_vertex ].vertex_rank, edgeArray[edge_id].message);
	
	
}
