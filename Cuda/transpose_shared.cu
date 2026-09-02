#include <cstdio>
#include <cuda_runtime.h>



#define TILE 3



__global__ void transpose_shared(
    float* input,
    float* output,
    int N)
{


    __shared__ float tile[TILE][TILE+1];



    int row =
        blockIdx.y*TILE
        +
        threadIdx.y;


    int col =
        blockIdx.x*TILE
        +
        threadIdx.x;



    // 第一步：
    // Global Memory -> Shared Memory

    if(row<N && col<N)
    {
        tile[threadIdx.y][threadIdx.x]
            =
        input[row*N+col];


        printf(
            "LOAD "
            "thread(%d,%d) "
            "A[%d][%d]=%.1f "
            "to tile[%d][%d]\n",

            threadIdx.x,
            threadIdx.y,

            row,
            col,

            input[row*N+col],

            threadIdx.y,
            threadIdx.x
        );
    }



    // 等所有线程搬完

    __syncthreads();



    // 第二步：
    // Shared Memory -> Global Memory


    int new_row = col;
    int new_col = row;


    if(new_row<N && new_col<N)
    {

        output[new_row*N+new_col]
        =
        tile[threadIdx.x][threadIdx.y];



        printf(
            "STORE "
            "thread(%d,%d) "
            "tile[%d][%d] "
            "to B[%d][%d]\n",

            threadIdx.x,
            threadIdx.y,

            threadIdx.x,
            threadIdx.y,

            new_row,
            new_col
        );
    }

}



int main()
{

    const int N=3;


    float h_A[9]=
    {
        1,2,3,
        4,5,6,
        7,8,9
    };


    float h_B[9]={0};



    float *d_A;
    float *d_B;



    cudaMalloc(
        &d_A,
        9*sizeof(float)
    );


    cudaMalloc(
        &d_B,
        9*sizeof(float)
    );



    cudaMemcpy(
        d_A,
        h_A,
        9*sizeof(float),
        cudaMemcpyHostToDevice
    );



    transpose_shared<<<1,dim3(3,3)>>>(
        d_A,
        d_B,
        N
    );



    cudaDeviceSynchronize();



    cudaMemcpy(
        h_B,
        d_B,
        9*sizeof(float),
        cudaMemcpyDeviceToHost
    );

    cudaFree(d_A);
    cudaFree(d_B);


    return 0;
}