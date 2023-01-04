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
        std::lock_guard<hierarchical_mutex> lock(low_level_mutex);//使用低级加锁  4<------------------------
        return doLowLevelStuff();
    }
    
    
    void highLevelStuff(int parm);
    
    void highLevelFunc(){
        std::lock_guard<hierarch_mutex> lock(high_level_mutex); 5<-------------------先锁住层级高的互斥 
        highLevelStuff(lowLevelFunc()); 6<-------------------------------在LowlevelFunc中再锁住层级低的互斥，这没有什么问题
    }
    
    
    void thread_a(){ 7<---------------------
        highLevelFunc();
    }
    
    void doOtherStuff();
    
    void otherStuff(){
        highLevelFunc(); 10<------------------------再试图锁住层级高的互斥，这不符合规则
        doOtherStuff();
    }
    
    void thread_b(){  8<--------------
        std::lock_guard<hierarchical_mutex> lock(other_mutex); 9<------------先锁住层级中间的互斥
        otherStuff();
    }

}
    

结论 层级互斥之间不可能发生死锁，因为互斥变量本身被强制限定了加锁次序；如果两个层级互斥量具有相同的层级编号，那么我们也无法一并持有它的锁，正如前面所说，层级枷锁要严格限定对互斥量的加锁次序。若将层级锁应用于前文交替前进的策略(高层级锁的信息传递给低层级锁的信息)，那么前一个互斥量的层级必须>后一个互斥量的层级，如5，6
  
```

```c++
/*简单的层级互斥*/
class HierarchicalMutex{
  private:
    std::mutex mtx;
    const unsigned long hierarchyValue;
    unsigned long previousHierarchyValue;
    static thread_local  unsigned long thisThreadHierarchyValue;
  
  public:
    explicit HierarchicalMutex(unsigned long value){
        hierarchyValue =value;
        previousHierarchyValue=0;
    }
  public:
    /*保证当前线程互斥的层级锁的层级编号*/
    void checkForHierarchyViolation(){
        if(thisThreadHierarchyvalue<=hierarchyValue){
            throw std::logic_error("mutex hierarchy violated!");
        }
    }
    
    void updateHierarchyValue(){
        previousHierarchyValue=thisThreadHierarchyValue;
        thisThreadHierarchyValue=hierarchyValue;
    }
    
    
    void lock(){
        checkForHierarchyValueViolation();
        mtx.lock();
        updateHierarchyValue();
    }
	
    bool try_lock(){
        checkForHierarchyValueViolation();
        if(!mtx.try_lock()){
            return false;
        }
        updateHierarchyValue();
        return true;
    }
    
    /*解锁的时候保证是从加锁的方向的逆顺序*/
    void unlock(){
        if(thisThreadHierarchyValue!=hierarychyValue){
            throw std::logic_error("mutex hierarchy violated!");
        }
        /*解锁*/
        mtx.unlock();
        /*更新层级编号*/
        thisThreadHierarchyValue=previousHierarchyValue;
    }
    
    
};
type class::var=init;
thread_local unsigned long HierarchicalMutex::thisThreadHierarchyValue=ULONG_MAX;



是这样使用吗？是
void function(){
 HierarchMutex high_level_mutex(1000);
 HIerarchMutex low_level_mutex(500);
    
 /*先锁定层级高的互斥，再锁定层级低的互斥，这并没有什么问题*/
 high_level_mutex.lock();
 low_level_mutex.lock();
    
 /*如果相反呢？这可能出现问题了,违反了层级锁的原则，在high_level_mutex.lock()的check的时候抛出异常*/
  low_level_mutex.lock();
  high_level_mutex.lock();
    
    
    
  /*解锁的操作先解锁低级锁再解锁高级锁这没有什么问题*/
   low_level_mutex.unlock();
   high_level_mutex.unlock();
    
  /*先解锁高级锁再解锁低级锁这就出现问题了*/
    high_level_mutex.unlock();//直接抛出异常
    low_level_mutex.unlock();
  
}




5将准则推广到锁操作之外
    我们通过本节的学习知道，死锁的现象不单单是因为加锁操作而产生，任何同步机制导致的循环等待都会导致死锁的产生。我们应该避免获取嵌套锁，类似的如果一个线程如果已经持有锁，如果再想加锁，只能比当前的层级低的加锁
```

***3.2.6使用排他锁std::unique_lock<>进行灵活加锁***

```c++
 /*使用std::lock()函数和unique_lock<>进行加锁*/
 
 class Data;
 void swap(Data& lhs,Data& rhs);
 class Object{
   friend void swap(Data& lhs,Data& rhs){
       if(&rhs==&lhs){return;}
       /*将互斥保留作无锁状态*/
       std::unique_lock<std::mutex> lock_a(lhs.mtx,std::defer_lock);
       std::unique_lock<std::mutex> lock_b(rhs.mtx,std::defer_lock);
       std::lock(lock_a,lock_b);/*在这里才开始加锁*/
       std::swap(lhs.data,rhs.data);
   }
   private:
     Data data;
     std::mutex mtx;
   public:
     Object(const Object& other):data(other){}
 };
 
unique_lock和lock_guard仅有微小的差别，那就是前者占用的空间大(可能是类的设计的原因),前者用起来比后者慢。
unique_lock类内部维护着lock(),unlock(),try_lock()三个成员函数,这也就解释了为什么unique_lock实例可以当作lock的传入参数。此外，unqiue_lock内部维护者一个标志，标明当前的互斥是否被当前的unique_lock实例所占有。这一个标志必须存在，作用是保证当前unique_locks实例的析构函数正确调用unlock().如果当前的互斥量确实被当前的unique_lock实例所占有,那么这个实例的析构函数必须调用unlock,否则就绝不能调用unlock.

我们知道，上述标记需要占用空间，这也就是为什么unqiue_lock体积大的原因，而且标记需要进行合适的检查和过呢更新，所以若采用将unique_lock换为lock_guard或者scoped_lock会导致轻微的性能损失，所以如果日常够用,请采用lock_guard;

```

***3.2.7 使用unique_lock在不同的作用域之间转移线程的归属权***

```c++
  因为unique_lock对象不占有与之关联的互斥，所以随着其对象的转移，互斥的归属权可以在多个unique_lock对象之间转移，转移在某些情况下会自动发生，我们需要针对个别的情形显示的调用std::move.本质上，这取决于移动的数据源是左值还是右值，如果是左值，必须显示的调用move，以免归属权被转移到别处，如果是右值转移会自动进行，unique_lock属于可移动却不可复制的类别。
  转移有一种用处，让函数锁定互斥后，然后把互斥的归属权(锁)转移给函数调用者，让他们在同一个锁的保护下执行其他操作。
  
  std::unique_lock<std::mutex> getLock(){
      std::mutex mtx;
      std::unique_lock<std::mutex> lock(mtx);/*先为prepar_data()锁定互斥，然后将锁返回给函数调用者*/
      prepar_data();
      return lock;
  }

  void process_data(){
      std::unique_lock<std::mutex> lock(getLock());//在转移锁的归属权时候就准备好了数据，秒啊
      doElseSomething();/*做一些其他的事情而且被锁住了 秒啊*/
  } 
```

***3.2.8按合适的粒度加锁***

```c++
  我们知道，锁粒度通常描述一个锁所保护的数据量，粒度精细的锁保护少量数据，粒度粗大的锁保护大量的数据。我们希望寻找到一种合适的锁，一是锁的粒度相对较大，确保目标数据得到保护，而是限制线程持锁的范围，务必只在必要的阶段持锁。多线程也试用这个道理，如果多个线程正在等待同一资源，如果任何线程在其必要范围内持锁，就会增加总的耗费时间。因此只要条件允许，我们仅在访问共享数据期间才锁住互斥，让数据处理尽可能不用锁保护。
  特别注意，线程在持锁期间应避免任何耗时的操作，如读写文件。同样是读写等量的数据，文件操作通常要比内存操作慢几千倍，除非锁的本意是保护文件的互斥访问，否则，IO操作加锁将阻塞其他线程(它们都在等待获取锁而被长时间的阻塞)，即使运用了多线程也无法提升性能。

 假如函数/代码不必访问共享数据，那我们就调用unlock解锁，若以后的代码需要重新访问，就调用lock加锁。
  void function(){
      std::unique_lock<std::mutex> lock(mtx);
      prepare_data();<-----假设在调用prepare期间无需枷锁
      lock.unlock();
      lock.lock();
      write_result_to_file();<------------重新锁住互斥以写出结果
  在末尾自动解锁----->}

 希望读者可以认清下面的事实，若只用单一的互斥保护整个数据结构，不但很可能加速锁的争夺，还难以缩短持锁时间，假设某项操作对一个互斥全程加锁，当中涉及的操作越多，持锁的时间也就越长，这是一种双重损失，恰恰想让我们使用精细度底的锁，下面的例子就得不尝失了。
  void function(){
      std::unique_lock<std::mutex> lock(mtx);
      prepare_data();<-----假设在调用prepare期间无需枷锁
      write_result_to_file();<------------重新锁住互斥以写出结果
  在末尾自动解锁----->}
  
 




	上述例子说明，加锁要选用恰当的粒度，不仅要考虑保护的数据量的大小，还要考虑持锁的时间和持锁时间进行了什么样的操作，一般地，若要执行某项操作，那我们应该只在所需的最短时间内持锁，换而言之，除非绝对必要，否则不得在持锁时间进行耗时操作，如等待IO操作完成或者获取另一个锁。
    下面的operator==操作，分别对两个互斥加锁使持锁的时间最短,但其中可能存在一些小小的细节问题.......

        
        
        
class Object{
  friend bool operator==(Object& lhs,Object& rhs){
      if(&lhs==&rhs){ return; }
      const int lhsValue=lhs.getData(); 2<----------------x时刻获得的值
      const int rhsValue=rhs.getData(); 3<---------------y时刻获得的值
      return lhsValue==rhsValue;
  }
  private:
    Data data;
    std::mutex mtx;
  public:
    Y(Data _data):data(_data);
  public:
    int getData(){
        std::lock_guard<> lock(mtx);
        return data;
    }
};
这样加锁的时间确实会缩短，但是比较的不是同一时刻的两个值。是比较的x时刻的lhsvalue与y时刻的rhsvalue。
     
     
```

***3.3 保护共享数据的其他工具***

```c++
  有些共享数据仅仅在初始化阶段进行保护就行了，若加锁仅仅是为了保护共享数据的初始化，在共享数据初始化之后仍然对其加锁，那就成了画蛇添足了。c++提供了一套机制，在共享数据初始化进行保护。
  假设我们需要某个共享数据(例如返回一个数据库连接的句柄)，但是它创建起来的开销很大，因为创建一个数据库连接需要分配大量的内存，所以等到必要使用的时候才会真正创建它，这种方式称之为延迟初始化，lazy initialization.常见于单线程代码，对于需要用到共享数据的操作，要先判断这个共享资源是否已经初始化，若没有，要先进行初始化后才方可使用。
 
  /*常见的单线程延迟初始化的代码*/      
  std::shared_ptr<data> pointer;
  void function(){
      if(!pointer){
          poniter.reset(new data);
      }
      pointer->dosomething();
  }

  /*用互斥实现线程安全的延迟初始化*/
  std::shared_ptr<data> pointer;
  std::mutex mtx;
  void function(){
      std::unqiue_lock<std::mutex> lock(mtx); 1<--------------
      if(!pointer){
          pointer.reset(new data);
      }
      lock.unlock();
      pointer->dosomething();
  }
  上述的代码很安全，但是有个致命的问题，所有的线程都在等待共享资源的初始化，造成线程阻塞。
      
  /*对上面的改进，但是备受诟病的双重检验锁定模式*/
   void withDoubleCheckedLock(){
      if(!pointer){ 1<--------------------         thd1
          std::lock_guard<> lock(mtx); 2<--------------------   thd2
          if(!pointer){
              pointer.reset(new data);3<---------------------
          }
      }
      
      pointer->dosomething(); 4<------------------------
  }
  


  首先，在无锁的条件下读取指针(1)，只有读取到空指针，随即立即加锁，但是这两步之间存在空隙，可能有其他的线程对pointer在这个间隙内进行了初始化，所以我们在初始化pointer之前，仍要检验一次（所谓的双重检验），乍看代码很美好，但是里面有很大的问题，会产生数据竞争/读的数据不一致的问题，(如果thd2一口气执行完毕，在4处操作新实例，更改了实例的初始值，当thd1过来时却毫不知情，thd1操作新实例仍按初始值操作，可能就会产生错误)
  断言：predicate 是函数或者仿函数返回布尔值，用于判断某个条件是否成立。
    
   
      
      
   为了解决延迟初始化的数据竞争问题/数据不一致的问题，c++引入了std::call_once.std::once_flag，令所有线程共同调用std::once函数，从而确保在函数调用返回时，pointer初始化由某个线程安全且唯一的完成。必要的同步数据由std::once_flag对象存储，每个once_flag对象对应一次不同的初始化，相比像上面显示的使用互斥，使用此函数开销更低(没有线程阻塞)，所以仅在初始化时共享变量受到的保护，最好采用此方法。
   
std::shared_ptr<data> pointer;
std::once_flag data_flag;
   
void init_data(){
       pointer.reset(new data);
   }
   
void function(){
       std::call_once(data_flag,init_data);
       pointer->doSomething();
   }

void main(){
    function();
    function(); // 是否可以这么理解，由于data_flag的存在，init_data只会调用一次。
}



/*更加细节的东西*/
class Object{
  private:
    connection_info connection_details;
    connection_handle connection;
    std::once_flag connection_init_flag;
  private:
    void open_connction(){
        connection=connection_manager.open(connection_details);
    }
  public:
    /*构造函数*/
    Object(const connection_info& _connection_detalis) : connection_detalis(_connection_details){}
  public:
    void send_data(const data_packet& data){ 1 <------
        std::call_once(connection_flag,&Object::open_connection,this);2<-----------
        connection.send_data(data);
    }
    data_packet receive_data(){3<---------------------
        std::call_once(connection_flag,&Object::open_connect,this); 2<--------------
        return connection.receive_data();
    }
};
   
void main(){
    data_packet somedata;
    connect_info somedetails;
    Object object(somedetails);
    objcet.send_data(somedata);// 会进行object的connection的初始化
    data_packet packet=object.receive_data(); //不会重新初始化object的connection，因为已经在前面初始化过了。
}

object要么在1要么在3在其2处进行connection的初始化，传入的this指针代表初始化的是当前对象的connection。 
 

    
```

***[扩展] 普通函数中将成员变量声明为static带来数据竞争的问题***

```c++
int& get_data_instance(){
	static int data;    
}

  根据c++的设计规则，只要控制流程第一次遇见static数据的声明语句，数据便会初始化，若多个线程调用一个函数，其内部含有一个静态变量，则任意线程都可能首先到达这个静态变量的声明处，众多线程都认为自己是先到达的都试图初始化变量；也有可能有一个线程正在初始化这个静态数据，但尚未完成，别的线程企图使用它....这些都构成了数据竞争。
  幸运的是，c++帮我们解决了这个问题，c++规定，初始化在一个单独的线程上完成，在静态数据未初始化完成之前，所有的线程不会超过静态数据的声明而继续运行。这样多个线程就可以放心的抵用get_data_instance函数了。
   仅仅在初始化的时候保护共享数据是一个特例，更普遍的是保护那些甚少更新的数据结构，这些数据很少更新，因此可以被多个线程并发访问，但偶尔也需要一些更新，我们需要一种新的机制来处理这种情况。
```

***3.3.2保护甚少更新的数据结构***

```c++
  考虑有一个DNS缓存表，将域名解析为对应的IP地址，我们知道DNS在很长的一段时间内会保持不变。为了判断数据是否有效，必须定期检查缓存表，只要相关的细节有所改动,就必须更新缓存表。
如果缓存表被多线程访问，现实中也确实如此，那么缓存表在更新的过程中应该得到妥善的保护，等到缓存表更新完成后这些多线程才能够访问。
  这本质是一个读者写者问题，一个写者在写的时候其他多个读者暂时不能读，直到写者写完更新完数据。（旨在写者更新数据的那么一小段时间内禁止并发访问）有多个读者在读数据的时候这个写者不能写进行更新数据，直到读者读取完毕。
  这属于读写互斥，c++提供了专门的读写互斥，std::shared_mutex和std::shared_timed_mutex(允许一个写线程进行完全的排他访问进行更新数据，也允许多个读线程共享数据或并发访问)。它们都在Boost库中，两者的区别在于后者支持的操作更多，进行的开销更多。，如果没有额外操作，建议使用前者，这会带来性能增益。
  对于某个写线程进行写操作更新缓存表可以使用**lock_guard<std::shared_mutex>/std::unique_lock<std::shared_mutex>**锁定，代替std::mutex的特化，它们都于std::mutex一样，都保证了访问的排他性质。
  对于多个读线程（无需更新数据结构的线程）可以使用共享锁***std::shared_lock<std::shared_mutex>***实现共享/并发的访问缓存表。
  对于共享锁，std::shared_lock<>，使用方式与其他锁基本相同，只不过是多个线程能够同时锁住***同一个***
std::shared_mutex。
 [限制]  共享锁仅有一个限制，如果它已经被某些线程持有，若其中有别的线程想要获取排它锁，这些已经获取排他锁的线程就会发生阻塞，直到这些已经获取排他锁的线程全部释放共享锁。如果任意线程持有排他锁，那么其他线程都无法获取该排他锁，直到持锁线程将该排他锁释放为止。
        
        
/*使用std::shared_mutex保护数据结构DNS缓存表*/
#include  <map>
#include  <string>
#include  <mutex>
#include  <shared_mutex>

class dns_entry;

class dns_cache{
	  private:
    	std::map<std::string,dns_entry> entries;
    	std::shared_mutex entry_mutex;
      public:
        /*读操作*/
    	/*多个线程可以持锁并发访问*/
    	dns_entry find_entry(const std::string& domain) const{
            std::shared_lcok<std::shared_mutex> lock(entry_mutex);
            std::map<std::string,dns_entry>::const_iterator iterator=enryies.find(domain);
            return (iterator==entries.end() ? NUll:iterator->second);
        }
        /*写操作*/
        /*当多个线程并发进行更新时，第一个到达的线程先进行更新，其他的阻塞，而后重复上面，直到所有线程更新结束*/
    	void update_entry(const std::string& domain,const dns_entry& dns_entries){
        	std::lock_guard<std::shared_mutex> lock(entry_mutex);
            entries[domain]=dns_entries;
        }
};
   
        
      
```

