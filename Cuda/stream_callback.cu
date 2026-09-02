#include <stdio.h>
#include <cuda_runtime.h>


// ============================
// GPU Kernel
// ============================

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


        printf(
            "GPU thread=%d A=%.1f C=%.1f\n",
            idx,
            A[idx],
            C[idx]
        );
    }
}



// ============================
// Callback函数
// 注意：运行在CPU
// ============================

void CUDART_CB callback(void *data)
{
    printf("\n===== Callback =====\n");

    printf("GPU task finished!\n");

    printf("====================\n\n");
}




int main()
{

    const int N = 8;

    size_t size =
        N * sizeof(float);



    // ----------------------
    // Host pinned memory
    // ----------------------

    float *h_A;
    float *h_C;


    cudaMallocHost(
        &h_A,
        size
    );


    cudaMallocHost(
        &h_C,
        size
    );


    for(int i=0;i<N;i++)
    {
        h_A[i]=i+1;
        h_C[i]=0;
    }



    // ----------------------
    // Device memory
    // ----------------------

    float *d_A;
    float *d_C;


    cudaMalloc(
        &d_A,
        size
    );


    cudaMalloc(
        &d_C,
        size
    );



    // ----------------------
    // Create Stream
    // ----------------------

    cudaStream_t stream;


    cudaStreamCreate(
        &stream
    );



    printf(
        "CPU: start\n"
    );



    // ----------------------
    // 1. Async H2D
    // ----------------------

    cudaMemcpyAsync(
        d_A,
        h_A,
        size,
        cudaMemcpyHostToDevice,
        stream
    );


    printf(
        "CPU: H2D submitted\n"
    );



    // ----------------------
    // 2. Kernel
    // ----------------------

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


    printf(
        "CPU: kernel submitted\n"
    );



    // ----------------------
    // 3. Async D2H
    // ----------------------

    cudaMemcpyAsync(
        h_C,
        d_C,
        size,
        cudaMemcpyDeviceToHost,
        stream
    );


    printf(
        "CPU: D2H submitted\n"
    );



    // ----------------------
    // 4. Callback
    // ----------------------

    cudaLaunchHostFunc(
        stream,
        callback,
        nullptr
    );


    printf(
        "CPU: callback submitted\n"
    );



    // ----------------------
    // Wait GPU
    // ----------------------

    cudaStreamSynchronize(
        stream
    );


    printf(
        "CPU: stream finished\n\n"
    );



    // ----------------------
    // Check result
    // ----------------------

    for(int i=0;i<N;i++)
    {
        printf(
            "C[%d]=%.1f\n",
            i,
            h_C[i]
        );
    }



    // ----------------------
    // Free
    // ----------------------

    cudaStreamDestroy(
        stream
    );


    cudaFree(d_A);

    cudaFree(d_C);


    cudaFreeHost(h_A);

    cudaFreeHost(h_C);



    return 0;
}