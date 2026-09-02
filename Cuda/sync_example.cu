#include <cstdio>
#include <cuda_runtime.h>

__global__ void syncExample(const float* input, float* output)
{
    __shared__ float sdata[4];

    int tid = threadIdx.x;

    // 每个线程负责写一个共享内存位置
    sdata[tid] = input[tid];

    printf(
        "WRITE: thread=%d  sdata[%d]=%.1f\n",
        tid,
        tid,
        sdata[tid]
    );

    // 等待这个 Block 中所有线程都完成上面的写入
    __syncthreads();

    // 每个线程读取“下一个线程”写入的数据
    int next = (tid + 1) % 4;

    float value = sdata[next];

    output[tid] = value;

    printf(
        "READ : thread=%d  read sdata[%d]=%.1f\n",
        tid,
        next,
        value
    );
}


int main()
{
    const int N = 4;

    float h_input[N] =
    {
        10,
        20,
        30,
        40
    };

    float h_output[N] = {0};

    float* d_input;
    float* d_output;

    cudaMalloc(&d_input, N * sizeof(float));
    cudaMalloc(&d_output, N * sizeof(float));

    cudaMemcpy(
        d_input,
        h_input,
        N * sizeof(float),
        cudaMemcpyHostToDevice
    );

    syncExample<<<1, 4>>>(d_input, d_output);

    cudaDeviceSynchronize();

    cudaMemcpy(
        h_output,
        d_output,
        N * sizeof(float),
        cudaMemcpyDeviceToHost
    );

    printf("\nFinal output:\n");

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