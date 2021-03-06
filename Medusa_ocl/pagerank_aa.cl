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


// sendMsg function
// edge_count_list is the list of edge counts in VertexArray
// rank_list is the list of vertex ranks in VertexArray
// edge_msg_list is the pointer to a list of edge messages
// edge_offset_list is the pointer to a list of edge offsets as in CAA layout
__kernel void send_msg (
	global T *rank_list,
	global int *edge_count,
	global int *edge_start_pos,
	global T *edge_msg_list
)
{
	int id = get_global_id(0);
		
	T msg = (rank_list[id] + edge_count[id]/2) / edge_count[id];	//round to nearest integer

	//broadcast to all out edges
	//edge list stored in AA format		
	for (int edge_local_index = 0; edge_local_index < edge_count[id]; edge_local_index++){
		edge_msg_list[edge_start_pos[id] + edge_local_index] = msg;
	}
}


/*
 * combine messages from edges
 */
__kernel void combine (
	global int *tail_vertex,
	global T *edge_msg_list,
	global int *edge_count,
	global int *edge_start_pos,
	global T *rank_list_output
)
{	

	int id = get_global_id(0);
	
	rank_list_output[id] = 0;

	for (int edge_local_index = 0; edge_local_index < edge_count[id]; edge_local_index++){
		int edge_id = edge_start_pos[id] + edge_local_index;
		atomic_add(&rank_list_output[ tail_vertex[edge_id] ], edge_msg_list[edge_id]);
	}
	
}
