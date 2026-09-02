#include <stdio.h>

#include <cuda_runtime.h>
#include <cudnn.h>


#define CHECK_CUDA(call)                     \
{                                            \
    cudaError_t err = call;                  \
    if(err != cudaSuccess)                   \
    {                                        \
        printf("CUDA Error: %s\n",           \
        cudaGetErrorString(err));             \
        return -1;                           \
    }                                        \
}



#define CHECK_CUDNN(call)                    \
{                                            \
    cudnnStatus_t status = call;              \
    if(status != CUDNN_STATUS_SUCCESS)        \
    {                                        \
        printf("cuDNN Error: %s\n",           \
        cudnnGetErrorString(status));         \
        return -1;                           \
    }                                        \
}



int main()
{


    // =========================
    // 创建cuDNN handle
    // =========================

    cudnnHandle_t cudnn;


    CHECK_CUDNN(
        cudnnCreate(&cudnn)
    );



    // =========================
    // 输入参数
    // N C H W
    // =========================

    int N = 1;
    int C = 1;
    int H = 5;
    int W = 5;



    float h_input[25]=
    {
        1,2,3,4,5,

        6,7,8,9,10,

        11,12,13,14,15,

        16,17,18,19,20,

        21,22,23,24,25
    };



    // =========================
    // filter参数
    //
    // K C R S
    // =========================


    int K = 1;
    int R = 3;
    int S = 3;



    float h_filter[9]=
    {
        1,1,1,

        1,1,1,

        1,1,1
    };



    // 输出尺寸

    int outH = 3;
    int outW = 3;



    float h_output[9]={0};



    // =========================
    // GPU memory
    // =========================


    float *d_input;

    float *d_filter;

    float *d_output;



    CHECK_CUDA(
        cudaMalloc(
            &d_input,
            sizeof(h_input)
        )
    );


    CHECK_CUDA(
        cudaMalloc(
            &d_filter,
            sizeof(h_filter)
        )
    );


    CHECK_CUDA(
        cudaMalloc(
            &d_output,
            sizeof(h_output)
        )
    );



    CHECK_CUDA(
        cudaMemcpy(
            d_input,
            h_input,
            sizeof(h_input),
            cudaMemcpyHostToDevice
        )
    );



    CHECK_CUDA(
        cudaMemcpy(
            d_filter,
            h_filter,
            sizeof(h_filter),
            cudaMemcpyHostToDevice
        )
    );



    // =========================
    // 创建Tensor Descriptor
    // =========================


    cudnnTensorDescriptor_t input_desc;

    cudnnTensorDescriptor_t output_desc;



    CHECK_CUDNN(
        cudnnCreateTensorDescriptor(
            &input_desc
        )
    );


    CHECK_CUDNN(
        cudnnCreateTensorDescriptor(
            &output_desc
        )
    );



    // 输入:

    // NCHW

    CHECK_CUDNN(
        cudnnSetTensor4dDescriptor(
            input_desc,

            CUDNN_TENSOR_NCHW,

            CUDNN_DATA_FLOAT,

            N,
            C,
            H,
            W
        )
    );



    // 输出

    CHECK_CUDNN(
        cudnnSetTensor4dDescriptor(
            output_desc,

            CUDNN_TENSOR_NCHW,

            CUDNN_DATA_FLOAT,

            N,
            K,
            outH,
            outW
        )
    );



    // =========================
    // Filter Descriptor
    // =========================


    cudnnFilterDescriptor_t filter_desc;



    CHECK_CUDNN(
        cudnnCreateFilterDescriptor(
            &filter_desc
        )
    );



    CHECK_CUDNN(
        cudnnSetFilter4dDescriptor(
            filter_desc,

            CUDNN_DATA_FLOAT,

            CUDNN_TENSOR_NCHW,

            K,
            C,
            R,
            S
        )
    );



    // =========================
    // Convolution Descriptor
    // =========================


    cudnnConvolutionDescriptor_t conv_desc;



    CHECK_CUDNN(
        cudnnCreateConvolutionDescriptor(
            &conv_desc
        )
    );



    CHECK_CUDNN(
        cudnnSetConvolution2dDescriptor(

            conv_desc,

            0, // pad_h

            0, // pad_w

            1, // stride_h

            1, // stride_w

            1, // dilation_h

            1, // dilation_w

            CUDNN_CROSS_CORRELATION,

            CUDNN_DATA_FLOAT
        )
    );



    // =========================
    // 选择卷积算法
    // =========================

    cudnnConvolutionFwdAlgoPerf_t perf;

    int returnedAlgoCount;


    CHECK_CUDNN(
        cudnnGetConvolutionForwardAlgorithm_v7(

            cudnn,

            input_desc,

            filter_desc,

            conv_desc,

            output_desc,

            1,

            &returnedAlgoCount,

            &perf
        )
    );


    cudnnConvolutionFwdAlgo_t algo =
        perf.algo;
    


    // =========================
    // workspace
    // =========================


    size_t workspace_size=0;


    CHECK_CUDNN(
        cudnnGetConvolutionForwardWorkspaceSize(

            cudnn,

            input_desc,

            filter_desc,

            conv_desc,

            output_desc,

            algo,

            &workspace_size
        )
    );



    void *workspace=nullptr;



    if(workspace_size>0)
    {
        CHECK_CUDA(
            cudaMalloc(
                &workspace,
                workspace_size
            )
        );
    }



    // =========================
    // 执行卷积
    // =========================


    float alpha=1.0f;

    float beta=0.0f;



    CHECK_CUDNN(
        cudnnConvolutionForward(

            cudnn,

            &alpha,

            input_desc,

            d_input,

            filter_desc,

            d_filter,

            conv_desc,

            algo,

            workspace,

            workspace_size,

            &beta,

            output_desc,

            d_output
        )
    );



    CHECK_CUDA(
        cudaDeviceSynchronize()
    );



    CHECK_CUDA(
        cudaMemcpy(
            h_output,

            d_output,

            sizeof(h_output),

            cudaMemcpyDeviceToHost
        )
    );



    // =========================
    // 输出结果
    // =========================


    printf("Output:\n");


    for(int i=0;i<3;i++)
    {

        for(int j=0;j<3;j++)
        {

            printf(
                "%.1f ",
                h_output[i*3+j]
            );

        }

        printf("\n");
    }



    // =========================
    // release
    // =========================


    if(workspace)
        cudaFree(workspace);



    cudaFree(d_input);

    cudaFree(d_filter);

    cudaFree(d_output);



    cudnnDestroyTensorDescriptor(input_desc);

    cudnnDestroyTensorDescriptor(output_desc);

    cudnnDestroyFilterDescriptor(filter_desc);

    cudnnDestroyConvolutionDescriptor(conv_desc);


    cudnnDestroy(cudnn);



    return 0;
}