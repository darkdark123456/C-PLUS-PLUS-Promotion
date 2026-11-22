// #include "relux.h"




// OrtStatusPtr KernelRelu::ComputeV2(OrtKernelContext* context){

//     Ort::KernelContext ctx (context);

//     auto input_tensor=ctx.GetInput(0);

//     const float* input_data=input_tensor.GetTensorData<float>();

//     auto shape=input_tensor.GetTensorTypeAndShapeInfo();
//     std::vector<int64_t> dim=shape.GetShape();
//     size_t elem_count =shape.GetElementCount();

//     auto output_tensor=ctx.GetOutput(0,dim.data(),dim.size());

//     float* output_data=output_tensor.GetTensorMutableData<float>();


//     for(size_t i = 0;i<elem_count;i++){

//         output_data[i]=(input_data[i] > 0 ) ? input_data[i] :0.0f;
//     }

//     return nullptr;
// }
#include "relux.h"

void KernelRelu::Compute(OrtKernelContext* context)
{
    Ort::KernelContext ctx(context);

    auto input = ctx.GetInput(0);
    const float* in = input.GetTensorData<float>();

    auto info = input.GetTensorTypeAndShapeInfo();
    auto shape = info.GetShape();
    size_t count = info.GetElementCount();

    auto output = ctx.GetOutput(0, shape.data(), shape.size());
    float* out = output.GetTensorMutableData<float>();

    for (size_t i = 0; i < count; i++) {
        out[i] = in[i] > 0 ? in[i] : 0.f;
    }
}
