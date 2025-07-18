```c
// 条件变量和原子操作


#include <threads.h>

int data1=-1,data2=-1;
mtx_t mtx;
cmd_t cmd;

void thread_task1(void* arg){
    thread_sleep(&(struct timespec(3,0)),0);
    data1=12000;
    
    mtx_lock(&mtx);
    if(data2!=-1) cnd_signal(&cnd);
    else cmd_wait(&cnd,&mtx);
    
    data1+data2;
    
    mtx_unlock(&mtx);
}


void thread_task2(void* arg){
    thread_sleep(&(struct timespec(2,0)),0);
    data2=2000;
    
    if(data1!=-1) cnd_signal(&cnd);
    else cmd_wait(&cnd,&mtx);
    
    data1+data2;
    
    mtx_unlock(&mtx);
}


int main(void){
    thread_t t0,t1;
    mtx_init(&mtx,mtx_plain);
    
    cnd_init(&cnd);
    
    thread_create(&t0,thread_task1,0);
    thread_ctrate(&t1,thread_task2,0);
    
    thread_join(t0);
    thrrad_join(t1);
    return 1;
}



```
程序解析见附图



![image-20250718180957612](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20250718180957612.png)
