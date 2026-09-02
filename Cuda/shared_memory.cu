#include <cstdio>
#include <cuda_runtime.h>

__global__ void sharedMemoryExample(
    const float* input,
    float* output,
    int N)
{
    // 每个 Block 有自己的一份共享内存
    __shared__ float shared_data[4];

    int tid = threadIdx.x;

    // 全局线程编号
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // --------------------------------
    // 第一步：Global Memory → Shared Memory
    // --------------------------------
    if (idx < N)
    {
        shared_data[tid] = input[idx];
    }

    // 等待整个 Block 的线程都加载完成
    __syncthreads();

    // --------------------------------
    // 第二步：从 Shared Memory 读取并计算
    // --------------------------------
    if (idx < N)
    {
        if (tid == 0)
        {
            // Block 中第一个线程没有左邻居
            output[idx] = shared_data[tid];
        }
        else
        {
            output[idx] =
                shared_data[tid]
                +
                shared_data[tid - 1];
        }
    }
}


int main()
{
    const int N = 8;

    float h_input[N] =
    {
        0, 1, 2, 3,
        4, 5, 6, 7
    };

    float h_output[N] = {0};

    float* d_input;
    float* d_output;

    // GPU Global Memory
    cudaMalloc(&d_input, N * sizeof(float));
    cudaMalloc(&d_output, N * sizeof(float));

    // CPU → GPU
    cudaMemcpy(
        d_input,
        h_input,
        N * sizeof(float),
        cudaMemcpyHostToDevice
    );

    // 每个 Block 4 个线程
    int threadsPerBlock = 4;

    // 一共 2 个 Block
    int blocksPerGrid =
        (N + threadsPerBlock - 1)
        / threadsPerBlock;

    printf("blocks = %d\n", blocksPerGrid);
    printf("threads/block = %d\n\n", threadsPerBlock);

    // 启动 Kernel
    sharedMemoryExample<<<
        blocksPerGrid,
        threadsPerBlock
    >>>(
        d_input,
        d_output,
        N
    );

    cudaDeviceSynchronize();

    // GPU → CPU
    cudaMemcpy(
        h_output,
        d_output,
        N * sizeof(float),
        cudaMemcpyDeviceToHost
    );

    printf("Input:\n");

    for (int i = 0; i < N; i++)
    {
        printf("%.0f ", h_input[i]);
    }

    printf("\n\nOutput:\n");

    for (int i = 0; i < N; i++)
    {
        printf("%.0f ", h_output[i]);
    }

    printf("\n");

    cudaFree(d_input);
    cudaFree(d_output);

    return 0;
}