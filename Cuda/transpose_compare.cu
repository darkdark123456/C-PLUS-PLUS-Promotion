#include <cstdio>
#include <cuda_runtime.h>


#define N 4096
#define TILE 32



// ============================
// Naive transpose
// ============================

__global__ void transpose_naive(
    float* in,
    float* out,
    int n)
{

    int row =
        blockIdx.y * blockDim.y
        +
        threadIdx.y;


    int col =
        blockIdx.x * blockDim.x
        +
        threadIdx.x;



    if(row<n && col<n)
    {
        out[col*n + row]
        =
        in[row*n + col];
    }
}




// ============================
// Shared transpose
// ============================

__global__ void transpose_shared(
    float* in,
    float* out,
    int n)
{

    __shared__
    float tile[TILE][TILE+1];



    int row =
        blockIdx.y*TILE
        +
        threadIdx.y;


    int col =
        blockIdx.x*TILE
        +
        threadIdx.x;



    // Global -> Shared

    if(row<n && col<n)
    {
        tile[threadIdx.y][threadIdx.x]
        =
        in[row*n+col];
    }


    __syncthreads();



    // Shared -> Global


    int new_row =
        blockIdx.x*TILE
        +
        threadIdx.y;


    int new_col =
        blockIdx.y*TILE
        +
        threadIdx.x;



    if(new_row<n && new_col<n)
    {

        out[new_row*n+new_col]
        =
        tile[threadIdx.x][threadIdx.y];

    }

}



int main()
{

    size_t bytes =
        N*N*sizeof(float);



    printf(
        "Matrix: %d x %d\n",
        N,N
    );

    printf(
        "Memory: %.2f MB\n",
        bytes/1024.0/1024.0
    );



    float *h_A =
        (float*)malloc(bytes);


    float *d_A;
    float *d_B;



    cudaMalloc(
        &d_A,
        bytes
    );

    cudaMalloc(
        &d_B,
        bytes
    );



    cudaMemcpy(
        d_A,
        h_A,
        bytes,
        cudaMemcpyHostToDevice
    );



    dim3 block(
        TILE,
        TILE
    );


    dim3 grid(
        N/TILE,
        N/TILE
    );



    cudaEvent_t start,stop;

    cudaEventCreate(&start);
    cudaEventCreate(&stop);



    // ============================
    // naive
    // ============================


    cudaEventRecord(start);


    transpose_naive<<<grid,block>>>(
        d_A,
        d_B,
        N
    );


    cudaEventRecord(stop);

    cudaEventSynchronize(stop);



    float t1;


    cudaEventElapsedTime(
        &t1,
        start,
        stop
    );



    printf(
        "\nNaive time: %.3f ms\n",
        t1
    );



    float GB =
        2.0*bytes/1e9;


    printf(
        "Bandwidth: %.2f GB/s\n",
        GB/(t1/1000)
    );





    // ============================
    // shared
    // ============================


    cudaEventRecord(start);



    transpose_shared<<<grid,block>>>(
        d_A,
        d_B,
        N
    );



    cudaEventRecord(stop);

    cudaEventSynchronize(stop);



    float t2;


    cudaEventElapsedTime(
        &t2,
        start,
        stop
    );



    printf(
        "\nShared time: %.3f ms\n",
        t2
    );



    printf(
        "Bandwidth: %.2f GB/s\n",
        GB/(t2/1000)
    );



    printf(
        "\nSpeedup %.2fx\n",
        t1/t2
    );



    cudaFree(d_A);
    cudaFree(d_B);


    free(h_A);


    return 0;
}