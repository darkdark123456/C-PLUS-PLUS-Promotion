#include <stdio.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>



int main()
{

    // A 2x2
    float h_A[] =
    {
        1,2,
        3,4
    };


    // B 2x2
    float h_B[] =
    {
        5,6,
        7,8
    };


    float h_C[4]={0};



    float *d_A;
    float *d_B;
    float *d_C;



    cudaMalloc(
        &d_A,
        sizeof(float)*4
    );


    cudaMalloc(
        &d_B,
        sizeof(float)*4
    );


    cudaMalloc(
        &d_C,
        sizeof(float)*4
    );



    cudaMemcpy(
        d_A,
        h_A,
        sizeof(float)*4,
        cudaMemcpyHostToDevice
    );


    cudaMemcpy(
        d_B,
        h_B,
        sizeof(float)*4,
        cudaMemcpyHostToDevice
    );



    // 创建cuBLAS句柄

    cublasHandle_t handle;


    cublasCreate(
        &handle
    );



    float alpha=1.0f;

    float beta=0.0f;



    /*
    
    C = alpha*A*B + beta*C

    */

    cublasSgemm(

        handle,


        CUBLAS_OP_T,
        CUBLAS_OP_T,


        2,
        2,
        2,


        &alpha,


        d_A,
        2,


        d_B,
        2,


        &beta,


        d_C,
        2

    );



    cudaMemcpy(
        h_C,
        d_C,
        sizeof(float)*4,
        cudaMemcpyDeviceToHost
    );



    printf("C=\n");


    for(int i=0;i<2;i++)
    {
        for(int j=0;j<2;j++)
        {
            printf(
                "%.1f ",
                h_C[i*2+j]
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