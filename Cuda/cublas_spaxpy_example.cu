#include <stdio.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>


void func_nrm2(){

    const int N = 2;


    float h_X[N]=
    {
        3.0f,
        4.0f
    };


    float *d_X;


    cudaMalloc(
        &d_X,
        N*sizeof(float)
    );



    cudaMemcpy(
        d_X,
        h_X,
        N*sizeof(float),
        cudaMemcpyHostToDevice
    );



    // 创建cuBLAS handle

    cublasHandle_t handle;


    cublasCreate(
        &handle
    );



    float result;



    /*
    
    result = sqrt(
        X0^2+X1^2
    )

    */


    cublasSnrm2(
        handle,

        N,

        d_X,

        1,

        &result
    );



    printf(
        "norm = %.2f\n",
        result
    );



    cublasDestroy(
        handle
    );


    cudaFree(d_X);
}





void func_dot(){


cublasHandle_t handle;
cublasCreate(&handle);

float h_A[3]={1,2,3};
float h_B[3]={4,5,6};


float *d_A;
float *d_B;


cudaMalloc(
    &d_A,
    3*sizeof(float)
);


cudaMalloc(
    &d_B,
    3*sizeof(float)
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



float result;


cublasSdot(
    handle,

    3,

    d_A,
    1,

    d_B,
    1,

    &result
);


printf(
    "dot=%f\n",
    result
);


}




int main()
{



    func_nrm2();
    func_dot();


    return 1;


    const int N = 5;


    float h_X[N] =
    {
        1,2,3,4,5
    };


    float h_Y[N] =
    {
        10,20,30,40,50
    };


    float *d_X;
    float *d_Y;


    cudaMalloc(
        &d_X,
        N*sizeof(float)
    );

    cudaMalloc(
        &d_Y,
        N*sizeof(float)
    );


    cudaMemcpy(
        d_X,
        h_X,
        N*sizeof(float),
        cudaMemcpyHostToDevice
    );


    cudaMemcpy(
        d_Y,
        h_Y,
        N*sizeof(float),
        cudaMemcpyHostToDevice
    );



    cublasHandle_t handle;

    cublasCreate(
        &handle
    );


    float alpha = 2.0f;


    /*
    
    Y = alpha*X + Y

    */

    cublasSaxpy(
        handle,

        N,

        &alpha,

        d_X,
        1,

        d_Y,
        1
    );



    cudaMemcpy(
        h_Y,
        d_Y,
        N*sizeof(float),
        cudaMemcpyDeviceToHost
    );


    for(int i=0;i<N;i++)
    {
        printf(
            "%.1f ",
            h_Y[i]
        );
    }


    printf("\n");


    cublasDestroy(handle);

    cudaFree(d_X);
    cudaFree(d_Y);


    return 0;
}