#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

// ============================================================
// 1. GPU Kernel
// ============================================================
__global__ void vecAdd(const float* A,
                       const float* B,
                       float* C,
                       int N)
{
    // 当前线程在整个 Grid 中的全局编号
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    // 防止线程编号超过数组长度
    if (i < N)
    {
        C[i] = A[i] + B[i];

        // Debug 时可以打开
        printf(
            "block=%d, thread=%d, global_id=%d, "
            "A=%.1f, B=%.1f, C=%.1f\n",
            blockIdx.x,
            threadIdx.x,
            i,
            A[i],
            B[i],
            C[i]
        );
    }
}


// ============================================================
// 2. CUDA 错误检查
// ============================================================
void checkCuda(cudaError_t error, const char* message)
{
    if (error != cudaSuccess)
    {
        printf(
            "CUDA ERROR: %s\n%s\n",
            message,
            cudaGetErrorString(error)
        );

        exit(EXIT_FAILURE);
    }
}


// ============================================================
// 3. main
// ============================================================
int main()
{
    // --------------------------------------------------------
    // Step 1：设置数组长度
    // --------------------------------------------------------
    const int N = 64;

    size_t bytes = N * sizeof(float);


    // --------------------------------------------------------
    // Step 2：CPU 内存
    // --------------------------------------------------------
    float* h_A = (float*)malloc(bytes);
    float* h_B = (float*)malloc(bytes);
    float* h_C = (float*)malloc(bytes);


    // --------------------------------------------------------
    // Step 3：初始化数据
    // --------------------------------------------------------
    for (int i = 0; i < N; i++)
    {
        h_A[i] = (float)i;
        h_B[i] = (float)(i * 10);
        h_C[i] = 0.0f;
    }


    // --------------------------------------------------------
    // Step 4：GPU 显存指针
    // --------------------------------------------------------
    float* d_A = nullptr;
    float* d_B = nullptr;
    float* d_C = nullptr;


    // --------------------------------------------------------
    // Step 5：申请 GPU 显存
    // --------------------------------------------------------
    checkCuda(
        cudaMalloc(&d_A, bytes),
        "cudaMalloc d_A failed"
    );

    checkCuda(
        cudaMalloc(&d_B, bytes),
        "cudaMalloc d_B failed"
    );

    checkCuda(
        cudaMalloc(&d_C, bytes),
        "cudaMalloc d_C failed"
    );


    // --------------------------------------------------------
    // Step 6：CPU → GPU
    // --------------------------------------------------------
    checkCuda(
        cudaMemcpy(
            d_A,
            h_A,
            bytes,
            cudaMemcpyHostToDevice
        ),
        "copy A failed"
    );

    checkCuda(
        cudaMemcpy(
            d_B,
            h_B,
            bytes,
            cudaMemcpyHostToDevice
        ),
        "copy B failed"
    );


    // --------------------------------------------------------
    // Step 7：设置 Grid / Block
    // --------------------------------------------------------

    // 为了方便 Debug，先让每个 Block 只有 4 个线程
    int threadsPerBlock = 4;

    // 向上取整
    int blocksPerGrid =
        (N + threadsPerBlock - 1)
        / threadsPerBlock;


    printf("N                = %d\n", N);
    printf("threadsPerBlock  = %d\n", threadsPerBlock);
    printf("blocksPerGrid    = %d\n\n", blocksPerGrid);


    // --------------------------------------------------------
    // Step 8：启动 Kernel
    // --------------------------------------------------------
    vecAdd<<<blocksPerGrid, threadsPerBlock>>>(
        d_A,
        d_B,
        d_C,
        N
    );


    // --------------------------------------------------------
    // Step 9：检查 Kernel 启动错误
    // --------------------------------------------------------
    checkCuda(
        cudaGetLastError(),
        "kernel launch failed"
    );


    // --------------------------------------------------------
    // Step 10：等待 GPU 完成
    // --------------------------------------------------------
    checkCuda(
        cudaDeviceSynchronize(),
        "kernel execution failed"
    );


    // --------------------------------------------------------
    // Step 11：GPU → CPU
    // --------------------------------------------------------
    checkCuda(
        cudaMemcpy(
            h_C,
            d_C,
            bytes,
            cudaMemcpyDeviceToHost
        ),
        "copy C failed"
    );


    // --------------------------------------------------------
    // Step 12：打印最终结果
    // --------------------------------------------------------
    printf("\nFinal Result:\n");

    for (int i = 0; i < N; i++)
    {
        printf(
            "C[%2d] = %6.1f + %6.1f = %6.1f\n",
            i,
            h_A[i],
            h_B[i],
            h_C[i]
        );
    }


    // --------------------------------------------------------
    // Step 13：释放 GPU 显存
    // --------------------------------------------------------
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);


    // --------------------------------------------------------
    // Step 14：释放 CPU 内存
    // --------------------------------------------------------
    free(h_A);
    free(h_B);
    free(h_C);


    return 0;
}