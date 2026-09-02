#include <stdio.h>
#include <math.h>

#include <cuda_runtime.h>
#include <cufft.h>


#define PI 3.14159265358979323846


int main()
{

    // ============================
    // 参数
    // ============================

    int N = 1024;          // FFT点数

    float Fs = 1000.0f;    // 采样率 Hz

    float signalFreq = 100.0f; // 信号频率 Hz



    // ============================
    // CPU生成信号
    // ============================


    cufftComplex *h_signal;


    h_signal =
        (cufftComplex*)malloc(
            sizeof(cufftComplex)*N
        );



    for(int i=0;i<N;i++)
    {

        float t =
            (float)i / Fs;


        float value =
            sinf(
                2*PI*signalFreq*t
            );


        h_signal[i].x = value; // real

        h_signal[i].y = 0.0f;  // imaginary


    }



    printf("Input signal:\n");


    for(int i=0;i<10;i++)
    {
        printf(
            "%.3f\n",
            h_signal[i].x
        );
    }



    // ============================
    // GPU memory
    // ============================


    cufftComplex *d_signal;


    cudaMalloc(
        &d_signal,
        sizeof(cufftComplex)*N
    );



    cudaMemcpy(
        d_signal,
        h_signal,
        sizeof(cufftComplex)*N,
        cudaMemcpyHostToDevice
    );



    // ============================
    // 创建FFT plan
    // ============================


    cufftHandle plan;



    cufftPlan1d(
        &plan,

        N,

        CUFFT_C2C,

        1
    );



    printf(
        "\nFFT start\n"
    );



    // ============================
    // 执行FFT
    // ============================


    cufftExecC2C(

        plan,

        d_signal,

        d_signal,

        CUFFT_FORWARD

    );



    cudaDeviceSynchronize();



    // ============================
    // 拷贝结果
    // ============================


    cufftComplex *h_fft;


    h_fft =
        (cufftComplex*)malloc(
            sizeof(cufftComplex)*N
        );



    cudaMemcpy(
        h_fft,

        d_signal,

        sizeof(cufftComplex)*N,

        cudaMemcpyDeviceToHost
    );



    // ============================
    // 找最大频率
    // ============================


    float maxMagnitude = 0;

    int maxIndex = 0;



    for(int i=0;i<N/2;i++)
    {

        float real =
            h_fft[i].x;


        float imag =
            h_fft[i].y;



        float magnitude =
            sqrtf(
                real*real
                +
                imag*imag
            );



        if(
            magnitude > maxMagnitude
        )
        {
            maxMagnitude = magnitude;

            maxIndex = i;
        }

    }



    float frequency =
        (float)maxIndex
        *
        Fs
        /
        N;



    printf(
        "\nPeak frequency:\n"
    );


    printf(
        "index = %d\n",
        maxIndex
    );


    printf(
        "frequency = %.2f Hz\n",
        frequency
    );


    printf(
        "magnitude = %.2f\n",
        maxMagnitude
    );



    // ============================
    // 释放
    // ============================


    cufftDestroy(plan);


    cudaFree(d_signal);


    free(h_signal);

    free(h_fft);



    return 0;
}