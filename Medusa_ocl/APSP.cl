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
// edge_offset_list is the pointer to a list of edge offsets as in CAA layout
__kernel void send_msg (
	global T *edge_weight_list,
	global T *row_weights,
	global T *col_weights,
	int k

)
{
	int id = get_global_id(0);
		
	row_weights[id] = edge_weight_list[k * 4 + id];
	col_weights[id] = edge_weight_list[id *4 + k];
	
}


/*
 * combine messages from edges
 */
__kernel void combine (
	global T *row_weights,
	global T *col_weights,
	global T *edge_weight_list
)
{	

	int row = get_global_id(0);
    int col = get_global_id(1);

	int row_weight = row_weights[col];
	int col_weight = col_weights[row];

    if(row_weight != -1 && col_weight != -1) { 
        int sum =  (row_weight + col_weight);
        if (edge_weight_list[row * 4 + col] == -1 || sum < edge_weight_list[row * 4 + col])
            edge_weight_list[row * 4 + col] = sum;
    }
    
		
}
