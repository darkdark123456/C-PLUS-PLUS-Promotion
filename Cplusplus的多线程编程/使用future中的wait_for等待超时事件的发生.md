```c++
int waitForFuture(){
    std::future<int> future=std::async(std::launch::async,[]{
                std::this_thread::sleep_for(3s);
                return 9;
                    });

    std::cout<<"waitting.............."<<std::endl;
    std::future_status status;
    // 在这里至少有两次time_out事件出现
    do{
        status=future.wait_for(1s);
        switch(status){
            case std::future_status::deferred :
                    std::cout<<"deferred "<<std::endl;
                    break;
            case std::future_status::timeout :
                    std::cout<<"time out"<<std::endl;
                    break;
            case std::future_status::ready:
                    std::cout<<"ready "<<std::endl;
                    break;
            default:
                    break;
        }

    } while(status != std::future_status::ready);
    std::cout<<"数据已经准备好 "<<future.get()<<std::endl;
    return 1;
}

```

![image-20230317202039505](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20230317202039505.png)