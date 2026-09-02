#include <cstdio>
#include <cuda_runtime.h>

// --------------------------------------------------
// GPU Constant Memory
// 所有线程都可以读取
// Kernel 内不能修改
// --------------------------------------------------
__constant__ float weights[4];


// --------------------------------------------------
// CUDA Kernel
// --------------------------------------------------
__global__ void constantMemoryExample(
    const float* input,
    float* output,
    int N)
{
    int idx =
        blockIdx.x * blockDim.x
        + threadIdx.x;

    if (idx < N)
    {
        // 根据 idx 选择 0~3 中的一个权重
        int weight_idx = idx % 4;

        // 从 Constant Memory 读取
        float w = weights[weight_idx];

        // 从 Global Memory 读取 input
        float x = input[idx];

        // 计算
        float result = x * w;

        // 写回 Global Memory
        output[idx] = result;

        printf(
            "block=%d thread=%d idx=%d "
            "input=%.1f weight[%d]=%.1f result=%.1f\n",
            blockIdx.x,
            threadIdx.x,
            idx,
            x,
            weight_idx,
            w,
            result
        );
    }
}


int main()
{
    const int N = 8;

    // CPU 数据
    float h_input[N] =
    {
        1, 2, 3, 4,
        5, 6, 7, 8
    };

    float h_output[N] = {0};

    // CPU 上准备的常量
    float h_weights[4] =
    {
        1, 2, 3, 4
    };


    // --------------------------------------------------
    // 把 CPU 的 h_weights
    // 复制到 GPU Constant Memory
    // --------------------------------------------------
    cudaMemcpyToSymbol(
        weights,
        h_weights,
        sizeof(h_weights)
    );


    // --------------------------------------------------
    // Global Memory
    // --------------------------------------------------
    float* d_input;
    float* d_output;

    cudaMalloc(
        &d_input,
        N * sizeof(float)
    );

    cudaMalloc(
        &d_output,
        N * sizeof(float)
    );


    // CPU → GPU Global Memory
    cudaMemcpy(
        d_input,
        h_input,
        N * sizeof(float),
        cudaMemcpyHostToDevice
    );


    // --------------------------------------------------
    // 启动 Kernel
    // --------------------------------------------------
    int threadsPerBlock = 4;

    int blocksPerGrid =
        (N + threadsPerBlock - 1)
        / threadsPerBlock;

    constantMemoryExample<<<
        blocksPerGrid,
        threadsPerBlock
    >>>(
        d_input,
        d_output,
        N
    );


    cudaDeviceSynchronize();


    // --------------------------------------------------
    // GPU → CPU
    // --------------------------------------------------
    cudaMemcpy(
        h_output,
        d_output,
        N * sizeof(float),
        cudaMemcpyDeviceToHost
    );


    // --------------------------------------------------
    // 打印最终结果
    // --------------------------------------------------
    printf("\nFinal result:\n\n");

    for (int i = 0; i < N; i++)
    {
        printf(
            "output[%d] = %.1f\n",
            i,
            h_output[i]
        );
    }


    cudaFree(d_input);
    cudaFree(d_output);

    return 0;
}