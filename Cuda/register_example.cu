#include <cstdio>
#include <cuda_runtime.h>

__global__ void registerExample(const float* A, const float* B, float* C, int N)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < N)
    {
        // 这些都是普通局部变量
        // 编译器通常会优先放入寄存器
        float a = A[idx];
        float b = B[idx];

        float sum = a + b;
        float result = sum * 2.0f;

        C[idx] = result;

        printf(
            "block=%d thread=%d idx=%d "
            "a=%.1f b=%.1f sum=%.1f result=%.1f\n",
            blockIdx.x,
            threadIdx.x,
            idx,
            a,
            b,
            sum,
            result
        );
    }
}

int main()
{
    const int N = 8;

    float h_A[N] = {1, 2, 3, 4, 5, 6, 7, 8};
    float h_B[N] = {10, 20, 30, 40, 50, 60, 70, 80};
    float h_C[N] = {0};

    float* d_A;
    float* d_B;
    float* d_C;

    cudaMalloc(&d_A, N * sizeof(float));
    cudaMalloc(&d_B, N * sizeof(float));
    cudaMalloc(&d_C, N * sizeof(float));

    cudaMemcpy(
        d_A,
        h_A,
        N * sizeof(float),
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        d_B,
        h_B,
        N * sizeof(float),
        cudaMemcpyHostToDevice
    );

    int threadsPerBlock = 4;
    int blocksPerGrid =
        (N + threadsPerBlock - 1) / threadsPerBlock;

    registerExample<<<blocksPerGrid, threadsPerBlock>>>(
        d_A,
        d_B,
        d_C,
        N
    );

    cudaDeviceSynchronize();

    cudaMemcpy(
        h_C,
        d_C,
        N * sizeof(float),
        cudaMemcpyDeviceToHost
    );

    printf("\nFinal result:\n");

    for (int i = 0; i < N; i++)
    {
        printf(
            "C[%d] = (%.1f + %.1f) * 2 = %.1f\n",
            i,
            h_A[i],
            h_B[i],
            h_C[i]
        );
    }

    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return 0;
}