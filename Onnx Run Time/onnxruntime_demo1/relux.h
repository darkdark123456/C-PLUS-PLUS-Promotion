#ifndef RELUX_H
#define RELUX_H

// #include <onnxruntime_cxx_api.h>

// class KernelRelu{
//     private:
//         const OrtApi& api_;
//         const OrtKernelInfo* info_;
//     public:
//         explicit KernelRelu(const OrtApi& api,const OrtKernelInfo* info) : api_(api),info_(info){}
//     public:
//         OrtStatusPtr ComputeV2(OrtKernelContext* context);
// };



// class ReluOption : public Ort::CustomOpBase<ReluOption,KernelRelu,true>{
//     public:

//         OrtStatusPtr CreateKernelV2(const OrtApi&api,const OrtKernelInfo* info,void** op_kernel) const {

//             *op_kernel= new KernelRelu(api,info);
//             return nullptr;
//         }

//         const char* GetName() const {return "ReluX";}

//         const char* GetExecutionProviderType() const {

//             return "CPUExecutionProvider";
//         }


//         size_t GetInputTypeCount() const {

//             return 1;
//         }

//         ONNXTensorElementDataType
//         GetInputType(size_t) const {

//             return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
//         }


//         size_t GetOutputTypeCount() const {

//             return 1;
//         }

//         ONNXTensorElementDataType
//         GetOutputType(size_t) const {

//             return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
//         }
// };










#pragma once
#include <onnxruntime_cxx_api.h>

class KernelRelu {
public:
    KernelRelu(const OrtApi& api, const OrtKernelInfo* info)
        : api_(api), info_(info) {}

    void Compute(OrtKernelContext* context);

private:
    const OrtApi& api_;
    const OrtKernelInfo* info_;
};

class ReluOption : public Ort::CustomOpBase<ReluOption, KernelRelu,false> {
public:
    void* CreateKernel(const OrtApi& api, const OrtKernelInfo* info) const {
        return new KernelRelu(api, info);
    }

    const char* GetName() const { return "ReluX"; }

    const char* GetExecutionProviderType() const {
        return "CPUExecutionProvider";
    }

    size_t GetInputTypeCount() const { return 1; }

    ONNXTensorElementDataType GetInputType(size_t) const {
        return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
    }

    size_t GetOutputTypeCount() const { return 1; }

    ONNXTensorElementDataType GetOutputType(size_t) const {
        return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
    }
};













#endif // RELUX_H
