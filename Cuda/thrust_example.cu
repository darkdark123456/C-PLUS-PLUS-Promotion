#include <stdio.h>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include <thrust/sort.h>
#include <thrust/reduce.h>
#include <thrust/transform.h>
#include <thrust/copy.h>

#include <iostream>


// transform使用的函数
struct square
{
    __host__ __device__
    float operator()(float x)
    {
        return x * x;
    }
};



int main()
{

    const int N = 5;


    // ==============================
    // CPU数组
    // ==============================

    float h_data[N] =
    {
        8,
        3,
        5,
        1,
        9
    };


    printf("Original CPU data:\n");


    for(int i=0;i<N;i++)
    {
        printf("%.1f ", h_data[i]);
    }

    printf("\n\n");



    // ==============================
    // copy:
    // CPU -> GPU
    // ==============================


    thrust::device_vector<float> d_vec(
        h_data,
        h_data + N
    );


    printf("Copy CPU -> GPU finished\n\n");



    // ==============================
    // sort
    // ==============================

    thrust::sort(
        d_vec.begin(),
        d_vec.end()
    );


    printf("After thrust::sort:\n");



    printf("\n\n");



    // ==============================
    // reduce
    // 求和
    // ==============================

    float sum =
        thrust::reduce(
            d_vec.begin(),
            d_vec.end()
        );


    printf(
        "After thrust::reduce:\n"
    );


    printf(
        "sum = %.1f\n\n",
        sum
    );



    // ==============================
    // transform
    // 每个元素平方
    // ==============================


    thrust::transform(

        d_vec.begin(),

        d_vec.end(),

        d_vec.begin(),

        square()

    );



    printf(
        "After thrust::transform(square):\n"
    );



    printf("\n\n");



    // ==============================
    // copy
    // GPU -> CPU
    // ==============================


    thrust::host_vector<float> h_result(N);


    thrust::copy(

        d_vec.begin(),

        d_vec.end(),

        h_result.begin()

    );



    printf(
        "After thrust::copy GPU -> CPU:\n"
    );


    for(int i=0;i<N;i++)
    {
        printf(
            "%.1f ",
            h_result[i]
        );
    }


    printf("\n");



    return 0;
}