通过promise对象来保存一个预期值，在将来的某个时刻我们可以通过future绑定到这个promise上来得到这个promise对象真正的执行结果,通俗的说，promise能够延后求值。

```c++
#include <iostream>
#include <future>
#include <thread>
#include <chrono>


void mythread(std::promise<int>& tmp,int calc){
    calc++;
    calc*=10;
    int res=calc;
    std::chrono::milliseconds dura(5000); // 5s
    std::this_thread::sleep_for(dura);
    tmp.set_value(res);---------------------------------------------------------|
}                                                                               |
     																			|
																				|
																				|
int main(){
    std::promise<int> prom; // 声明并定义一个prom 它没有保存值                       |
    std::thread t1(mythread,std::ref(prom),99); // 创建一个thread投入执行
    t1.join(); // 等待mythread执行完毕                                             ^
    std::future<int> ful=prom.get_future(); // 如果tmp.set_value(res)执行完，ful进入就绪状态
    std::cout<<ful.get()<<std::endl; // 通过ful获取到promise对象保存的值
    return 1;
}







```

