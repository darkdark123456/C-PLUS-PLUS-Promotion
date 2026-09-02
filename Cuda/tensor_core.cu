#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>

#include <mma.h>
#include <cuda_fp16.h>


using namespace nvcuda;


#define SIZE 4096
#define TILE 16



// =================================================
// Tensor Core WMMA kernel
// 一个warp计算一个16x16 C tile
// =================================================

__global__ void wmmaKernel(
    half *A,
    half *B,
    float *C
)
{

    // 一个block只有一个warp
    if(threadIdx.x >= 32)
        return;



    // 当前block负责哪个C tile
    int tileRow = blockIdx.y;

    int tileCol = blockIdx.x;



    // A fragment
    wmma::fragment<
        wmma::matrix_a,
        16,
        16,
        16,
        half,
        wmma::row_major
    > a_frag;



    // B fragment
    wmma::fragment<
        wmma::matrix_b,
        16,
        16,
        16,
        half,
        wmma::col_major
    > b_frag;



    // C accumulator
    wmma::fragment<
        wmma::accumulator,
        16,
        16,
        16,
        float
    > c_frag;



    // C=0

    wmma::fill_fragment(
        c_frag,
        0.0f
    );



    // K方向循环

    // 4096/16=256次

    for(int k=0;k<SIZE/TILE;k++)
    {


        /*
            A tile位置

            A:

            tileRow决定行

            k决定列
        */


        half *tileA =
            A
            +
            tileRow*TILE*SIZE
            +
            k*TILE;



        /*
            B tile位置

            B:

            k决定行

            tileCol决定列

        */


        half *tileB =
            B
            +
            k*TILE*SIZE
            +
            tileCol*TILE;



        // 加载A

        wmma::load_matrix_sync(
            a_frag,
            tileA,
            SIZE
        );



        // 加载B

        wmma::load_matrix_sync(
            b_frag,
            tileB,
            SIZE
        );



        // Tensor Core计算

        wmma::mma_sync(
            c_frag,
            a_frag,
            b_frag,
            c_frag
        );

    }



    // C写回位置

    float *tileC =
        C
        +
        tileRow*TILE*SIZE
        +
        tileCol*TILE;



    wmma::store_matrix_sync(
        tileC,
        c_frag,
        SIZE,
        wmma::mem_row_major
    );

}




int main()
{


    printf("Matrix size: %d x %d\n",
           SIZE,
           SIZE);



    size_t sizeA =
        SIZE*SIZE*sizeof(half);


    size_t sizeB =
        SIZE*SIZE*sizeof(half);


    size_t sizeC =
        SIZE*SIZE*sizeof(float);



    // ==========================
    // Host memory
    // ==========================


    half *h_A =
        (half*)malloc(sizeA);


    half *h_B =
        (half*)malloc(sizeB);


    float *h_C =
        (float*)malloc(sizeC);



    // 初始化

    for(long long i=0;i<(long long)SIZE*SIZE;i++)
    {

        h_A[i]=__float2half(1.0f);

        h_B[i]=__float2half(2.0f);

        h_C[i]=0;

    }



    // ==========================
    // Device memory
    // ==========================


    half *d_A;

    half *d_B;

    float *d_C;



    cudaMalloc(
        &d_A,
        sizeA
    );


    cudaMalloc(
        &d_B,
        sizeB
    );


    cudaMalloc(
        &d_C,
        sizeC
    );



    cudaMemcpy(
        d_A,
        h_A,
        sizeA,
        cudaMemcpyHostToDevice
    );


    cudaMemcpy(
        d_B,
        h_B,
        sizeB,
        cudaMemcpyHostToDevice
    );



    // ==========================
    // Kernel launch
    // ==========================


    dim3 block(32);


    dim3 grid(
        SIZE/TILE,
        SIZE/TILE
    );


    printf(
        "Launching kernel...\n"
    );


    wmmaKernel<<<
        grid,
        block
    >>>(
        d_A,
        d_B,
        d_C
    );



    cudaError_t err =
        cudaDeviceSynchronize();



    if(err != cudaSuccess)
    {
        printf(
            "CUDA error: %s\n",
            cudaGetErrorString(err)
        );

        return -1;
    }



    cudaMemcpy(
        h_C,
        d_C,
        sizeC,
        cudaMemcpyDeviceToHost
    );



    // ==========================
    // 检查结果
    // ==========================


    printf("\nCheck result:\n");


    for(int i=0;i<5;i++)
    {

        for(int j=0;j<5;j++)
        {

            printf(
                "%.1f ",
                h_C[i*SIZE+j]
            );

        }

        printf("\n");

    }



    printf(
        "\nExpected value = 8192\n"
    );



    // ==========================
    // free
    // ==========================


    cudaFree(d_A);

    cudaFree(d_B);

    cudaFree(d_C);


    free(h_A);

    free(h_B);

    free(h_C);



    return 0;
}