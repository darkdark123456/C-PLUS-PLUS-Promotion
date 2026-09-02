#include <cstdio>
#include <cuda_runtime.h>


__global__ void transpose_naive(
    float* input,
    float* output,
    int N)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;


    if(row < N && col < N)
    {
        // A[row][col]
        float value = input[row * N + col];


        // B[col][row]
        output[col * N + row] = value;


        printf(
            "thread(%d,%d) "
            "read A[%d][%d]=%.1f "
            "write B[%d][%d]\n",

            threadIdx.x,
            threadIdx.y,

            row,
            col,
            value,

            col,
            row
        );
    }
}


int main()
{
    const int N = 3;


    float h_A[N*N] =
    {
        1,2,3,
        4,5,6,
        7,8,9
    };


    float h_B[N*N]={0};



    float *d_A;
    float *d_B;


    cudaMalloc(
        &d_A,
        N*N*sizeof(float)
    );

    cudaMalloc(
        &d_B,
        N*N*sizeof(float)
    );


    cudaMemcpy(
        d_A,
        h_A,
        N*N*sizeof(float),
        cudaMemcpyHostToDevice
    );


    dim3 block(3,3);

    dim3 grid(1,1);


    transpose_naive<<<grid,block>>>(
        d_A,
        d_B,
        N
    );


    cudaDeviceSynchronize();



    cudaMemcpy(
        h_B,
        d_B,
        N*N*sizeof(float),
        cudaMemcpyDeviceToHost
    );

    cudaFree(d_A);
    cudaFree(d_B);

    return 0;
}