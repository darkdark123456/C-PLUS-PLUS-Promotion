/*
 * @author: MENG ZHEN CHUAN
 * @date: do not edit
 * @description: HADOOP Project
 * @method: note
 */
/**
 * 绝对安全的二级内存分配器，注意全部成员变量和方法用的static，方便直接用类名调用
 **/
#include <cstdlib>
#include "FirstLevelMemoryAllocator.h"



/**
 * 向上对齐使用的字节数,最小分配的字节数
*/
enum{ALIGN=8};
/**
 * 最大分配的字节数
*/
enum{MAX_BYTES=128};
/**
 * list node节点的数量
*/
enum{FREE_LIST_NUMBER=MAX_BYTES/ALIGN};


class SecondLevelMemoryAllocator{
    public:
        SecondLevelMemoryAllocator()=default;
        SecondLevelMemoryAllocator(const SecondLevelMemoryAllocator &)=default;
        SecondLevelMemoryAllocator &operator=(const SecondLevelMemoryAllocator &)=default;
        ~SecondLevelMemoryAllocator()=default;
    private:
        struct object{
            struct object * freeListLink;
        };
        /**
         * FreeList指向该list的头
        */
        static object * FreeList[FREE_LIST_NUMBER];
    private:
        /**
         * pool newest status
        */
        static char   * startFreePointer;
        static char   * endFreePointer;
        static size_t heapSize;
    private:
        /**
         * if user apply  n bytes,os will roundup n to the multiples(倍数) of 8，example ,the client apply 13B,os will apply 16B to client
        */
        static size_t ROUND_UP(size_t bytes){
            return (bytes+ ALIGN -1) & ~(ALIGN -1);
        }
        /**
         * locate to where the list node
        */
        static size_t FREELIST_INDEX(size_t bytes){
            return (bytes+ ALIGN -1) / ALIGN -1;
        }
    private:
        /**
         * refill n ？
        */
        static void * refill(size_t n);
        /**
         * allocate n*NOBJS B ,n:bytes 已经变为8的倍数，NOBJS : 初值为20,但不一定能malloc到20 
        */
        static char * chunkAlloc(size_t n,int & NOBJS);
        /**
         * 开始试着分配内存
        */
        static void * Allocate(size_t n);
    public:
        static void print();
};


SecondLevelMemoryAllocator::
object* SecondLevelMemoryAllocator::FreeList[FREE_LIST_NUMBER]={0,0,0,0,
                                                                0,0,0,0,
                                                                0,0,0,0,
                                                                0,0,0,0};                                                              
char * SecondLevelMemoryAllocator::startFreePointer=nullptr;
char * SecondLevelMemoryAllocator::endFreePointer=nullptr;
size_t SecondLevelMemoryAllocator::heapSize=0;



void * SecondLevelMemoryAllocator::Allocate(size_t n){
    object ** MyFreeList;
    object *result;
    /**
     * 申请的内存太大，二级内存分配器无法满足要求，改为一级内存分配器
    */
    if(n>MAX_BYTES){
        return malloc(n);
    }
    else{
        /**
         * 让MyFreeList指向应该分配block的list node
        */
        MyFreeList=FreeList + FREELIST_INDEX(n);
        result=*MyFreeList;
        /**
         * 此时MyFreeList没有任何指向
        */
        if(result==nullptr){
            void *r=refill(ROUND_UP(n));
        }
    }
}


/**在FreeList上挂载block*/
void * SecondLevelMemoryAllocator::refill(size_t n){
    int NOBJS=20;
    char *chunk=chunkAlloc(n,NOBJS);
    object **myFreeList;
    object *result;
    object *currentObject;
    object *nextObject;
    int number;
    if(NOBJS==1){   return (chunk); }
    myFreeList=FreeList+FREELIST_INDEX(n);
    /**
     * 找到第一个分配给客户后的第一个未被使用的block
     */
    *myFreeList=result=(object *)(chunk+n);
    for(number=1;;number++){
        currentObject=nextObject;
        /**指向下一个节点*/
        nextObject=(object*)((char *) nextObject +number);
        if(number==NOBJS-1){
            currentObject->freeListLink=nullptr;
            break;
        }
        else{
            currentObject->freeListLink=nextObject;
        }

    }
    return result;
    }


/**返回一个n个字节的pool，让一个char * pointer指向这个pool*/
char * SecondLevelMemoryAllocator::chunkAlloc(size_t size,int &NOBJS){
    /*list获得block的起始位置*/
    char * result; 
    size_t totalBytes=size * NOBJS;
    size_t bytesLeft=endFreePointer-startFreePointer;
    /**
    * pool 满足分配的内存要求
    */
    if(bytesLeft >= totalBytes){
        result=startFreePointer;
        startFreePointer+=totalBytes;
        /*返回从pool划分的块的指针*/
        return result; 
    }
    /**
    * 满足一块或者多块
    */
    else if(bytesLeft >= size){
        /*实际获得的block数*/
        NOBJS=bytesLeft / size;
        /*改变需求量，减少从pool划分的字节总量*/
        totalBytes=NOBJS*size;
        result=startFreePointer;
        startFreePointer+=totalBytes;
        /*返回从pool划分的块的指针*/
        return result;
    }
    else{
       /**
       * 连一块也不满足,先回收不能满足的pool的内存块
       */
        size_t BytesToGet=2*totalBytes+ROUND_UP(heapSize >> 4);/**应该申请的总字节*/
        if(bytesLeft>0){
            /**
             * 巧妙的类型转换进行内存碎片回收
            */
            object** myFreeList=FreeList+FREELIST_INDEX(bytesLeft);
            ((object*)startFreePointer)->freeListLink=*myFreeList;
            *myFreeList=(object *)startFreePointer;
       }

       /**
       * malloc 失败
       */
       if(startFreePointer==nullptr){
            int number;
            object ** myFreeList;
            object *pointer;
            for(number=size;number<=MAX_BYTES;number+=ALIGN){
                /**
                 * 定位到number的节点
                */
                myFreeList=FreeList+FREELIST_INDEX(number);
                pointer=*myFreeList;
                /**
                 * 将第一个可用节点作为pool分配
                */
                if(pointer != nullptr){
                    /** *myFreeList此时指向第一个空block */
                    *myFreeList=pointer->freeListLink;
                    startFreePointer=(char*)pointer;
                    endFreePointer=startFreePointer+number;
                    return chunkAlloc(size,NOBJS);
                }
            }
            /**
             * 内存容量山穷水尽，穷途末路，已经无法满足用户需求，改用一级分配器
            */
            endFreePointer=nullptr;
            startFreePointer=(char *)malloc(BytesToGet);/**申请一块BytesToGet大小的内存块，让startFreePointer指向它*/
       }
       heapSize+=bytesLeft;
       endFreePointer=startFreePointer+bytesLeft;
       /**
        * 递归的分配内存
       */
       return (chunkAlloc(size,NOBJS));
    }

}


void SecondLevelMemoryAllocator::print(){
    std::cout<<startFreePointer-endFreePointer<<std::endl;
}





int main(int argc, char const *argv[])
{   /**没有测试分配内存*/
    SecondLevelMemoryAllocator secondLevelMemoryAllocator;
    return 0;
}
