#include <stdio.h>
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cublas_v2.h>


int main()
{

    int M=2;
    int N=2;
    int K=2;


    // FP16输入

    half h_A[4];

    half h_B[4];


    h_A[0]=__float2half(1);
    h_A[1]=__float2half(3);
    h_A[2]=__float2half(2);
    h_A[3]=__float2half(4);



    h_B[0]=__float2half(5);
    h_B[1]=__float2half(7);
    h_B[2]=__float2half(6);
    h_B[3]=__float2half(8);



    // FP32输出

    float h_C[4]={0};



    half *d_A;
    half *d_B;

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

    cublasCreate(&handle);



    float alpha=1.0f;
    float beta=0.0f;



    cublasGemmEx(

        handle,


        CUBLAS_OP_N,
        CUBLAS_OP_N,


        M,
        N,
        K,


        &alpha,


        d_A,
        CUDA_R_16F,
        M,


        d_B,
        CUDA_R_16F,
        K,


        &beta,


        d_C,
        CUDA_R_32F,
        M,


        CUDA_R_32F,


        CUBLAS_GEMM_DEFAULT

    );



    cudaMemcpy(
        h_C,
        d_C,
        sizeof(h_C),
        cudaMemcpyDeviceToHost
    );



    printf("C:\n");


    for(int i=0;i<2;i++)
    {
        for(int j=0;j<2;j++)
        {

            printf(
                "%.1f ",
                h_C[j*M+i]
            );

        }

        printf("\n");
    }



    cublasDestroy(handle);


    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);


    return 0;
}