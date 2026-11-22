#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <onnxruntime_cxx_api.h>
#include <QVector>
#include <qmath.h>
#include <QString>
#include <opencv2/opencv.hpp>
#include <QFileInfo>
#include <cuda_runtime.h>


/**
由于硬件资源额条件限制，这里只用了单卡推理
*/

void func_get_image_data(QVector<float>& input_tensor){
    cv::Mat source_image=cv::imread("E:/dingjijizhang/onnx_pro/archive/test_set/test_set/dogs/dog.4012.jpg");
    if(source_image.empty()){

        return;
    }

    cv::resize(source_image,source_image,cv::Size(224,224));
    cv::cvtColor(source_image,source_image,cv::COLOR_BGR2RGB);

    source_image.convertTo(source_image,CV_32FC3,1.0/255.0);

    cv::Mat blob = cv::dnn::blobFromImage(source_image,1.0,cv::Size(224,224),cv::Scalar(),false,false);

    input_tensor.resize(blob.total());
    memcpy(input_tensor.data(),blob.ptr<float>(),blob.total()*sizeof(float));
}


void onnx_run_time_func(void){

    // 预处理
    cv::Mat source_image=cv::imread("E:/dingjijizhang/onnx_pro/archive/test_set/test_set/dogs/dog.4012.jpg");
    if(source_image.empty()){

        return;
    }

    cv::resize(source_image,source_image,cv::Size(224,224));
    cv::cvtColor(source_image,source_image,cv::COLOR_BGR2RGB);

    source_image.convertTo(source_image,CV_32FC3,1.0/255.0);

    cv::Mat blob = cv::dnn::blobFromImage(source_image,1.0,cv::Size(224,224),cv::Scalar(),false,false);

    QVector<float> input_tensor(blob.total());

    memcpy(input_tensor.data(),blob.ptr<float>(),blob.total()*sizeof(float));


    // 创建session
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING,"QT");
    Ort::SessionOptions session_option;
    try {
        OrtCUDAProviderOptions cuda_options{};

        // 指定显卡
        cuda_options.device_id=0;

        // 设置GPU显存上限
        cuda_options.gpu_mem_limit=4ULL*1024*1024*1024;

        // 设置GPU显存扩充策略
        cuda_options.arena_extend_strategy=0;

        // 设置最优卷积
        cuda_options.cudnn_conv_algo_search=OrtCudnnConvAlgoSearchExhaustive;

        // 添加CUDA
        session_option.AppendExecutionProvider_CUDA(cuda_options);

        // 算子内并行
        session_option.SetIntraOpNumThreads(2);

        // 算子内并行
        session_option.SetInterOpNumThreads(2);

        // 开启图优化
        session_option.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // 设置优化后的onnx模型保存路径
        QString save_onnx_path="optimized_model.onnx";
        session_option.SetOptimizedModelFilePath(save_onnx_path.toStdWString().c_str());

        // 开启cpu内存池
        session_option.EnableCpuMemArena();

        qDebug()<<"启动GPU推理\n";
    }
    catch (const Ort::Exception& exception) {

        qDebug()<<exception.what()<<"\n";
    }
    catch(const std::exception& exception ){
        qDebug()<<exception.what()<<"\n";
    }


    QString onnx_file_path=QString("E:/dingjijizhang/onnx_pro/onnx/cat_dog_model_sim.onnx");

    #ifdef _WIN32
        std::wstring wpath = QFileInfo(onnx_file_path).absoluteFilePath().toStdWString();

        try {
            Ort::Session session(env, wpath.c_str(), session_option);
            // 构造输入Tensor
            std::vector<int64_t> shape ={1,3,224,224};
            Ort::MemoryInfo memory_info=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);

            Ort::Value inputTensor=Ort::Value::CreateTensor<float>(memory_info,input_tensor.data(),input_tensor.size(),shape.data(),shape.size());

            // 执行推理
            Ort::AllocatorWithDefaultOptions allocator;
            Ort::AllocatedStringPtr input_name_ptr=session.GetInputNameAllocated(0,allocator);
            Ort::AllocatedStringPtr output_name_ptr=session.GetOutputNameAllocated(0,allocator);

            const char* input_names[]={input_name_ptr.get()};
            const char* output_names[]={output_name_ptr.get()};

            auto output_tensors=session.Run(
                Ort::RunOptions{nullptr},
                input_names,
                &inputTensor,
                1,
                output_names,
                1
                );


            float* output = output_tensors[0].GetTensorMutableData<float>();
            size_t output_size = output_tensors[0].GetTensorTypeAndShapeInfo().GetElementCount();

            QVector<float> expected(output_size,0.0f);

            bool ok= true;
            const float dert=1e-5;
            float conf=output[0];

            qDebug()<<"狗狗的置信度: "<<conf<<"\n";
            if(conf > 0.5){
                qDebug()<<"当前是狗\n";
            }else{
                qDebug()<<"当前是猫\n";
            }

            qDebug()<<(ok ? "验证通过" : "验证失败");
        }
        catch (const Ort::Exception& exception) {

            qDebug()<<exception.what()<<"\n";
        }
        catch(const std::exception& exception ){
            qDebug()<<exception.what()<<"\n";
        }

    #else
        std::string spath = model_path.toStdString();
        Ort::Session session(env, spath.c_str(), session_options);
    #endif

}


void func_thread_test(int thread_id){

    qDebug()<<"当前执行的线程: "<<thread_id<<"\n";
}


void func_thread(void){

    std::vector<std::thread> thread_vector;
    for(int i=0;i<4;++i){
        thread_vector.emplace_back(func_thread_test,i);
    }

    for(std::thread & thd : thread_vector){

        thd.join();
    }
}



void onnx_run_time_tensor_rt(void){
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING,"tensor_rt_test");

    Ort::SessionOptions  session_opt;

    session_opt.SetLogSeverityLevel(0);

    OrtTensorRTProviderOptions trt_options{};

    // 指定显卡
    trt_options.device_id=0;

    // 指定缓存上限
    trt_options.trt_max_workspace_size=4ULL*1024*1024*1024;

    // 启用fp16推理
    trt_options.trt_fp16_enable=1;

    // 禁用int8推理
    trt_options.trt_int8_enable=0;

    // 启用tensor rt缓存
    trt_options.trt_engine_cache_enable=1;
    trt_options.trt_engine_cache_path="E:/dingjijizhang/onnx_pro/cache";

    // 禁用dla
    trt_options.trt_dla_enable=0;


    try {

        // 创建tensor rt session
        session_opt.AppendExecutionProvider_TensorRT(trt_options);

        QString onnx_file_path=QString("E:/dingjijizhang/onnx_pro/onnx/cat_dog_model_sim.onnx");

        std::wstring wpath = QFileInfo(onnx_file_path).absoluteFilePath().toStdWString();

        Ort::Session session(env,wpath.c_str(),session_opt);



        // 构造输入Tensor
        QVector<float> input_tensor;
        func_get_image_data(input_tensor);

        std::vector<int64_t> shape ={1,3,224,224};


        //gpu info
        Ort::MemoryInfo memory_info(
            "Cuda",
            OrtAllocatorType::OrtArenaAllocator,
            0,
            OrtMemTypeDefault
            );

        //gpu buffer
        float* gpu_input=nullptr;
        cudaMalloc(&gpu_input,input_tensor.size()*sizeof(float));

        cudaMemcpy(gpu_input,input_tensor.data(),input_tensor.size()*sizeof(float),cudaMemcpyHostToDevice);

        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memory_info,gpu_input,input_tensor.size(),shape.data(),shape.size());



        // 构造输入输出name

        Ort::AllocatorWithDefaultOptions allocator;
        Ort::AllocatedStringPtr input_name_ptr=session.GetInputNameAllocated(0,allocator);
        Ort::AllocatedStringPtr output_name_ptr=session.GetOutputNameAllocated(0,allocator);


        const char* input_names[] = {input_name_ptr.get()};
        const char* output_names[] ={output_name_ptr.get()};


        auto output_tensors=session.Run(
            Ort::RunOptions{nullptr},
            input_names,
            &inputTensor,
            1,
            output_names,
            1
            );


        float* output = output_tensors[0].GetTensorMutableData<float>();
        size_t output_size = output_tensors[0].GetTensorTypeAndShapeInfo().GetElementCount();


        bool ok= true;
        const float dert=1e-5;
        float conf=output[0];

        qDebug()<<"狗狗的置信度: "<<conf<<"\n";
        if(conf > 0.5){
            qDebug()<<"当前是狗\n";
        }else{
            qDebug()<<"当前是猫\n";
        }

        qDebug()<<(ok ? "验证通过" : "验证失败");

    } catch (const Ort::Exception& exception) {
        qDebug()<<exception.what();
    }
    catch(const std::exception& excep){
        qDebug()<<excep.what();
    }
}




void func_custsom_kernel(void ){




}





int main(int argc, char *argv[])
{



    auto providers= Ort::GetAvailableProviders();
    for(auto& provider : providers){
        qDebug()<<provider<<"\n";
    }



    //onnx_run_time_vertify();


    //onnx_run_time_func();

    //func_thread();

    onnx_run_time_tensor_rt();

    int i=0;

}
