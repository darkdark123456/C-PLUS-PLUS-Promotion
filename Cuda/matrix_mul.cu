#include <cstdio>
#include <cuda_runtime.h>

__global__ void matMulKernel(
    const float* A,
    const float* B,
    float* C,
    int M,
    int N,
    int K)
{
    // y 决定行
    int row = blockIdx.y * blockDim.y + threadIdx.y;

    // x 决定列
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < M && col < N)
    {
        // 每个线程自己的一份局部变量
        // 通常会放在寄存器中
        float sum = 0.0f;

        for (int k = 0; k < K; k++)
        {
            float a = A[row * K + k];
            float b = B[k * N + col];

            sum += a * b;

            printf(
                "block=(%d,%d) "
                "thread=(%d,%d) "
                "row=%d col=%d k=%d "
                "a=%.1f b=%.1f sum=%.1f\n",

                blockIdx.x,
                blockIdx.y,

                threadIdx.x,
                threadIdx.y,

                row,
                col,
                k,

                a,
                b,
                sum
            );
        }

        C[row * N + col] = sum;
    }
}


int main()
{
    // A: M×K
    const int M = 2;
    const int K = 3;

    // B: K×N
    const int N = 2;

    float h_A[M * K] =
    {
        1, 2, 3,
        4, 5, 6
    };

    float h_B[K * N] =
    {
         7,  8,
         9, 10,
        11, 12
    };

    float h_C[M * N] = {0};


    float* d_A;
    float* d_B;
    float* d_C;


    cudaMalloc(&d_A, M * K * sizeof(float));
    cudaMalloc(&d_B, K * N * sizeof(float));
    cudaMalloc(&d_C, M * N * sizeof(float));


    cudaMemcpy(
        d_A,
        h_A,
        M * K * sizeof(float),
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        d_B,
        h_B,
        K * N * sizeof(float),
        cudaMemcpyHostToDevice
    );


    // ------------------------------
    // 二维 Thread Block
    // ------------------------------
    dim3 threadsPerBlock(2, 2);

    dim3 blocksPerGrid(
        (N + threadsPerBlock.x - 1) / threadsPerBlock.x,
        (M + threadsPerBlock.y - 1) / threadsPerBlock.y
    );


    printf("Grid = (%d, %d)\n",
           blocksPerGrid.x,
           blocksPerGrid.y);

    printf("Block = (%d, %d)\n\n",
           threadsPerBlock.x,
           threadsPerBlock.y);


    matMulKernel<<<
        blocksPerGrid,
        threadsPerBlock
    >>>(
        d_A,
        d_B,
        d_C,
        M,
        N,
        K
    );


    cudaDeviceSynchronize();


    cudaMemcpy(
        h_C,
        d_C,
        M * N * sizeof(float),
        cudaMemcpyDeviceToHost
    );


    printf("\nFinal Matrix C:\n\n");

    for (int row = 0; row < M; row++)
    {
        for (int col = 0; col < N; col++)
        {
            printf(
                "%6.1f ",
                h_C[row * N + col]
            );
        }

        printf("\n");
    }


    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return 0;
}