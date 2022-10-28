/*
 * @author: MENG ZHEN CHUAN
 * @date: do not edit
 * @description: HADOOP Project
 * @method: note
 */
#ifndef FirstLevelMemoryAllocator_H
#define FirstLevelMemoryAllocator_H

#include <cstdlib>
#include "MemoryOutOfBoundException.h"
using std::size_t;
/**
 * This is a contiguous memory allocator without memory out-of-bounds
 * /*/
template<typename T>
class FirstLevelMemoryAllocator{
    private: 
        /**
        * 分配的内存块大小
        * /*/
        unsigned long allocateSize;
        T *beginPointer;
        T *endPointer;
        T *nowElementPointer; 
    public:
        FirstLevelMemoryAllocator()=default;
        ~FirstLevelMemoryAllocator()=default;
        FirstLevelMemoryAllocator(const FirstLevelMemoryAllocator&object)=default;
        FirstLevelMemoryAllocator& operator=(const FirstLevelMemoryAllocator &object)=default;
    public:
        /**
         * 分配内存并设置指针防止内存越界
         * /*/
        inline void  AllocateSequence(long size){
            allocateSize=size;
            beginPointer=(allocateSize==0 ? (T*)calloc(1,sizeof(T)) : (T*)calloc(allocateSize,sizeof(T)));
            setNowelementPointer();
            setEndPointer();
        }
        /**
        * 向内存中填入数据
        * /*/
        inline  void Construct(T object){
            
            if(nowElementPointer<endPointer){
                (*nowElementPointer++)=object;
                return; 
            }
            else{
                throw MemoryOutOfBoundException{};/*抛出内存越界异常*/
                return;   
            }  
        }
        /**
         *释放掉分配的内存
         * /*/
        inline  void DeallocateSequence(){  
            free(beginPointer); 
            beginPointer=NULL;
            nowElementPointer=NULL;
            endPointer=NULL;
        }
        /**
         *返回指向第一个元素的指针
         * /*/
        inline T* begin(){
            return beginPointer;
        }
        /**
         *返回指向最后一个元素的指针
         * /*/
        inline T* end(){
            return endPointer;
        } 
    protected:
        inline void setEndPointer(){
            if(allocateSize==0){
                endPointer=beginPointer+allocateSize+1;
            }
            else{
                endPointer=beginPointer+allocateSize;
            }
        }
        inline void setNowelementPointer(){
            nowElementPointer=beginPointer;
        }
};
#endif