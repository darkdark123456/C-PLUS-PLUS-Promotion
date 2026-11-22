// #include "relux.h"

// static ReluOption relu_opt;

// extern "C"{


// OrtStatus* ORT_API_CALL RegisterCustomOps(OrtSessionOptions* options,const OrtApiBase* api){


//     const OrtApi* ort_api =api->GetApi(ORT_API_VERSION);

//     OrtStatus* status = nullptr;
//     OrtCustomOpDomain* domain =nullptr;

//     // 创建域名成功返回nullptr111
//     status=ort_api->CreateCustomOpDomain("custom",&domain);

//     // 失败返回
//     if(status) {

//         return status;
//     }

//     ;

//     // 自定义操作添加域名
//     status=ort_api->CustomOpDomain_Add(domain,&relu_opt);

//     // 失败返回
//     if(status){

//         ort_api->ReleaseCustomOpDomain(domain);
//     }


//     // 将域添加到session会话中
//     status=ort_api->AddCustomOpDomain(options,domain);

//     // 域已完成使命，释放当前域
//     ort_api->ReleaseCustomOpDomain(domain);


//     return status;
// }

// }



#include "relux.h"

static ReluOption relu_op;

extern "C" __declspec(dllexport) OrtStatus* ORT_API_CALL RegisterCustomOps(OrtSessionOptions* options, const OrtApiBase* api)
{
    const OrtApi* ort = api->GetApi(ORT_API_VERSION);

    OrtCustomOpDomain* domain = nullptr;
    OrtStatus* status = ort->CreateCustomOpDomain("custom", &domain);
    if (status) return status;

    status = ort->CustomOpDomain_Add(domain, &relu_op);
    if (status) {
        ort->ReleaseCustomOpDomain(domain);
        return status;
    }

    status = ort->AddCustomOpDomain(options, domain);
    ort->ReleaseCustomOpDomain(domain);

    return status;
}
