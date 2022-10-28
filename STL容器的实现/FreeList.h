#ifndef FreeList_h
#define FreeList_h
#include <cstdlib>





enum {MIN_BYTES=8};
enum {MAX_BYTES=128};
enum {FREE_LIST_NODE_SIZE=MAX_BYTES/MIN_BYTES};


union FreeListNode{
    char MemorySize[1];
    union FreeListNode * nextFreeListNode;
};


class FreeList{
    private:
        FreeListNode *freeListNodeLink;
        char *S_Start_Free;
        char *S_end_Free;
        unsigned int heapSize;
        unsigned int freeListLength=FREE_LIST_NODE_SIZE;
    public:
        FreeList();
        FreeList(const FreeList& object)=default;
        FreeList & operator=(const FreeList &object)=default;
        ~FreeList();
    public:
        /**
         * 内存调整为8的倍数
        */
        unsigned int AlignUp(unsigned int bytes){
            return (bytes +MIN_BYTES-1) & ~(MIN_BYTES-1);
        }

    public:
        bool fillChunkForNode();
        void AllocateLinkedMemory(unsigned int memorySize);
        void DellocateMemory();
    

};


FreeList::FreeList(){
    freeListNodeLink=(FreeListNode*)malloc(freeListLength*sizeof(FreeListNode));

    fillChunkForNode();
    
}



FreeList::~FreeList(){

}

bool FreeList::fillChunkForNode(){
    unsigned int listLength=begin()-end();
    FreeListNode *pointer=NULL;
}



void FreeList::AllocateLinkedMemory(unsigned int memorySize){
    if(memorySize>128){
     (int *)malloc(memorySize * sizeof(int));//有一个模板参数
    }

}


#endif