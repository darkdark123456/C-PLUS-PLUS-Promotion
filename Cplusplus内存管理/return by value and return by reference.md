### return by value and return by reference

##### 	1 函数交出去的是一个局部变量的引用或者地址

```cpp
// 返回一个局部变量的引用或者指针是危险的，不论在main函数中使用值接收还是引用接收
float& by_refer(){float pi=3.14f;return pi}

float* by_pointer(){float pi=3.14f; return &pi}


int main(int*arg,int** args){
    
    
    // 情况1 意图使用一个函数返回局部变量的引用初始化另一个变量
    float pi_copy=by_refer()//错误
        
   // 情况2 意图使用一个函数返回的局部变量的引用绑定到main函数中的一个引用    
   float& pii=by_refer();
    
    
    // 编译器返回一个局部变量的引用时，返回的只是引用的名字，无论你用什么接收，当你拿到别名的时候，这块别名的内存空间已经被释放掉了。试图将一个引用绑定到一个已经释放掉的空间，这是非法的。其中，返回局部变量的地址也是这个问题。本质上是局部变量是在栈上，函数执行完后会被自动回收内存。
    
    // 但当这块内存是在堆上分配时，返回值是有效的，堆上的变量的生命周期由用户决定.
    
    
    
    
    
    // 编译器认为你指向的这块内存没有什么实际意义，会将ptr=0
    // 这是编译器帮你做的优化，为了程序在运行时不崩溃
    // 实际上你的程序设计已经出错了
    float*ptr=by_pointer();
    
    
    
    return 1;
}











```

2 函数交出去的是一个值

```c++
float by_val() {float pi=3.14f;return pi;}

int main(int* arg,int** args){
    
    float pi=by_val();
    // 这是被允许的，编译器先拿到的是一个值而不是地址或者引用（这里仅以局部变量进行讨论
    
    
    // 和上面类似，将局部变量绑定在引用上，这是非法的
    float& pi=by_val();
    
}



```

3 两者的区别

|                | BY_VALUE                                           | BY_REFERENCE                                                 |
| :------------- | -------------------------------------------------- | :----------------------------------------------------------- |
| 返回寄存器的值 | 一个具体的变量值                                   | 一个地址或者别名（实际也是地址）                             |
| 是否发生拷贝？ | 有                                                 | 引用不做拷贝，main函数中左值引用接收局部变量的左值引用时没有任何实际含义，实际上返回局部变量的指针时编译器也会将main中左值ptr置为null。 |
| 有结果对象吗？ | 有，归调用方所有                                   | 无。编译器认为你引用/指向了一块已经释放的内存，这个指针/引用没有实际意义。指向该对象的指针或者引用还在，但是具体内存空间已经不在了。就像房间号还在，但是房子已经被拆除了，不能存储任何物品了，这个房间号也就没什么用了。 |
| 局部变量       | 无所谓，我已经拿到你的返回值进行自己变量的初始化。 | 地址悬空，引用悬空。                                         |
| 语义           | 值语义：把"值"交出去。                             | 引用/指针语义：把"对象的身份"交出去                          |
|                |                                                    |                                                              |

###### 	4 如何返回一个局部变量的引用或者指针

```c++
// 在程序设计中，我们如果想计算得来一个变量或者引用的值，一般是先在当前函数中进行定义，再将其设计为计算函数的参数，使用该变量的地址或者引用作为形参就可以了。
void calculate_by_pointer(const float *q,float* ptr){
		
	(*ptr)=(*q)*2;
}

void calculate_by_reference(const float& q,float& p){
    
    p=q*2;
}


int main(int* arg,int** args){
	float pi=3.14f;

    //欲获得局部变量pi_2的值
    float pi_2=0; //想要计算2pi
	calculate_by_pointer(pi,&pi_2); 
	calculate_by_reference(pi,pi_2);//比返回引用或者指针更加安全
	return 1;
} // 所有的局部变量被自动回收

// 但是你实在想在函数返回一个局部变量的引用或者地址，请将其局部变量开辟在堆空间上，并记得释放分配的内存空间


//一个计算2Π的程序
 float* return_local_var_by_heap(){ 		    
     float* pi=(float*)malloc(sizeof(float));
     *pi=3.14f;
     (*pi)*=2;
     return pi;//那么我为什么不直接在调用此函数中的函数中直接malloc pi，并作为函数参数进行传递，而是在新函数中malloc冒着遗忘free的风险呢？
 }

//在此函数中调用
void function(){
    
	float* ptr=return_local_var_by_heap();//设计者自己知道返回值ptr是指向堆区的内存的。
    
    //拿到2Π，*ptr
    
    
    free(ptr); //如果调用者不知情，那么就容易遗忘
}





// 计算2Π
void calculate_2pi(float* pi,float*pi_2){
    
    //只注重计算逻辑，而不用关心内存
    (*pi_2)=(*pi)*2;
}



// 更好的设计规则
void function(){
    
    // 既然程序中明确要计算的变量，且希望手动管理内存
    // 使用下面的程序
    
    
    float* pi=(float*)malloc(sizeof(float)); 
    float* pi_2=(float*)malloc(sizeof(float));
    (*pi)=3.14f;
    
    
    calculate(pi,pi_2);//调用者直接完成计算模块
    
    
    
    free(pi);  //分配释放一目了然.
    free(pi_2);
}
```

