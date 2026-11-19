如何理解内存序的底层到底做了什么？
std::atomic<T>::compare_exchange_weak
std::atomic<T>::compare_exchange_strong

  在进行比较交换中，由于cpu架构的不同底层实现的也不同。
在ARMV8.0以下，比较交换指令不是由一条lock指令执行的，winx86 64架构下比较交换指令是由一条lock指令执行的，天然自带原子屏障操作，而ARM8.0以下需要监视器配合其他指令来完成原子操作，指令中间是有"缝隙"的。

retry:
    ldxr  w1, [x0]      ; ①
    add   w1, w1, #1    ; ②
    stxr  w2, w1, [x0]  ; ③
    cbnz  w2, retry     ; ④
    dmb   ish           ; ⑤

在理解指令之前，先了解指令的具体含义:

