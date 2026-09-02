#include <stdio.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>


int main()
{

    const int batch = 3;

    const int M=2;
    const int N=2;
    const int K=2;



    float h_A[] =
    {
        // A0
        1,3,
        2,4,

        // A1
        10,30,
        20,40,

        // A2
        2,4,
        3,5
    };



    float h_B[] =
    {
        // B0
        5,7,
        6,8,

        // B1
        1,3,
        2,4,

        // B2
        6,8,
        7,9
    };


    float h_C[12]={0};



    float *d_A;
    float *d_B;
    float *d_C;


    cudaMalloc(
        &d_A,
        sizeof(h_A)
    );


    cudaMalloc(
        &d_B,
        sizeof(h_B)
    );


    cudaMalloc(
        &d_C,
        sizeof(h_C)
    );



    cudaMemcpy(
        d_A,
        h_A,
        sizeof(h_A),
        cudaMemcpyHostToDevice
    );


    cudaMemcpy(
        d_B,
        h_B,
        sizeof(h_B),
        cudaMemcpyHostToDevice
    );



    cublasHandle_t handle;


    cublasCreate(
        &handle
    );



    float alpha=1.0f;
    float beta=0.0f;



    int strideA = M*K;

    int strideB = K*N;

    int strideC = M*N;



    cublasSgemmStridedBatched(

        handle,


        CUBLAS_OP_N,
        CUBLAS_OP_N,


        M,
        N,
        K,


        &alpha,


        d_A,
        M,
        strideA,


        d_B,
        K,
        strideB,


        &beta,


        d_C,
        M,
        strideC,


        batch

    );



    cudaMemcpy(
        h_C,
        d_C,
        sizeof(h_C),
        cudaMemcpyDeviceToHost
    );



    for(int b=0;b<batch;b++)
    {
        printf("\nC%d:\n",b);


        for(int i=0;i<M;i++)
        {
            for(int j=0;j<N;j++)
            {
                printf(
                    "%.1f ",
                    h_C[b*4+i+j*2]
                );
            }

            printf("\n");
        }
    }



    cublasDestroy(handle);


    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);


    return 0;
}