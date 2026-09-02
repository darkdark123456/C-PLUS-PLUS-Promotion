#include <cstdio>
#include <cuda_runtime.h>

__global__ void histogram(int *data, int *hist, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    if (tid < n)
    {
        // 当前线程负责读取一个数据
        int value = data[tid];

        // 当前数据应该统计到哪个 bin
        int bin = value % 6;

        // 原子加
        int old = atomicAdd(&hi.t[bin], 1);

        printf(
            "thread=%d  "
            "data[%d]=%d  "
            "bin=%d  "
            "old=%d  "
            "new=%d\n",
            tid,
            tid,
            value,
            bin,
            old,
            old + 1
        );
    }
}

int main()
{
    const int N = 8;
    const int NUM_BINS = 6;

    int h_data[N] =
    {
        2, 5, 2, 2, 5, 1, 3, 2
    };

    int h_hist[NUM_BINS] =
    {
        0, 0, 0, 0, 0, 0
    };

    int *d_data;
    int *d_hist;

    cudaMalloc(&d_data, N * sizeof(int));
    cudaMalloc(&d_hist, NUM_BINS * sizeof(int));

    cudaMemcpy(
        d_data,
        h_data,
        N * sizeof(int),
        cudaMemcpyHostToDevice
    );

    cudaMemcpy(
        d_hist,
        h_hist,
        NUM_BINS * sizeof(int),
        cudaMemcpyHostToDevice
    );

    // 1 个 Block，8 个线程
    histogram<<<1, 8>>>(d_data, d_hist, N);

    cudaDeviceSynchronize();

    cudaMemcpy(
        h_hist,
        d_hist,
        NUM_BINS * sizeof(int),
        cudaMemcpyDeviceToHost
    );

    printf("\nFinal Histogram:\n");

    for (int i = 0; i < NUM_BINS; i++)
    {
        printf(
            "hist[%d] = %d\n",
            i,
            h_hist[i]
        );
    }

    cudaFree(d_data);
    cudaFree(d_hist);

    return 0;
}