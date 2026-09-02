#include <stdio.h>
#include <cuda_runtime.h>


// GPU kernel
__global__ void doubleKernel(
    float *A,
    float *C,
    int N
)
{
    int idx =
        blockIdx.x * blockDim.x
        +
        threadIdx.x;


    if(idx < N)
    {
        C[idx] = A[idx] * 2.0f;
    }
}



int main()
{
    const int N = 16;

    size_t size = N * sizeof(float);



    // -------------------------
    // CPU 内存
    // -------------------------

    float *h_A;
    float *h_C;

    float *h_B;
    float *h_D;


    // 注意：
    // Async需要Pinned Memory
    cudaMallocHost(
        &h_A,
        size
    );


    cudaMallocHost(
        &h_C,
        size
    );


    cudaMallocHost(
        &h_B,
        size
    );


    cudaMallocHost(
        &h_D,
        size
    );




    for(int i=0;i<N;i++)
    {
        h_A[i] = i + 1;
        h_C[i] = 0;

        h_B[i]=i+2;
        h_D[i]=0;
    }



    // -------------------------
    // GPU内存
    // -------------------------

    float *d_A;
    float *d_C;

    float *d_B;
    float *d_D;


    cudaMalloc(
        &d_A,
        size
    );


    cudaMalloc(
        &d_C,
        size
    );


    cudaMalloc(
        &d_B,
        size
    );

    cudaMalloc(
        &d_D,
        size
    );





    // -------------------------
    // 创建Stream
    // -------------------------

    cudaStream_t stream;
    cudaStream_t stream_1;


    cudaStreamCreate(
        &stream
    );

    cudaStreamCreate(
        &stream_1
    );




    printf("Start Async pipeline\n\n");



    // -------------------------
    // 1. 异步上传
    // -------------------------

    cudaMemcpyAsync(
        d_A,
        h_A,
        size,
        cudaMemcpyHostToDevice,
        stream
    );

    cudaMemcpyAsync(
        d_B,
        h_B,
        size,
        cudaMemcpyHostToDevice,
        stream_1
    );


    printf("CPU submit H2D copy\n");


    



    // -------------------------
    // 2. kernel加入stream
    // -------------------------

    dim3 block(4);

    dim3 grid(
        (N+block.x-1)/block.x
    );


    doubleKernel<<<
        grid,
        block,
        0,
        stream
    >>>(
        d_A,
        d_C,
        N
    );


    doubleKernel<<<
        grid,
        block,
        0,
        stream_1
    >>>(d_B,d_D,N);


    printf("CPU submit kernel\n");



    // -------------------------
    // 3. 异步下载
    // -------------------------

    cudaMemcpyAsync(
        h_C,
        d_C,
        size,
        cudaMemcpyDeviceToHost,
        stream
    );

    cudaMemcpyAsync(
        h_D,
        d_D,
        size,
        cudaMemcpyDeviceToHost,
        stream_1
    );

    cudaStreamSynchronize(
        stream
    );

    cudaStreamSynchronize(
        stream_1
    );

    printf("CPU submit D2H copy\n");
    printf("\nGPU finished\n\n");



    // -------------------------
    // 释放
    // -------------------------

    cudaStreamDestroy(
        stream
    );

    cudaStreamDestroy(
        stream_1;
    )


    cudaFree(d_A);


    cudaFree(d_C);

    cudaFree(d_B);
    cudaFree(d_D);


    cudaFreeHost(h_A);

    cudaFreeHost(h_C);

    cudaFreeHost(h_B);

    cudaFreeHost(h_D);


    return 0;
}