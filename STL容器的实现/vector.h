#ifndef _VECTOR_H_
#define _VECTOR_H_



#include<memory>
using std::size_t;
using std::allocator;
using std::uninitialized_fill_n;
using std::uninitialized_copy;
using std::_Destroy;
using std::copy;
using std::uninitialized_copy_n;



namespace MENG{
/*一个简易的分配器*/
template<typename T,typename Alloc=std::allocator<T>>
class  Simple_Alloc{
    public:
        Alloc alloc; //创建一个分配器
        T* allocate(size_t n){return 0==n ? nullptr : (T*)alloc.allocate(n*sizeof(T));}
        T*allocate(void){return  (T*)alloc.allocate(sizeof(T));}
        void construct(T*&q,T object){alloc.construct(q++,object);}
        void destroy(T*iterator1,T*iterator2){_Destroy(iterator1,iterator2);}
        void destroy(T*iterator){_Destroy(iterator);}
        void deallocate(T*p,size_t n){if (n!=0){ alloc.deallocate(p,n*sizeof(T));};}
        void deallocate(T*p){alloc.deallocate(p,sizeof(T));}

};


template<typename T,typename Alloc=std::allocator<T>>
class  vector{
        typedef T value_type;
        typedef T* iterator;
        typedef T& reference;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
    public:
        typedef Simple_Alloc<T> Data_Alloc;
        iterator start;
        iterator finish;
        iterator end_of_element;
        Data_Alloc alloc;
    public:
        inline void insert_aux(iterator postion,const T&element);
        inline void deallocate();
        inline void fill_initialize(size_t n,const T&element);
        inline void fill_initialize(size_t n);
    public:
        vector() :start(0),finish(0),end_of_element(0){}
        vector(size_type n,const T&element){fill_initialize(n,element);}
        explicit vector(size_type n){fill_initialize(n);} //只指定vector大小的初始化， 如果T是int，那么int() 返回0，填充n个0
        ~vector(){}
    public:
        inline iterator  begin()          {return start;}
        inline iterator  end()            {return finish;}
        inline iterator  end_of_Element() {return end_of_element;}
        inline size_type size()const      {return finish-start;}
        inline size_type capacity()const  {return end_of_element-start;} 
        inline bool      empty()const     {return start==finish ? 1:0;}
        inline reference front()          {return *begin();}
        inline reference back()           {return *(end()-1);}
        inline reference operator[](size_type n){return *(begin()+n);} // 重载[],  用[]取元素
        inline void push(const T&element);
        inline void pop();
        inline iterator erase(iterator pos); // 清除某个位置上的元素
        inline iterator erase(iterator first,iterator last); 
        inline void resize(size_type new_size,const T&element);
        inline void resize(size_type new_size);
        inline void clear(){erase(begin(),end_of_Element());}
        inline void insert(iterator postion,size_t n,const T&element);
        inline void sort();
    protected:
        iterator allocate_Fill(size_type n,const T&element);
};


template<typename T,typename Alloc>
void   MENG::vector<T,Alloc>::fill_initialize(size_t n,const T&element){
    start=allocate_Fill(n,element);
    finish=start+n;
    end_of_element=finish;
}

/*只指定容器大小的初始化*/
template<typename T,typename Alloc>
void  MENG::vector<T,Alloc>::fill_initialize(size_t n){
    start=allocate_Fill(n,T());
    finish=start+n;
    end_of_element=start;
}



/*为vector分配内存，用 填充元素 的方式*/
template<typename T,typename Alloc>
T* MENG::vector<T,Alloc>::allocate_Fill(size_type n,const T&element){
    T* p=alloc.allocate(n);
    uninitialized_fill_n(p,n,element);
    return p;
}



/*释放已为元素分配的内存块，这些释放后的内存块可再次使用*/
template<typename T,typename Alloc>
void MENG::vector<T,Alloc>::deallocate(){
    if (start){
        alloc.deallocate(start,end_of_element-start);
    }  
}


template<typename T,typename Alloc>
void MENG::vector<T,Alloc>::push(const T&element){
    if (end()!=end_of_element){
        alloc.construct(end_of_element,element);
    }
    else{
        insert_aux(end(),element);
    }  
}


template<typename T,typename Alloc>
void MENG::vector<T,Alloc>::insert_aux(iterator postion,const T&element){
    const size_type old_Size=size();
    const size_type new_Size=(old_Size==0?1:3*old_Size);
    // 分配一个新的容器
    iterator new_Start=alloc.allocate(new_Size);
    iterator new_Finish=new_Start;
    iterator new_End_of_element=new_Start;
    new_End_of_element=uninitialized_copy(start,postion,new_Start);
    alloc.construct(new_End_of_element,element);
    new_Finish=new_Finish+new_Size;
   // 释放以前的容器
    alloc.destroy(start,finish);
    alloc.deallocate(start,size());
   //改变三个指针
   start =new_Start;
   finish=new_Finish;
   end_of_element=new_End_of_element;

}


template<typename T,typename Alloc>
void MENG::vector<T,Alloc>::pop(){
    if (begin()!=end_of_element){   
        --end_of_element;
        alloc.destroy(end_of_element);
    }

}


template<typename T,typename Alloc>
T* MENG::vector<T,Alloc>::erase(iterator first,iterator last){
    if(last==end_of_element&&first==start){
        alloc.destroy(first,last);
        end_of_element=start;
        }
    else{
        alloc.destroy(first,start);
        ::copy(last,end_of_element,first);
        end_of_element=first+(end_of_element-last);
    }
        

}


template<typename T,typename Alloc>
T* MENG::vector<T,Alloc>::erase(iterator pos){
    if (pos+1==end_of_element){
        alloc.destroy(pos);
        --end_of_element;
    }
    else{
        alloc.destroy(pos);
        ::copy(pos+1,end_of_element,pos);
        --end_of_element;
}



}


template<typename T,typename Alloc>
void MENG::vector<T,Alloc>::insert(iterator postion,size_t n,const T&element){
    //容器的大小充足
    if (n>0){
        if(finish-end_of_element>=n){
            if(postion==end_of_Element()){
                uninitialized_fill_n(postion,n,element);
                end_of_element=end_of_element+n;}
            else{
               uninitialized_copy_n(postion,end_of_element-postion,postion+n);
               uninitialized_fill_n(postion,n,element);
               end_of_element=(end_of_element-postion)+n+postion;
                 }
                                         }
                                     
    //容器的大小不足，重新分配一个新的容器
        else{
            const size_type old_Size=size();
            const size_type old_Capacity=capacity();
            const size_type new_Size=(old_Size==0?1:3*old_Size);

            // 分配一个新的容器
            iterator new_Start=alloc.allocate(new_Size);
            iterator new_Finish=new_Start;
            iterator new_End_of_element=new_Start;
            uninitialized_copy(begin(),end_of_Element(),new_Start);
            new_End_of_element=new_End_of_element+old_Capacity;
            new_Finish=new_Finish+new_Size;
            auto postion_num=end_of_element-postion;//记录postion指针

        // 释放以前的容器
            alloc.destroy(start,end_of_element);
            alloc.deallocate(start,size());
            //改变三个指针
            start =new_Start;
            finish=new_Finish;
            end_of_element=new_End_of_element;
            postion=end_of_element-postion_num;

        // 和上述逻辑一样，开始插入n个element
            if(postion==end_of_element){
                uninitialized_fill_n(postion,n,element);
                end_of_element=end_of_element+n;
                                         } 
            if(postion!=new_End_of_element){
               uninitialized_copy_n(postion,end_of_element-postion,postion+n);
               uninitialized_fill_n(postion,n,element);
               end_of_element=(end_of_element-postion)+n+postion;
                }
            
             }



            }

}        
}
#endif