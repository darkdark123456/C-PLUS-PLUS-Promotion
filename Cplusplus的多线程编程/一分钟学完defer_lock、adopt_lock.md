###### 原子变量atomic

1 可以用将int变量原子化用来计数(本质还是原子变量具有互斥性)

2 可以将bool变量原子化来通过主线程控制创建的线程的运行时间

###### std::defer_lock std::adopt_lock 和std::lock

1 需要注意的是std::lock的参数形式std::lock(lockable& lock_1,lockable& lock_2,...)

2 std::defer_lock  延迟加锁的枚举类型，std::unique_lock<mtx,std::defer_lock>表示我会对前面的mtx互斥量加锁，但是要等到std::lock()才进行，由于std::lock()的参数至少为两个，所以使用std::defer_lock至少对两个互斥量加锁；

```c++
#include <mutex>
#include <thread>
#include <iostream>

class BankCount{
  public:
    double balance;
    std::mutex mtx;
  public:
    explicit BankCount(int _balance):balance(_balance){}  
};

void transfer(BankCount& fromCount,BankCount& toCount,double amount){
    if(&fromCount==&toCount){return;}
    std::unique_lock<std::mutex> lock_1(fromCount.m,std::defer_lock);
    std::unique_lock<std::mutex> lock_2(toCount.m,std::defer_lock);
    std::lock(lock_1,lock_2);// 由于lock_1 lock_2的枚举类型为defer_lock好处是不用手动解锁
    fromCount.balance-=amount;
    toCount.balance+=amount;
在这里持有锁的线程自动释放两个锁，lock_1,lock_2---------------->}

int main(){
    BankCount  zhangsan(100);
    BankCount  mengzhenchuan(200);
    std::thread t1(transfer,std::ref(zhangsan),std::ref(mengzhenchuan),50);
    t1.join();
    std::cout<<"zhangsan count "<<zhangsan.balance<<std::endl;
    std::cout<<"mengzhenchuan count"<<mengzhenchuan.balance<<std::endl;
}
```

3 std::adopt_lock 和std::defer_lock的用法差不多，它说明的是你前面要保证对mtx加了锁，我后面也就不用手动解锁了

```c++
#include <mutex>
#include <thread>
#include <iostream>

// 与上面等价的代码

class BankCount{
  public:
    double balance;
    std::mutex mtx;
  public:
    explicit BankCount(int _balance):balance(_balance){}  
};

void transfer(BankCount& fromCount,BankCount& toCount,double amount){
    if(&fromCount==&toCount){return;}
    std::lock(fromCount.m,toCount.m);
    std::unique_lock<std::mutex> lock_1(fromCount.m,std::adopt_lock);//你要保证对fromcount加了锁
    std::unique_lock<std::mutex> lock_2(toCount.m,std::adopt_lock);//你要保证对toCount加了锁
    fromCount.balance-=amount;
    toCount.balance+=amount;
在这里持有锁的线程自动释放两个锁，lock_1,lock_2---------------->}

int main(){
    BankCount  zhangsan(100);
    BankCount  mengzhenchuan(200);
    std::thread t1(transfer,std::ref(zhangsan),std::ref(mengzhenchuan),50);
    t1.join();
    std::cout<<"zhangsan count "<<zhangsan.balance<<std::endl;
    std::cout<<"mengzhenchuan count"<<mengzhenchuan.balance<<std::endl;
}
```

```c++
#include <mutex>
#include <thread>
#include <iostream>

// 与上面等价的代码 将unique_lock<>换做lock_guard<>

class BankCount{
  public:
    double balance;
    std::mutex mtx;
  public:
    explicit BankCount(int _balance):balance(_balance){}  
};

void transfer(BankCount& fromCount,BankCount& toCount,double amount){
    if(&fromCount==&toCount){return;}
    std::lock(fromCount.m,toCount.m);
    std::lock_guard<std::mutex> lock_1(fromCount.m,std::adopt_lock);//你要保证对fromcount加了锁
    std::lock_guard<std::mutex> lock_2(toCount.m,std::adopt_lock);//你要保证对toCount加了锁
    fromCount.balance-=amount;
    toCount.balance+=amount;
在这里持有锁的线程自动释放两个锁，lock_1,lock_2---------------->}

int main(){
    BankCount  zhangsan(100);
    BankCount  mengzhenchuan(200);
    std::thread t1(transfer,std::ref(zhangsan),std::ref(mengzhenchuan),50);
    t1.join();
    std::cout<<"zhangsan count "<<zhangsan.balance<<std::endl;
    std::cout<<"mengzhenchuan count"<<mengzhenchuan.balance<<std::endl;
}
```

