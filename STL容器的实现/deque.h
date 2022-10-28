#ifndef _DEQUE_H
#define _DEQUE_H


#include<memory>
using std::ostream;
using std::size_t;
using std::allocator;
using std::uninitialized_fill;
using std::uninitialized_copy;
using std::_Destroy;
using std::_Construct;
using std::copy;
using std::max;
using std::ptrdiff_t;
using std::equal;
using std::lexicographical_compare;



namespace MENG{
template<typename T,size_t BufSiz>  class _dueue_iterator;
template<typename T,typename Alloc,size_t BufSiz>   class deque;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t _deque_buf_size(size_t n,size_t sz){
    return n!=0 ?n:(sz<512?size_t(512/sz):size_t(1));
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////


template<typename T,size_t BufSiz>
class _dueue_iterator{
        typedef T** map_pointer;
        typedef _dueue_iterator<T,BufSiz> iterator;
        typedef _dueue_iterator<const T,BufSiz> const_iterator;
        typedef T value_type;
        typedef T* pointer;
        typedef T& reference;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef _dueue_iterator self;
    public:
        _dueue_iterator(T*x,map_pointer y):cur(x),first(*y),last(*y+buffer_size()),node(y){}
        _dueue_iterator():cur(nullptr),first(nullptr),last(nullptr),node(nullptr){}
        _dueue_iterator(const iterator&x):cur(x.cur),first(x.first),last(x.last),node(x.node){}
    public:
        self&       operator +=(ptrdiff_t n);
        self&       operator -=(ptrdiff_t n);
        self&       operator ++();
        self        operator ++(int);
        self&       operator--();
        self        operator--(int);
        self        operator+(ptrdiff_t n);
        self        operator-(ptrdiff_t n);
        size_t      operator-(iterator &iter);
        reference   operator*()  const          {return *cur;}
        pointer     operator->() const          {return cur;}
        reference   operator[](ptrdiff_t n)     {return *(*this+n);} 
        bool        operator==(const self &dt)  {return this->cur==dt.cur;}
        bool        operator!=(const self &dt)  {return !(this->cur==dt.cur);}
        bool        operator<(const self  &dt)  {return (node==dt.node)?(cur<dt.cur):(node<dt.node);}
    public:
        static  size_t buffer_size(){return _deque_buf_size(BufSiz,sizeof(T));}
        void set_node(T** new_node){node=new_node;first=*new_node;last=first+buffer_size();}//last=first +8
    public:
        T*cur;
        T*first;
        T*last;
        map_pointer node;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>
_dueue_iterator<T,BufSiz>& MENG::_dueue_iterator<T,BufSiz>::operator++(){
    ++cur;
    if (cur==last){
        set_node(node+1);
        cur=first;
        }
    return *this;  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::_dueue_iterator<T,BufSiz>::operator++(int n){
    iterator tmp=*this;
    ++*this;
    return tmp;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::_dueue_iterator<T,BufSiz>::operator+(ptrdiff_t n){
    iterator tmp=*this;
    return tmp+=n;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>
_dueue_iterator<T,BufSiz>& MENG::_dueue_iterator<T,BufSiz>::operator--(){
    if (cur==first){
        set_node(node-1);
        cur=last;
        }
    --cur;
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::_dueue_iterator<T,BufSiz>::operator--(int n){
    iterator tmp=*this;
    --*this;
    return tmp;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::_dueue_iterator<T,BufSiz>::operator-(ptrdiff_t n){
    iterator tmp=*this;
    return tmp-=n;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>
_dueue_iterator<T,BufSiz>& MENG::_dueue_iterator<T,BufSiz>::operator +=(ptrdiff_t n){
    difference_type offset=n+(cur-first);
    /*在同一缓冲区*/
    if( offset>=0 && offset<difference_type(buffer_size())) {
        cur+=n;
        }
    /*不在同一缓冲区*/
    else{
        difference_type node_offset =
        offset > 0 ? offset / difference_type(buffer_size()) : -difference_type((-offset - 1) / buffer_size()) - 1;
        set_node(node + node_offset);
        cur = first + (offset - node_offset * difference_type(buffer_size()));
        }
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>
_dueue_iterator<T,BufSiz>& MENG::_dueue_iterator<T,BufSiz>::operator -=(ptrdiff_t n){
    *this+=-n;
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,size_t BufSiz>//finish -(start+12)//finish-start
size_t MENG::_dueue_iterator<T,BufSiz>::operator-(iterator &iter){
    iterator    mstart=iter;
    iterator    mfinish=*this;
    size_t      size =0;
    map_pointer iter1=mstart.node+1;
    pointer     tmp;
    size_t      map_node_num=mfinish.node-mstart.node+1;
    size_t      buf_size=buffer_size();
    if(iter.first==this->first){size=(*this).cur-iter.cur;return size;}
    for(tmp=mstart.cur;tmp!=mstart.last;++tmp){++size;}
    for(;iter1!=mfinish.node;++iter1){
        for(tmp=*iter1;tmp!=*iter1+buf_size;++tmp){
                ++size;
            }
         } 
    for(tmp=mfinish.first;tmp!=mfinish.cur;++tmp){++size;}
    return size;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////


template<typename T,typename Alloc=std::allocator<T>,size_t BufSiz=0>
class deque{
        typedef T value_type;
        typedef T* pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T** map_pointer;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef _dueue_iterator<T,BufSiz> iterator;
        typedef _dueue_iterator<const T,BufSiz> const_iterator;
        typedef allocator<value_type>   data_allocator;
        typedef allocator<pointer>      map_allocator;
        friend  ostream&operator<<(ostream&os,const deque& du){
            size_t bufsize=du.buffer_size();
            T** mpointer=du.start.node+1;
            T*  bpointer;
            for(bpointer=du.start.cur;bpointer!=du.start.last;++bpointer){os<<*bpointer<<"  ";}
            for(;mpointer!=du.finish.node;++mpointer){
                for(bpointer=*(mpointer);bpointer!=*(mpointer)+bufsize;++bpointer){
                    os<<*bpointer<<"  ";
                }
            }
            for(bpointer=du.finish.first;bpointer!=du.finish.cur;++bpointer){os<<*bpointer<<"  ";}
            return os;
    }
    public:
        iterator start;
        iterator finish;
        map_pointer map;
        size_t map_size;
    public:
        deque():start(),finish(),map(nullptr),map_size(0)                      {creat_map_and_nodes(0);}
        deque(const deque&du):start(),finish(),map(nullptr),map_size(0)        {creat_map_and_nodes(du.size());uninitialized_copy(du.begin(),du.end(),start);}
        deque(int n,const value_type&value):start(),finish(),map(0),map_size(0){fill_initialize(n,value);}
        deque(int n, value_type*arr):start(),finish(),map(0),map_size(0)       {fill_initialize(n,arr);}
        ~deque()                                                               {destroy(start,finish);}
    protected:
        void creat_map_and_nodes(size_t num_element);
        void fill_initialize(int n,const value_type &value);
        void fill_initialize(int n,value_type *arr);
        void destroy(iterator &iter1,iterator& iter2);
        void destroy_map_noed();
        void copy(iterator first,iterator last,iterator object_first);
        void copy(iterator first,iterator last,iterator object_first,int i);
    protected:
        static  size_t   buffer_size()               {return _deque_buf_size(8,sizeof(T));}//8
        static  size_t   initial_map_size()          {return 8;}
        static  pointer  allocate_node()             {return data_allocator().allocate(buffer_size());}
        static  void     deallocate_node(pointer p)  {data_allocator().deallocate(p,buffer_size());}
        void     destroy_nodes_at_front(iterator before_start){for(map_pointer tmp=before_start.node;tmp<start.node;++tmp)  {deallocate_node(*tmp);}}
        void     destroy_nodes_at_back(iterator behind_finish){for(map_pointer tmp=behind_finish.node;tmp>finish.node;--tmp){deallocate_node(*tmp);}}
    public:
        inline  iterator        begin()                      {return start;}
        inline  iterator        end()                        {return finish;}
        inline  const_iterator  begin()    const             {return start;}
        inline  const_iterator  end()      const             {return finish;}
        inline  reference       operator[](size_t n)         {return start[difference_type(n)];}
        inline  const_reference operator[](size_t n) const   {return difference_type(n);}
        inline  reference       front()                      {return *start;}
        inline  const_reference front()    const             {return *start;}
        inline  reference       back()                       {iterator tmp=finish;--tmp;return *tmp;}
        inline  const_reference back()     const             {iterator tmp=finish;--tmp;return *tmp;}  
        inline  size_t          size()                       {return finish-start;}//
        inline  size_t          max_size() const             {return size_t(-1);}
        inline  bool            empty()                      {return finish==start;}
        void push_back(const value_type &x);
        void push_front(const value_type&x);
        void pop_back();
        void pop_front();
    protected:
        void reserve_map_at_back(size_type nodes_to_add=1);
        void reallocate_map(size_type nodes_to_add,bool add_to_front);
        void reserve_map_at_front(size_type nodes_to_add=1);
    public:
        iterator insert(iterator pos,const value_type &x);
        iterator insert(iterator pos);
        iterator insert(iterator pos,T *arr,size_t n);
        iterator insert(iterator pos,size_type n,const value_type &x);
        iterator insert(iterator pos,int n,const value_type &x);
        iterator insert(iterator pos,long n,const value_type &x);
        iterator insert(iterator pos,iterator first,iterator last);
        iterator erase(iterator pos);
        iterator erase(iterator first,iterator last);
        void     clean();
        void     resize(size_t new_size,const value_type &x);
    protected:
        void pop_front_aux();
        void pop_back_aux();
        void push_front_aux(const value_type&x);  
        void push_back_aux(const value_type &x);
        iterator insert_aux(iterator pos,const value_type &x);
        iterator insert_aux(iterator pos,iterator first,iterator last,size_type n);
        void new_elements_at_front(size_t new_elements);
        void new_elements_at_back (size_t new_elements );
        void before_to_behind_n(iterator first,iterator last);
        void behind_to_before_n(iterator first,iterator last);
    public:
        bool     operator==(const deque<T,Alloc,BufSiz>&d1){return d1.size()==(*this).size() && equal(d1.begin(),d1.end(),*this.begin());}
        bool     operator<(const deque<T,Alloc,BufSiz>&du){return(this->begin(),this->end(),du.begin(),du.end());}    
};      


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::creat_map_and_nodes(size_t num_element){
    size_t num_nodes=num_element/buffer_size()+1;
    map_size=::max(initial_map_size(),num_nodes+2);
    map=map_allocator().allocate(map_size);
    map_pointer mstart=map+(map_size-num_nodes)/2;
    map_pointer mfinish=mstart+num_nodes-1;
    for(map_pointer cur=mstart;cur<=mfinish;++cur){*cur=allocate_node();}
    start.set_node(mstart);
    finish.set_node(mfinish);
    start.cur=start.first;
    finish.cur=finish.first+num_element%buffer_size();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::fill_initialize(int n,const value_type &value){
    creat_map_and_nodes(n);
    for(map_pointer cur=start.node;cur<=finish.node;++cur){
        uninitialized_fill(*cur,*cur+buffer_size(),value);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::fill_initialize(int n, value_type *arr){
    fill_initialize(n,T());
    map_pointer cur=start.node;
    T* address_1=arr;
    T* address_2=arr+buffer_size();
    for(cur=start.node;cur<=finish.node;++cur){
        uninitialized_copy(address_1,address_2,*cur);
        address_1=address_2;
        if(address_2+buffer_size()>&arr[n]) {address_2=&arr[n];}
        else                                {address_2=address_2+buffer_size();}
     }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::destroy(iterator &iter1,iterator &iter2){
    map_pointer cur=iter1.node;
    for(cur=iter1.node;cur<=iter2.node;cur++){
        while(*cur!=*cur+buffer_size()){
            _Destroy(*cur);
            ++*cur;
            }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::destroy_map_noed(){
    for(map_pointer iter=start.node;iter!=finish.node+1;++iter){
        _Destroy(iter);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::push_back(const value_type &x){
    /*当前缓冲区的空间至少有两个*/
    if (finish.cur!=finish.last-1){
        std::_Construct(finish.cur,x);
        ++finish.cur;
        }
    /*当前只剩下一个空间时会提前分配一个buffsize()大小的缓冲区*/
    else{push_back_aux(x);} 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::push_back_aux(const value_type &x){
    value_type x_copy=x;
    /*map尾后的空间不足*/
    reserve_map_at_back();          
    *(finish.node+1)=allocate_node();
    _Construct(finish.cur,x_copy);
    finish.set_node(finish.node+1);
    finish.cur=finish.first;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::reserve_map_at_back(size_t nodes_to_add){
    if(map_size-(finish.node-map)<nodes_to_add+1){reallocate_map(nodes_to_add,false);}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*nodes_to_add 为map至少增大的个数，map比原来至少大一*/
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::reallocate_map(size_type nodes_to_add,bool add_to_front){
    size_type old_num_map_nodes=finish.node-start.node+1;
    size_type new_num_map_nodes=old_num_map_nodes+nodes_to_add;
    map_pointer new_start;

    if(2*new_num_map_nodes<map_size){
        new_start=map+(map_size-new_num_map_nodes)/2+(add_to_front?nodes_to_add:0);
        if(new_start<start.node){
            uninitialized_copy(start.node,finish.node+1,new_start);
        }
        if(new_start>start.node){
            uninitialized_copy(start.node,finish.node,new_start+old_num_map_nodes);
        }
    }
    else{
        size_type new_map_size=map_size+max(map_size,nodes_to_add)+2;
        map_pointer new_map=map_allocator().allocate(new_map_size);
        new_start=new_map+(new_map_size-new_num_map_nodes)/2+(add_to_front?nodes_to_add:0);
        uninitialized_copy(start.node,finish.node+1,new_start);
        map_allocator().deallocate(map,map_size);
        map_allocator().destroy(map);
        map=new_map;
        map_size=new_map_size;
    }
    start.set_node(new_start);
    finish.set_node(new_start+old_num_map_nodes-1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::push_front(const value_type &x){
    if(start.cur!=start.first){
        _Construct(start.cur-1,x);
        --start.cur;
    }
    else{
        push_front_aux(x);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::push_front_aux(const value_type &x){
    value_type x_copy=x;
    reserve_map_at_front();
    *(start.node-1)=allocate_node();
    start.set_node(start.node-1);
    start.cur=start.last-1;
    _Construct(start.cur,x_copy);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::reserve_map_at_front(size_type nodes_to_add){
    if(start.node-map<nodes_to_add){
        reallocate_map(nodes_to_add,true);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::pop_back(){
    if(finish.cur!=finish.first){
        --finish.cur;
        _Destroy(finish.cur);
    }
    else{
        pop_back_aux();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::pop_back_aux(){
    deallocate_node(finish.first);
    finish.set_node(finish.node-1);
    finish.cur=finish.last-1;
    _Destroy(finish.cur);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::pop_front(){
    if(start.cur!=start.last-1){
        _Destroy(start.cur);
        ++start.cur;
    }
    else{
        pop_front_aux();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::pop_front_aux(){
    _Destroy(start.cur);
    deallocate_node(start.first);
    start.set_node(start.node+1);
    start.cur=start.first;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::insert(iterator pos){
    return insert(pos,value_type());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::insert(iterator pos,size_type n,const value_type &x){
    for (size_t i = 0; i < n; i++){
        insert(pos,x);
    }
    return pos;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::insert(iterator pos,int n,const value_type &x){
    return insert(pos,(size_t)n,x);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::insert(iterator pos,long n,const value_type &x){
    return insert(pos,(size_type)n,x);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::insert(iterator pos,const value_type &x){
    if(pos.cur==start.cur){
        push_front(x);
        return start;
    }
    else if(pos.cur==finish.cur){
        push_back(x);
        iterator tmp=finish;
        --tmp;
        return tmp;
    }
    else{
        return  insert_aux(pos,x);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::insert(iterator pos,T *arr,size_type n){
    size_t low=0;
    size_t high=n-1;
    T tmp;
    while(low<high){
        tmp=arr[low];
        arr[low]=arr[high];
        arr[high]=tmp;
        ++low;
        --high;
    }
    for (size_t i = 0; i < n; i++){
        insert(pos,arr[i]);
    }
    return pos;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void MENG::deque<T,Alloc,BufSiz>::copy(iterator Input_first,iterator Input_last,iterator object_first){
    map_pointer tmp=Input_last.node;
    if(Input_first.first==Input_last.first){
        uninitialized_copy(Input_first.cur,Input_last.cur,Input_first.cur+1);
    }
    else{
        uninitialized_copy(Input_last.first,Input_last.cur,Input_last.first+1);
        **tmp=*(*(tmp-1)+buffer_size()-1);
        for(tmp=Input_last.node-1;tmp!=Input_first.node;--tmp){
            uninitialized_copy(*tmp,*tmp+buffer_size()-1,*tmp+1);
            **tmp=*(*(tmp-1)+buffer_size()-1);  
        }
        uninitialized_copy(Input_first.cur,Input_first.last-1,Input_first.cur+1);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>//start pos
void MENG::deque<T,Alloc,BufSiz>::copy(iterator Input_first,iterator Input_last,iterator object_first,int i){
    size_t map_node_num=Input_last.node-Input_first.node;
    map_pointer tmp=Input_first.node+1;
    if(Input_first.cur!=Input_first.last-1){
        uninitialized_copy(Input_first.cur,Input_first.last,Input_first.cur-1);
        *(Input_first.last-1)=*(*(Input_first.node+1));
    }
    for(;tmp!=Input_last.node;++tmp){
        uninitialized_copy(*tmp+1,*tmp+buffer_size(),*tmp);
        *(*tmp+buffer_size()-1)=**(tmp+1);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::insert_aux(iterator pos,const value_type &x){
    difference_type index=pos-start;
    value_type x_copy=x;
    size_type pos_after_elements_num=finish-pos;
    size_type pos_pre_elements_num=index;
    if(pos_after_elements_num<size()/2){
        push_back(back());
        copy(pos,finish,pos+1);
        *pos=x_copy;
    }
    else{
        push_front(front());
        copy(start,pos,pos-1,0);
        *(pos-1)=x_copy;
    return pos;
 }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::insert(iterator pos,iterator first,iterator last){
    size_type n=last-first;
    value_type *arr;
    size_type i=0;
    if(last.first==first.first){
        for(auto tmp=first.cur;tmp!=last.cur;++tmp){
        arr[i++]=*tmp;
    }
        insert(pos,arr,n);
     }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::erase(iterator pos){
    iterator next=pos;
    ++next;
    difference_type index=pos-start;
    size_t map_node_1=finish.node-pos.node;
    size_t map_node_2=pos.node-start.node;
    if(index>size()/2){
        uninitialized_copy(pos.cur+1,pos.last,pos.cur);
        *(pos.last-1)=**(pos.node+1);
        for(map_pointer tmp=pos.node+1;tmp!=finish.node;tmp++){
            uninitialized_copy(*tmp+1,*tmp+buffer_size(),*tmp);
            *(*tmp+buffer_size()-1)=**(tmp+1);
        }
        uninitialized_copy(finish.first+1,finish.cur,finish.first);
        --finish;
    }
    else{
        if(pos.node==start.node){
            uninitialized_copy(start.cur,pos.cur,start.cur+1);
            pop_front();
        }
        else{
            for(map_pointer tmp=pos.node;tmp!=start.node;--tmp){
            uninitialized_copy(*tmp,*tmp+buffer_size()-1,*tmp+1);
            **tmp=*(*(tmp-1)+buffer_size()-1);
        }   
        uninitialized_copy(start.cur,start.last-1,start.cur+1);
        --start.cur;   
    }}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
_dueue_iterator<T,BufSiz> MENG::deque<T,Alloc,BufSiz>::erase(iterator first,iterator last){
    if(first==start&&last==finish){clean();}
    else{
        difference_type n = last - first;
        difference_type elems_before=first-start;
        if(elems_before<(size()-n)/2){
            before_to_behind_n(first,last);
        }
        else{
            behind_to_before_n(first,last);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void  MENG::deque<T,Alloc,BufSiz>::clean(){
    for(map_pointer tmp=start.node+1;tmp<finish.node;++tmp){
        _Destroy(*tmp,*tmp+buffer_size());
        data_allocator().deallocate(*tmp,buffer_size());
    }
    if(start.node!=finish.node){
        _Destroy(start.cur,start.last);
        _Destroy(finish.first,finish.cur);
        data_allocator().deallocate(finish.first,buffer_size());
    }
    else{
        _Destroy(start.cur,finish.cur);
    }
    finish=start;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void  MENG::deque<T,Alloc,BufSiz>::new_elements_at_front(size_t new_elements){
    size_type new_nodes=(new_elements+buffer_size()-1)/buffer_size();
    reserve_map_at_front(new_nodes);
    for (size_t i = 1; i <= new_nodes; i++){
        *(start.node-i)=allocate_node();
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void  MENG::deque<T,Alloc,BufSiz>::new_elements_at_back(size_t new_elements){
    size_type new_nodes=(new_elements+buffer_size()-1)/buffer_size();
    reserve_map_at_back(new_nodes);
    for (size_t i = 1; i <=new_nodes; i++){
        *(finish.node+i)=allocate_node();
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void  MENG::deque<T,Alloc,BufSiz>::before_to_behind_n(iterator first,iterator last){
    size_t before_buffsize_num=first.node-start.node;
    size_t element_num=last-first;
    size_t start_node_ele_num=start.last-start.cur;
    uninitialized_copy(*first.node,first.cur,*first.node+(last-first));
    for(map_pointer tmp=first.node-1;tmp!=start.node;--tmp){
        for(size_t i=0;i<element_num;i++){
            *(*(tmp+1)+i)=*(*tmp+buffer_size()-element_num+i);
        }
        uninitialized_copy(*tmp,*tmp+buffer_size()-element_num,*tmp+element_num);       
    }
    if(start_node_ele_num<element_num){
        for (size_t i = 0; i < (start.last-start.cur); i++){
        *(*(start.node+1)+element_num-i-1)=*(start.last-i-1);
        } 
        map_pointer tmp=start.node;
        start.set_node(start.node+1);
        start.cur=start.first+element_num-(start_node_ele_num);
        deallocate_node(*tmp);
        _Destroy(*tmp);
        }
    else{
        pointer tmp=start.last-element_num;
        for (size_t i = 0; i < start.last-tmp; i++){
            *(*(start.node+1)+element_num-i-1)=*(start.last-i-1);
        }
        for (pointer p=tmp-1;p!=start.cur;--p){
            *(p+3)=*p;
        }
        *(start.cur+element_num)=*(start.cur);
        start.cur=start.cur+element_num;
        for(pointer p=start.cur;p!=tmp;++p){_Destroy(p);}
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename Alloc,size_t BufSiz>
void  MENG::deque<T,Alloc,BufSiz>::behind_to_before_n(iterator first,iterator last){
    size_type element_num=last-first;
    size_type buff_num=finish.node-last.node;
    size_type behind_last_element_num=finish-last;
    size_type arr[behind_last_element_num];
    size_type finish_ele_num=finish.cur-finish.last;
    size_t i=0;
    size_t j=0;
    for(pointer p=last.cur;p!=last.last;++p){
        arr[i++]=*p;
    }
    for(map_pointer p=first.node+1;p!=finish.node;++p){
        for (size_t j = 0; j < buffer_size(); j++){
            arr[i++]=*(*p+j);
        } 
    }
    for(pointer p=finish.first;p!=finish.cur;++p){
        arr[i++]=*p;
    }
    for(pointer p=first.cur;p!=first.last;++p){
        *p=arr[j++];
    }
    for(map_pointer p=first.node+1;p!=finish.node;++p){
        for(size_t m=0;m<buffer_size();++m){
            *(*p+m)=arr[j++];
        }
    }
    for(pointer p=finish.first;p!=finish.cur;++p){
        *p=arr[j++];
    }
    if(finish_ele_num<=element_num){
        map_pointer tmp=finish.node;
        finish.set_node(finish.node-1);
        finish.cur=*(finish.node-1)+buffer_size()+(finish_ele_num)-element_num;
        deallocate_node(*tmp);
        _Destroy(*tmp);
    }
    else{
        for (size_t i = 0; i < element_num; i++){
            --finish.cur;
        }
    }
}

}
#endif