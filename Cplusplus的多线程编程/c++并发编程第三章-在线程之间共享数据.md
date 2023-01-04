***第三章 在线程之间共享数据***

 ***本章内容***

​	***在线程之间共享数据的问题***

​    ***利用互斥量保护共享数据***

​    ***利用其他工具保护共享数据***

***3.1 线程间共享数据存在的问题***

​	   ***3.1.1 数据竞争出现的问题***

```c++
  /*假设有一个线程thd1正在删除B元素，一个线程thd2正在从左至右读取链表的元素，那么线程thd2很可能读取到的不是正确的数据*/
    
    
     ①              
    -----------------              -—---------------              --------------------
-->|  .  | data |  .-|----------->|  .  | data |  .-|----------->| .   | data |  . ---|---->
    ---|-------------              --|--------------              --|-----------------
       |          ^                  |            ^                 |              ^
       |          |                  |            |                 |              |
  <-----          --------------------            -------------------              ---------
        
           A                             N                                 B
    
         
                                ----------------pointer-------------
    ②                           |                                   \>
     -----------------          |   -—---------------             --------------------
-->|  .  | data |  .-|----------  |  .  | data |  .-|----------->| .   | data |  . ---|---->
    ---|-------------              --|--------------              --|-----------------
       |          ^                  |            ^                 |              ^
       |          |                  |            |                 |              |
  <-----          --------------------            -------------------              ---------
        
           A                             N                                 B   
    
         
         
         
    
                                 ------------------------------------
   ③                            |                                   \>
     -----------------          |   -—---------------             --------------------
-->|  .  | data |  .-|----------  |  .  | data |  .-|----------->| .   | data |  . ---|---->
    ---|-------------              --|--------------              --|-----------------
       |    ^     ^                  |                              |              ^
       |    |     |                  |                              |              |
  <-----    |     --------------------                              |              ---------
            |                                                       |
           A|                            N                          |       B      
             -----------------------------------------------------
         
         
         
         
                                ------------------------------------
   ④                            |                                   \>
     -----------------          |                                 --------------------
-->|  .  | data |  .-|----------                                 | .   | data |  . ---|---->
    ---|-------------                                            --|-----------------
       |    ^                                                      |              ^
       |    |                                                      |              |
  <-----    |                                                      |              ----------
            |                                                      |
           A|                                                      |       B      
             -----------------------------------------------------
                 
   
         
         
  考虑在②出现的一种崩溃程序的状况：thd在进行删除操作时的pointer还没有形成指向(thd1正在修改指向),thd2恰好访问完A元素正想使用pointer来访问下一个元素，此时由于pointer没有明确的指向，程序就会崩溃！归根原因是破坏了不变量pointer，也可以认为，thd1和thd2都在同时竞争pointer出现的错误(条件竞争),其中条件为pointer；
         
```

​       ***3.1.2 条件竞争***

```c++
/*
条件竞争： 你得到什么样的数据取决于thread的相对执行次序
良性的条件竞争我们不感兴趣，恶行的条件竞争c++称作为数据竞争，我们主要是解决这一头疼的问题：

诱发数据竞争的主要场景是：要完成一项操作，可能需要改动两份或者多份不同的数据，列如修改上述两个连接pointer，
因为要操作两个单独的指针，在单独的指令下(可以粗略理解为cpu的执行速度很慢)，当其中一份数据pointer正在被线程修改，另一个线程却不期而访，这就会出现问题； 如果改动是由连读不间断的cpu指令完成的(cpu的执行速度非常快)，就不太可能出现上述的问题，
因为pointer的修改在一瞬间就修改完成了；
*/
```

***3.2用互斥量保护数据***

 	  ***3.2.1  对访问的共享数据进行mutex***

```c++
/*如果我们能将在访问数据结构时进行标记，令各个线程在访问上互斥，只要线程访问被加锁的互斥量就必须等待，这并非异想天开，运用名为互斥，mutual exclusion的同步原语synchronization primitive就能达到我们的想要的效果。访问一个数据结构前，先锁住互斥，访问结束后，再解锁互斥，这就确保了全部线程所见到的共享数据是自洽的；在前面的RAII手法中，在构造函数中判断thread是否可汇合，再在析构函数中让thread汇合；在这里，在构造函数中对互斥量加锁，在析构时对互斥量解锁，c++使用了标准的类模板std::lock_guard<>(),但是互斥量有一个致命的问题，死锁！我们稍后会讨论到。*/



用互斥量保护链表

#include  <list>
#include  <thread>
#include  <mutex>
#include  <algorithm>
    
std::list<int>  List;
std::mutex mtx;


void addToList(int value){
    std::lock_guard<std::mutex> guard(mtx);①<-----对List加锁
    List.push_back(value);
自动对List解锁------->②}


Iterator listContain(int value){
    std::lock_guard<> guard(mtx);①<----对List加锁
    return std::find(List.begin(),List.end(),value);
自动对List解锁------->②}



```

​		***3.2.2 在设置互斥量和共享量的类时通过摒弃不良的接口来保护共享数据(靠程序员自己了！)***

```c++
/*

我们通常将互斥量和共享量放在一个类里实现，这样即提高了共享量的安全访问级别又实现了互斥的进行访问，何乐而不为？
***********************************但互斥量不是万能的！***************************************
这其中可能存在非常严重的问题，假设有这么一种情况，成员函数返回了指针或者引用(引用本身也是一种指针)，指向了受保护或者私有的共享变量，那么即使成员函数全部按良好有序的方式锁定互斥，仍然无济于事，因为这种保护已经被打破，外来者可以轻松的使用该指针修改指向的共享数据，所以在设计类的接口上要谨慎设计接口，保证不出现上述的情况。

考虑另一种情况，如果成员函数mf在内部调用了别的函数func(例如定义在全局的函数func),而这些func不仅不受我们掌控，而且需要mf传递给func指向共享变量的指针，func可能将这个指针暂存起来，等到共享变量脱离了互斥再通过指针访问这个共享量，也是同样危险的！


如果结解决了上述的游离指针的问题，那么共享数据既可以被互斥访问又不会被修改，便安然无恙。

*/


class    Data{
    protected:
    	int a;
    	std::string s;
    public:
    	void doSomething();
};


class DataWrapper{
    private:
    	Data data;
    	std::mutex mtx;
    public:
    	template<typename Function>
    	void processData(Function func){
            std::thread_guard<std::mutex> lock(mtx);
            func(data);
        }
};

/*全局*/
data* unprotected;

void maliciousFunction(data& protectedData){
    unprotected=&protectedData;
}

/*全局*/
DataWrapper x;


void function(){
    x.processData(maliciousFunction);
    unprotected->doSomething();
}




/*
观察下图容易得到：使用者提供的函数func修改了受保护的共享变量data，这是不合理的，既然data受保护，那么不应该允许func外来的接口修改它。c++线程库对这种错误无能为力，只有靠我们自己来解决，从乐观的角度看，只要遵循以下指针引用的规则即可:
	***************不得向锁所在的作用域之外传递指针或者引用指向受保护的共享数据，无论是通过函数返回值将它们保存到对外可见的内存，还是将它们作为参数传递给使用提供的外部函数*************************




*/
```

![](C:\Users\Administrator\Desktop\3.2.2.png)

​		***3.2.3	发现接口访问共享数据时的固有的条件竞争***

```c++
/*
有些固有的条件竞争我们无法避免，例如前面的list例子，我们必须禁止三个节点的并发访问，正在删除的节点N和它的左右邻居AB，但考虑到删除行为是一个整体的操作，所以我们使用一个互斥mtx将整个list保护起来，但有时将整个共享量单独用一个互斥量保存起来也会带来麻烦,详情如下，c++总是这样，在引入了一个新问题后又带来了另一个或者多个新问题，而且你可能无法将现有的解决容器A的方法同样的应用到其他容器中
*/



template<typename T,typename Container=std::deque<T>>
class Stack{
  public:
    explict Stack(const Container&);
    explict Stack(Container&&=Container());
    template<typename Alloc> explict Stack(const Alloc&);
    template<typename Alloc> explict Stack(const Container&,const Alloc&);
    template<typename Alloc> explict Stack(Container&& ,const Alloc&);
    template<typename Alloc> explict Stack(Stack&&,const Alloc&);
  public:
    bool empty() const;
    size_t size() const;
    T& top();
    const T& top() const;
    void push(const T&);
    void push(T&&);
    void pop();
    void swap(Stack&& );
    };



/*
在单线程的环境下访问一个stack是没有问题的，如果使用多线程，这里的empty()和size()返回的未必是正确的值
考虑如下代码：在空栈上使用top()会导致未定义行为，上面的代码已经做好了数据防备，对于单线程而言，它安全又符合我们的预期，可是一旦涉及到数据共享，这一连串调用便不再安全，考虑thd1和thd2的这么一种情况，它们都想访问栈顶元素2,thd1正运行在①②之间，thd2刚刚运行完③，那么thd2拿到的是正确的元素2，而thd1拿到的却是元素1，如果将这种情况推广到n元素的stack(元素足够多)，可能会出现这么一种情况，如果希望thd1和thd2都想使用栈顶元素，thd2拿到的是正确的元素v，thd1拿到的是v的后一个元素u;更一般的特殊情况，stack中只有一个元素，thd2拿走后，thd1拿到的直接是一个不在stack内不合法的元素！如果你想这么设计，在top的接口里的内部细节加上Stack不是空才top也就是说在②处隐式的判断了一次栈是否为空，也许会解决thd1上述读取到非法数据的问题，但是你无法预知线程的执行次序，下面的另一种可能执行的次序你即使在top接口的内部细节里加上判栈空这一条件，也无济于事，因为下面的次序直接导致了元素不合法的丢弃！
*/

void function(){
    Stack<int> stack;
    if(!stack.empty()){①<-----------------
        const int value=stack.top();②<----------------
        stack.pop();③<------------
        doSomething(value);
    }
}


                   |       |
                   |-------|<-------top
                   |   2   |
                   |-------|<-------top/*thd2运行完stack后的top指针*/
                   |   1   |
                    -------

                       
                       
                       
                       
 ***************另一种可能的执行次序 导致元素1直接被丢弃了****************************
                       
 ------------------------------------------------------------------------------
          thread2                                        thread1
 -------------------------------------------------------------------------------
      if(!stack.empty())                                 
 -------------------------------------------------------------------------------
                                                      if(!stack.empty())
 -------------------------------------------------------------------------------
      const int value=stack.top()              
 -------------------------------------------------------------------------------
  													  const int value=stack.top()  
 -------------------------------------------------------------------------------
      stack.pop()
 -------------------------------------------------------------------------------
 	  doSomething(value)									stack.pop()				
 -------------------------------------------------------------------------------
                                                            doSomething(value)
 -------------------------------------------------------------------------------
              
                       
                       
 /*上面的数据竞争是固有的,我们无法完全消除，只能根据不同情况来选择不同的策略*/
                                                          
  方法1
        借用一个外部变量result接收容器弹出的元素，并将pop的函数参数用引用接收result,也就是说在pop执行后用参数引用result得到弹出的元素；
        int result;
        stack.pop(result);
在很多情况下是可行的，例如int,double,float这些基本数据类型都没有什么问题考虑如下情况，Person是自定义类有很多成员变量，height，sex，age，hair......
        Person person;
        stack.pop(person);
这就有两个小小的问题在pop的函数中要将栈顶元素（一个Person类型）赋值给person，如果你忘了operator=，编译器会报出错误，还有一个小问题就是pop时要构造一个Person，可能还要给构造函数传递参数，花费的时间的耗费的资源都是巨大的，这与c++的设计初衷背道而驰！
  
  方法2   提供不抛出异常的拷贝构造函数
    假设pop按值返回 int result=stack.pop(),对result初始化时会调用int的构造函数，如果限制其类型为int，string，内置类型，这没什么问题，但栈的元素类型却受到很大的限制；
   
   
  方法3   使用smart pointer指向被弹出的元素，
    pointer可以被自由的复制和赋值（同类型的pointer）并且不会抛出异常，而且smart pointer会自动管理内存，避免内存泄漏；
    
  方法4   进行方法组合 23 13 我感觉只使用方法3就很好了
    
    
    
/*线程安全的栈容器的类的定义 将上面的方法应用到栈中*/
#include  <exception>
#include  <memory>
 
class EmptyStackException: std::exception{w
    /*重写父类的what方法，throw 这个异常对象时，会自动调用what方法*/
    const char* what() const noexcept{
        std::cout<<"Empty Stack Exception "<<std::endl;
    }
    
};
    

template<typename T>
class ThreadSafeStack{
  public:
    ThreadSafeStack();
    ThreadSafeStack(const ThreadSafeStack& );
    ThreadSafeStack& operator=(const ThreadSafeStack&)=delete;
    void push(T value);
    std::share_ptr<T> pop() throw EmptyStackException;③<------方法三且在空栈上弹出元素会抛出异常
    void pop(T& value) throw EmptyStackException;②<----方法①且在空栈上弹出元素会抛出异常
    bool empty() const;
};
		



/*更加详细的安全栈的定义及内部细节的实现*/

/*定义一个空栈的异常*/
class EmptyStackExcception : std::exception{
    const char* what() const noexcept{
        std::cout<<"Empty Stack Exception"<<std::endl;
    }
}                                                  

/*安全栈的定义*/
#include  <exception>
#inlcude  <memory>
#inlcude  <mutex>
#inlcude  <stack>


template<typename T>
class ThreadSafeStack{
    private:
    	std::mutex mtx;
    	std::stack<T> data;
    public:
    	ThreadSafeStack(){}
    	
    	ThreadSafeStack(const ThreadSafeStack& rhs){
            std::lock_guard<std::mutex> lock(mtx);
            data=rhs.data;
        }
    	
    	Thread& operator=(const ThreadSafeStack& rhs)=delete;
    	
    	void push(int value){
            std::lock_guard<std::mutex> lock(mtx);
            data.push(std::move(value));/*会通过移动元素来进行push*/
        }
    
    	std::shared_ptr<T> pop(){
            std::lock_guard<std::mutex> lock(mtx);
            if(data.empty()){ throw EmptyStackException();}
            shared_ptr<T> ret(std::shared_ptr<T>(data.top()));
            data.pop();
            return ret;
        }
    	
       void pop(int& value){
           std::lock_guard<std::mutex> lock(mtx);
           if(data.empty()) {throw EmptyStackException();}
           value=data.top();
           data.pop();
       }
    	
       bool empty(){
           std::lock_guard<std::mutex> lock(mtx);
           return data.empty();
       }
    
    
}
                       
```

******



***3.2.4死锁 问题解决和方法***

```c++
我们为了保证资源在访问时不会出现异常，我们通常会对资源加锁，可是会出现一些以外的情况，如果在对资源的加锁的细粒度太小，例如对前面的链表只对当前删除的节点加锁，会出现问题。如果加锁的细粒度太大，例如对所有的资源都加锁，就会对系统的性能产生一定影响；
线程在互斥上争抢锁可能会出现死锁的问题，假设有这么一种情况，有两个线程，分别需要锁住两个互斥才能正常工作，现在第一个线程锁住了第一个互斥想要锁住第二个互斥才能工作，第二个线程锁住了第二个互斥想要锁住第一个互斥才能工作，现在它们都不想释放自身锁住的互斥，都在苦苦等待对方释放互斥量，这就导致了死锁出现的问题，防范死锁的建议是我们始终按相同的顺序对资源加锁，若我们总是先锁住第一个互斥量再锁住第二个互斥量就不会出现上述的死锁问题；可是这种建议不是所有情况都适用，考虑下面的情况：

class Info{
  private:
    size_t height;
    size_t age;
    string name; 
};
    

class Employee{
  private:
    Info info;
  	std::mutex mtx;
  public:
	friend void swap(Employee& lhs,Employee& rhs){
 /*再交换两个Employee对象时，两个对象被先后加锁*/
        std::lock(lhs.mtx,rhs.mtx);①<------可能出现死锁
        std::lock_guard<std::mutex> lock_a(lhs.m,std::adopt_lock);<-----指明已经加锁
        std::lock_guard<std::mutex> lock_b(rhs.m,std::adopt_lock);<-----指明已经加锁
        swap(lhs.info,rhs.info);
 /*交换完毕，被解锁*/
    }    
};

上述的swap即使被先后加锁仍会出现问题，交换两个相同的对象时在加锁的过程中会出现死锁！
    Employee e1；
    swap(e1,e1);
解决方法：先判断是否是自我赋值
class Employee{
  private:
    Info info;
  	std::mutex mtx;
  public:
	friend void swap(Employee& lhs,Employee& rhs){
 /*再交换两个Employee对象时，两个对象被先后加锁*/
        if(&rhs==&lhs){return;}/*如果当前是自赋值，直接返回，还交换个什么玩意？*/
        std::lock(lhs.mtx,rhs.mtx);①<------可能出现死锁
        std::lock_guard<std::mutex> lock_a(lhs.m,std::adopt_lock);<-----指明已经加锁
        std::lock_guard<std::mutex> lock_b(rhs.m,std::adopt_lock);<-----指明已经加锁
        swap(lhs.info,rhs.info);
 /*交换完毕，被解锁*/
    }    
};   


/*更简便的方法使用scoped_lock来代替*/
class Employee{
  private:
    Info info;
  	std::mutex mtx;
  public:
	friend void swap(Employee& lhs,Employee& rhs){
 /*再交换两个Employee对象时，两个对象被先后加锁*/
        if(&rhs==&lhs){return;}/*如果当前是自赋值，直接返回，还交换个什么玩意？*/
		std::scoped_lock<> lock(lhs.mtx,rhs.mtx);		
        swap(lhs.info,rhs.info);
 /*交换完毕，被解锁*/
    }    
};   



```

***3.2.5防范死锁的补充准则***

```c++
1 避免嵌套锁
2 一旦持锁，必须避免调用由用户提供的程序接口
3 按照固定顺序加锁
4 按层级加锁 
5 将准则推广到锁之外
    使用std::unique_lock<> 灵活加锁
    在不同的作用域之间转移互斥的归属权
    按合适的细粒度加锁

下面依次解释这些情况：
1 避免嵌套锁
    如果有一个线程已经持有了一个锁，那么就不要试图再获得第二个锁了，若能遵守这一点，每个线程持有唯一的锁，仅锁的使用本身不用引起任何死锁，但是，如果线程如果需要获得两个锁才能保证工作，那么我们应该采用std::lock()来获取全部锁，通过让线程一次性获取全部锁让其正常执行来避免死锁的发生。
2 一旦持锁，必须谨慎或避免由用户提供的程序接口
    这个我们在前面已经讨论过了，如果使用用户提供的程序接口很可能会修改掉受保护的互斥变量，这是致命的
3 按固定顺序加锁
    3.2.4的防范死锁的建议就是按固定顺序加锁，假设有两个个互斥量m1，m2，线程想对m2加锁的必要条件是当且仅当对m1已经加了锁，这样能防止线程循环等待产生的死锁。
4 按层级加锁 
    类似于按优先级，我们划分优先级，明确每个互斥量位于哪个优先级，***如果线程已经对低优先级的互斥量加锁，就不能对高优先级的互斥量加锁***。具体的做法是将层级的编号赋予对应层级应用程序上的互斥并记录各线程分别锁定了哪些互斥。遗憾的是标准库并未能提供，故我们需要自己编写优先级互斥锁，下面是一种实现方式。
    
 namespace  Meng{
    hierarchical_mutex high_level_mutex(10000); // 最高的优先级
	hierarchical_mutex low_level_mutex(5000); // 最低的优先级
	hierarchical_mutex other_mutex(6000); //中间的优先级
    
    int doLowLevelStuff();
    int lowLevelFunc(){
        std::lock_guard<hierarchical_mutex> lock(low_level_mutex);//使用低级加锁
        return doLowLevelStuff();
    }
    
    void highLevelStuff(int parm);
    void highLevelFunc(){
        std::lock_guard<hierarch>
    }
    
    

}
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
 
    
    
    
    
    
 
    
    
    
    
```

