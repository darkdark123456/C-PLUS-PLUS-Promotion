#include <stdio.h>

#include <cuda_runtime.h>

#include <cudnn.h>
#include <cudnn_backend.h>
#include <cudnn_graph.h>


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


    cudnnHandle_t cudnn;


    CHECK_CUDNN(
        cudnnCreate(&cudnn)
    );



    // =========================
    // 输入
    // N C H W
    // =========================


    int N=1;
    int C=1;
    int H=5;
    int W=5;

    float h_input[25]=
    {
        1,2,3,4,5,

        6,7,8,9,10,

        11,12,13,14,15,

        16,17,18,19,20,

        21,22,23,24,25
    };



    // =========================
    // filter
    // K C R S
    // =========================


    int K=1;
    int R=3;
    int S=3;
    float h_filter[9]=
    {
        1,1,1,

        1,1,1,

        1,1,1
    };



    int outH=3;
    int outW=3;



    float h_output[9]={0};



    // =========================
    // device memory
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



    printf("CUDA memory ready\n");

    // =================================================
    // 创建 Backend Tensor Descriptor
    // X: input
    // W: filter
    // Y: output
    // =================================================


    cudnnBackendDescriptor_t x_tensor;
    cudnnBackendDescriptor_t w_tensor;
    cudnnBackendDescriptor_t y_tensor;



    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(
            CUDNN_BACKEND_TENSOR_DESCRIPTOR,
            &x_tensor
        )
    );


    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(
            CUDNN_BACKEND_TENSOR_DESCRIPTOR,
            &w_tensor
        )
    );


    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(
            CUDNN_BACKEND_TENSOR_DESCRIPTOR,
            &y_tensor
        )
    );



    // ==============================
    // Tensor dimensions
    //
    // N C H W
    // ==============================


    int64_t x_dim[4] =
    {
        N,
        C,
        H,
        W
    };


    int64_t w_dim[4] =
    {
        K,
        C,
        R,
        S
    };


    int64_t y_dim[4] =
    {
        N,
        K,
        outH,
        outW
    };



    // NCHW stride

    int64_t x_stride[4]=
    {
        C*H*W,
        H*W,
        W,
        1
    };


    int64_t w_stride[4]=
    {
        C*R*S,
        R*S,
        S,
        1
    };


    int64_t y_stride[4]=
    {
        K*outH*outW,
        outH*outW,
        outW,
        1
    };



    cudnnDataType_t dtype =
        CUDNN_DATA_FLOAT;



    // ==============================
    // X Tensor
    // ==============================


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            x_tensor,

            CUDNN_ATTR_TENSOR_DATA_TYPE,

            CUDNN_TYPE_DATA_TYPE,

            1,

            &dtype
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            x_tensor,

            CUDNN_ATTR_TENSOR_DIMENSIONS,

            CUDNN_TYPE_INT64,

            4,

            x_dim
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            x_tensor,

            CUDNN_ATTR_TENSOR_STRIDES,

            CUDNN_TYPE_INT64,

            4,

            x_stride
        )
    );






    // ==========================
    // alignment
    // ==========================

    int64_t alignment = 16;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(
            x_tensor,
            CUDNN_ATTR_TENSOR_BYTE_ALIGNMENT,
            CUDNN_TYPE_INT64,
            1,
            &alignment
        )
    );



    int64_t x_uid = 1;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(
            x_tensor,
            CUDNN_ATTR_TENSOR_UNIQUE_ID,
            CUDNN_TYPE_INT64,
            1,
            &x_uid
        )
    );




    bool is_virtual = false;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            x_tensor,

            CUDNN_ATTR_TENSOR_IS_VIRTUAL,

            CUDNN_TYPE_BOOLEAN,

            1,

            &is_virtual
        )
    );



    // ==============================
    // W Tensor
    // ==============================


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            w_tensor,

            CUDNN_ATTR_TENSOR_DATA_TYPE,

            CUDNN_TYPE_DATA_TYPE,

            1,

            &dtype
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            w_tensor,

            CUDNN_ATTR_TENSOR_DIMENSIONS,

            CUDNN_TYPE_INT64,

            4,

            w_dim
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            w_tensor,

            CUDNN_ATTR_TENSOR_STRIDES,

            CUDNN_TYPE_INT64,

            4,

            w_stride
        )
    );




    int64_t w_alignment = 16;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            w_tensor,

            CUDNN_ATTR_TENSOR_BYTE_ALIGNMENT,

            CUDNN_TYPE_INT64,

            1,

            &w_alignment
        )
    );





    int64_t w_uid = 2;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            w_tensor,

            CUDNN_ATTR_TENSOR_UNIQUE_ID,

            CUDNN_TYPE_INT64,

            1,

            &w_uid
        )
    );


    // ==============================
    // Y Tensor
    // ==============================


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            y_tensor,

            CUDNN_ATTR_TENSOR_DATA_TYPE,

            CUDNN_TYPE_DATA_TYPE,

            1,

            &dtype
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            y_tensor,

            CUDNN_ATTR_TENSOR_DIMENSIONS,

            CUDNN_TYPE_INT64,

            4,

            y_dim
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            y_tensor,

            CUDNN_ATTR_TENSOR_STRIDES,

            CUDNN_TYPE_INT64,

            4,

            y_stride
        )
    );



    int64_t y_alignment = 16;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            y_tensor,

            CUDNN_ATTR_TENSOR_BYTE_ALIGNMENT,

            CUDNN_TYPE_INT64,

            1,

            &y_alignment
        )
    );





    int64_t y_uid = 3;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            y_tensor,

            CUDNN_ATTR_TENSOR_UNIQUE_ID,

            CUDNN_TYPE_INT64,

            1,

            &y_uid
        )
    );



    // finalize tensor
    CHECK_CUDNN(
        cudnnBackendFinalize(x_tensor)
    );


    CHECK_CUDNN(
        cudnnBackendFinalize(w_tensor)
    );


    CHECK_CUDNN(
        cudnnBackendFinalize(y_tensor)
    );



    printf("Tensor descriptors ready\n");


    // =================================================
    // Convolution Descriptor
    // =================================================


    cudnnBackendDescriptor_t conv_desc;



    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(

            CUDNN_BACKEND_CONVOLUTION_DESCRIPTOR,

            &conv_desc
        )
    );



    // padding

    int64_t pre_padding[2] =
    {
        0,
        0
    };


    int64_t post_padding[2] =
    {
        0,
        0
    };


    // stride

    int64_t conv_stride[2] =
    {
        1,
        1
    };


    // dilation

    int64_t dilation[2] =
    {
        1,
        1
    };



    cudnnDataType_t conv_type =
        CUDNN_DATA_FLOAT;



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_desc,

            CUDNN_ATTR_CONVOLUTION_PRE_PADDINGS,

            CUDNN_TYPE_INT64,

            2,

            pre_padding
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_desc,

            CUDNN_ATTR_CONVOLUTION_POST_PADDINGS,

            CUDNN_TYPE_INT64,

            2,

            post_padding
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_desc,

            CUDNN_ATTR_CONVOLUTION_DILATIONS,

            CUDNN_TYPE_INT64,

            2,

            dilation
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_desc,

            
            CUDNN_ATTR_CONVOLUTION_FILTER_STRIDES,

            CUDNN_TYPE_INT64,

            2,

            conv_stride
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_desc,

            CUDNN_ATTR_CONVOLUTION_COMP_TYPE,

            CUDNN_TYPE_DATA_TYPE,

            1,

            &conv_type
        )
    );



    cudnnConvolutionMode_t conv_mode =
    CUDNN_CROSS_CORRELATION;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_desc,

            CUDNN_ATTR_CONVOLUTION_CONV_MODE,

            CUDNN_TYPE_CONVOLUTION_MODE,

            1,

            &conv_mode
        )
    );



    int64_t spatial_dims = 2;

    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_desc,

            CUDNN_ATTR_CONVOLUTION_SPATIAL_DIMS,

            CUDNN_TYPE_INT64,

            1,

            &spatial_dims
        )
    );


    CHECK_CUDNN(
        cudnnBackendFinalize(conv_desc)
    );



    printf("Convolution descriptor ready\n");




    // =================================================
    // 创建 Convolution Forward Operation
    // =================================================


    cudnnBackendDescriptor_t conv_op;



    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(

            CUDNN_BACKEND_OPERATION_CONVOLUTION_FORWARD_DESCRIPTOR,

            &conv_op
        )
    );



    float alpha = 1.0f;

    float beta  = 0.0f;



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_op,

            CUDNN_ATTR_OPERATION_CONVOLUTION_FORWARD_ALPHA,

            CUDNN_TYPE_FLOAT,

            1,

            &alpha
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_op,

            CUDNN_ATTR_OPERATION_CONVOLUTION_FORWARD_BETA,

            CUDNN_TYPE_FLOAT,

            1,

            &beta
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_op,

            CUDNN_ATTR_OPERATION_CONVOLUTION_FORWARD_CONV_DESC,

            CUDNN_TYPE_BACKEND_DESCRIPTOR,

            1,

            &conv_desc
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_op,

            CUDNN_ATTR_OPERATION_CONVOLUTION_FORWARD_X,

            CUDNN_TYPE_BACKEND_DESCRIPTOR,

            1,

            &x_tensor
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_op,

            CUDNN_ATTR_OPERATION_CONVOLUTION_FORWARD_W,

            CUDNN_TYPE_BACKEND_DESCRIPTOR,

            1,

            &w_tensor
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            conv_op,

            CUDNN_ATTR_OPERATION_CONVOLUTION_FORWARD_Y,

            CUDNN_TYPE_BACKEND_DESCRIPTOR,

            1,

            &y_tensor
        )
    );



    CHECK_CUDNN(
        cudnnBackendFinalize(conv_op)
    );



    printf("Convolution operation ready\n");

    // =================================================
    // Operation Graph
    // =================================================


    cudnnBackendDescriptor_t op_graph;



    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(

            CUDNN_BACKEND_OPERATIONGRAPH_DESCRIPTOR,

            &op_graph
        )
    );



    cudnnBackendDescriptor_t operations[1];

    operations[0] = conv_op;



//                     conv_desc
//                     |
//                     |
//                     ↓

// x_tensor  --->  conv_op  --->  y_tensor
//                     ↑
//                     |
//                 w_tensor


// alpha = 1
// beta  = 0













//                  conv_desc
//                      |
//                      |
//                      ↓

// x_tensor --------> conv_op --------> y1_tensor --------> relu_op --------> y_tensor
//                      ^
//                      |
//                  w_tensor


//Operation 是计算节点，Tensor 是数据流，Descriptor 是给 Operation 提供规则。



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            op_graph,

            CUDNN_ATTR_OPERATIONGRAPH_OPS,

            CUDNN_TYPE_BACKEND_DESCRIPTOR,

            1,

            operations
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            op_graph,

            CUDNN_ATTR_OPERATIONGRAPH_HANDLE,

            CUDNN_TYPE_HANDLE,

            1,

            &cudnn
        )
    );



    CHECK_CUDNN(
        cudnnBackendFinalize(op_graph)
    );


    printf("Operation graph ready\n");




    // =================================================
    // Engine Heuristic 探索引擎 算法推荐器 为上面的计算图推荐一个或者几个最优算法
    // =================================================


    cudnnBackendDescriptor_t heuristic;



    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(

            CUDNN_BACKEND_ENGINEHEUR_DESCRIPTOR,

            &heuristic
        )
    );


    
    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            heuristic,

            CUDNN_ATTR_ENGINEHEUR_OPERATION_GRAPH,

            CUDNN_TYPE_BACKEND_DESCRIPTOR,

            1,

            &op_graph
        )
    );


    //分析策略  快速启发模式
    cudnnBackendHeurMode_t mode =
        CUDNN_HEUR_MODE_INSTANT;



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            heuristic,

            CUDNN_ATTR_ENGINEHEUR_MODE,

            CUDNN_TYPE_HEUR_MODE,

            1,

            &mode
        )
    );



    CHECK_CUDNN(
        cudnnBackendFinalize(heuristic)
    );



    // =================================================
    // 获取 engine config 从算法推荐器里面拿到结果
    // =================================================


    cudnnBackendDescriptor_t engine_config;


    int64_t returned_count = 0;


    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(

            CUDNN_BACKEND_ENGINECFG_DESCRIPTOR,

            &engine_config
        )
    );



    CHECK_CUDNN(
        cudnnBackendGetAttribute(

            heuristic,

            CUDNN_ATTR_ENGINEHEUR_RESULTS,

            CUDNN_TYPE_BACKEND_DESCRIPTOR,

            1,

            &returned_count,

            &engine_config
        )
    );




    // =================================================
    // Execution Plan
    // =================================================


    cudnnBackendDescriptor_t execution_plan;



    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(

            CUDNN_BACKEND_EXECUTION_PLAN_DESCRIPTOR,

            &execution_plan
        )
    );



    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            execution_plan,

            CUDNN_ATTR_EXECUTION_PLAN_ENGINE_CONFIG,

            CUDNN_TYPE_BACKEND_DESCRIPTOR,

            1,

            &engine_config
        )
    );



    CHECK_CUDNN(
        cudnnBackendFinalize(execution_plan)
    );


    printf("Execution plan ready\n");





    // =================================================
    // Variant Pack 将GPU中的数据绑定到计算图中 之前的计算图中的数据都是逻辑描述
    // 绑定真实 GPU pointer
    // =================================================


    cudnnBackendDescriptor_t variant_pack;


    CHECK_CUDNN(
        cudnnBackendCreateDescriptor(

            CUDNN_BACKEND_VARIANT_PACK_DESCRIPTOR,

            &variant_pack
        )
    );



    void *data_ptrs[3];

    data_ptrs[0] = d_input;
    data_ptrs[1] = d_filter;
    data_ptrs[2] = d_output;








    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            variant_pack,

            CUDNN_ATTR_VARIANT_PACK_DATA_POINTERS,

            CUDNN_TYPE_VOID_PTR,

            3,

            data_ptrs
        )
    );




    // 传递uid

    // 

    int64_t uids[3];


    uids[0] = x_uid;
    uids[1] = w_uid;
    uids[2] = y_uid;


    CHECK_CUDNN(
        cudnnBackendSetAttribute(

            variant_pack,

            CUDNN_ATTR_VARIANT_PACK_UNIQUE_IDS,

            CUDNN_TYPE_INT64,

            3,

            uids
        )
    );


    //通过uid形成tensor和实际数据的对应关系
    //uid 1 x_tensor : d_input
    //uid 2 w_tensor : d_filter
    //uid 3 y_tensor : d_output



    CHECK_CUDNN(
        cudnnBackendFinalize(variant_pack)
    );




    // =================================================
    // Execute
    // =================================================


    CHECK_CUDNN(
        cudnnBackendExecute(

            cudnn,

            execution_plan,

            variant_pack
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



    printf("\nOutput:\n");



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




    // =================================================
    // Destroy
    // =================================================


    cudnnBackendDestroyDescriptor(variant_pack);

    cudnnBackendDestroyDescriptor(execution_plan);

    cudnnBackendDestroyDescriptor(engine_config);

    cudnnBackendDestroyDescriptor(heuristic);

    cudnnBackendDestroyDescriptor(op_graph);

    cudnnBackendDestroyDescriptor(conv_op);

    cudnnBackendDestroyDescriptor(conv_desc);

    cudnnBackendDestroyDescriptor(x_tensor);

    cudnnBackendDestroyDescriptor(w_tensor);

    cudnnBackendDestroyDescriptor(y_tensor);



    cudaFree(d_input);
    cudaFree(d_filter);
    cudaFree(d_output);



    cudnnDestroy(cudnn);



    return 0;
}



        //         你的数据
        //           |
        //           |
        // +-------------------+
        // |  Tensor Descriptor |
        // |                   |
        // | X: 输入           |
        // | W: 卷积核         |
        // | Y: 输出           |
        // +-------------------+
        //           |
        //           |
        // +-------------------+
        // | Conv Descriptor   |
        // |                   |
        // | stride            |
        // | padding           |
        // | dilation          |
        // +-------------------+
        //           |
        //           |
        // +-------------------+
        // | Operation         |
        // |                   |
        // | Y = Conv(X,W)     |
        // +-------------------+
        //           |
        //           |
        // +-------------------+
        // | Operation Graph   |
        // |                   |
        // | 把计算连接起来     |
        // +-------------------+
        //           |
        //           |
        // +-------------------+
        // | Engine Heuristic  |
        // |                   |
        // | 选择最快kernel     |
        // | CUDA Core?        |
        // | Tensor Core?      |
        // +-------------------+
        //           |
        //           |
        // +-------------------+
        // | Execution Plan    |
        // |                   |
        // | 确定怎么执行       |
        // +-------------------+
        //           |
        //           |
        // +-------------------+
        // | Variant Pack      |
        // |                   |
        // | 绑定真实地址       |
        // | d_input           |
        // | d_filter          |
        // | d_output          |
        // +-------------------+
        //           |
        //           |
        //           ↓

        // cudnnBackendExecute()

        //           |
        //           ↓

        //      GPU执行卷积

        //           |
        //           ↓

        //       输出 Y






//                      x_tensor
//                  |
//                  |
//              conv_op
//                  |
// w_tensor ---> conv_op ---> y_tensor
//                  |
//              conv_desc


//                  |
//                  v


//              op_graph

//                  |
//                  v


//           engine_heuristic

//         +----------------+
//         | engine0        |
//         | engine1        |
//         | engine2        |
//         +----------------+

//                  |
//                  |
//                  v

//           engine_config


//                  |
//                  v

//           execution_plan

//                  |
//                  v

//              execute