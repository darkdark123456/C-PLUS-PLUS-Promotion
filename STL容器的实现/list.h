#ifndef _LIST_H
#define _LIST_H
#include<memory>
using std::allocator;
using std::ptrdiff_t;
using std::bidirectional_iterator_tag;
using std::_Destroy;
using std::ostream;
using std::swap;
using std::string;


namespace MENG{
template<typename T> struct _list_node;
template<typename T> class  Simple_Alloc;
template<typename T>    class _list_iterator;
template<typename T,typename Alloc>   class list;



/*node*/
template<typename T>
struct _list_node
{   T data;
    _list_node*pre=NULL;
    _list_node*next=NULL;
};



/*a simple alloctor*/
template<typename T>
class  Simple_Alloc{
    typedef _list_node<T>* link_type;
    public:
        std::allocator<T> alloc;
        link_type allocate(){return  (link_type)alloc.allocate(sizeof(_list_node<T>));}
        void deallocate(link_type p){alloc.deallocate(p,sizeof(_list_node<T>));}
        void construct(T*q,const T object){alloc.construct(q,object);}
        void destroy(link_type&p){_Destroy(p);}
        //void destroy(T*iterator1,T*iterator2){_Destroy(iterator1,iterator2);}

};




/*list iterator*/
template<typename T>
class _list_iterator{
        friend ostream&    operator<<(ostream&os,_list_iterator<T>&iter){os<<iter.node;return os;}
        typedef _list_iterator<T> iterator;
        typedef const _list_iterator<T> const_iterator;
        typedef T value_type;
        typedef T* pointer;
        typedef T& reference;
        typedef _list_node<T>* link_type;  
        typedef ptrdiff_t difference_type;
        typedef bidirectional_iterator_tag iterator_category;
    public:
        link_type node;
        _list_iterator(link_type x):node(x){}
        _list_iterator(){}
        _list_iterator(const iterator&x):node(x.node){}
    public:
        inline  bool        operator==(const iterator&x) const {return this->node==x.node;} 
        inline  bool        operator!=(const iterator&x) const {return this->node!=x.node;}
        inline  void        operator=(const  link_type&x){this->node=x;}
        inline  reference   operator*() const {return node->data;}
        inline  pointer     operator->() const{}
        inline  iterator&   operator++(){node=node->next;return *this;}  // pre
        inline  iterator    operator++(int){auto temp=*this;++*this;return temp;}    //post
        inline  iterator&   operator--(){node=node->pre;return*this;}
        inline  iterator    operator--(int){auto temp=*this;--*this;return temp;}
        inline  iterator&   operator+(T x){iterator tmp;for(T  i=1;i<=x;i++){++(*this);} return *this;}
};


/*define list*/
template<typename T,typename Alloc=std::allocator<T>>
class list{
        typedef _list_node<T> list_node;
        typedef Simple_Alloc<T> List_Node_Alloc;
        typedef T value_type;
        typedef T* pointer;
        typedef T& reference;
        typedef const T* const_pointer;
        typedef const T& const_reference;
        typedef _list_node<T>* link_type;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef _list_iterator<T> iterator;
        typedef iterator InputIterator;
        typedef const _list_iterator<T> const_iterator;
        List_Node_Alloc list_node_alloc;
        friend void quick_sort(int *r,int low,int high);
        friend int  partition();
    public:
        list();
        list(size_t n,const T&element);
        list(size_t n,const T*array);
        list(size_t n);
        ~list(){}
    protected:
        inline  link_type   initlize(size_t n,const T&element);
        inline  link_type   initlize(size_t n,const T*array);
        inline  link_type   get_node(){return list_node_alloc.allocate();}
        inline  void        put_node(link_type p){list_node_alloc.deallocate(p);}
        inline  link_type   create_node(const T& x){link_type p=get_node();list_node_alloc.construct(&p->data,x);return p;}
        inline  void        destroy_node(link_type p){list_node_alloc.destroy(p);}
    public:
        link_type node;
    protected:
        inline size_type            distance(iterator   begin,iterator    end);
        inline link_type            fill_initialize(size_t n,const T&element);
    public:
        inline bool          operator==(const list<T>&list);
        inline list<T>&      operator= (const list<T>&list);
    public:
        inline  iterator        begin()         {return node->next;}
        inline  const_iterator  begin() const   {return node->next;}
        inline  iterator        end()           {return node;}
        inline  const_iterator  end() const     {return node;}
        inline  bool            empty() const   {return node->next==node && node->pre==node;}
        inline  size_type       size()   {size_type result=distance(begin(),end());return   result;}
        inline  size_type       max_size()  const{return    size_t(-1);}
        inline  reference       front()     {return *begin();}
        inline  reference       const_front()   const {return *begin();}
        inline  reference       back()       {return*(--end());}
        inline  reference       const_back() const  {return*(--end());}
    public:
        iterator insert(iterator postion,const T&x);
        iterator insert(iterator postion){return insert(postion,T());};
        void insert(iterator postion,InputIterator first,InputIterator last);
        void insert(iterator postion,const InputIterator first,const InputIterator last,int i);
        iterator insert(iterator postion,size_t n,const T&x);
        void insert(iterator postion,int n,const T&x){insert(postion,(size_t)n,x);};
        void insert(iterator postion,long n,const T&x){insert(postion,(size_t)n,x);};
        void push_back(const T&x){insert(end(),x);};
        void push_front(const T&x){insert(begin(),x);};
        iterator erase(iterator postion);
        iterator erase(iterator first,iterator last);
        void pop_front(){erase(begin());};
        void pop_back(){erase(--end());};
        void remove(const T&x);
        void clear(){erase(begin(),end());};
        void merge(const list<T>& list);
        void reverse();
        void unique();
        void unique(int x);
        _list_iterator<T> transfer(iterator first1,iterator first2);
        void  transfer(iterator pos,iterator first,iterator last);
        void resize(size_type new_size,const T&x);
        void swap(iterator first,iterator next);
        void splice(iterator pos,list<T>&x);
        void splice(iterator pos,list<T>&x,iterator iter);
        void splice(iterator pos,list<T>&x,iterator first,iterator last);
        void q_sort();
};






/*单个节点的初始化*/
template<typename T,typename Alloc>
MENG::list<T,Alloc>::list(){
    node=get_node();
    node->data=T();
    node->next=node->pre=node;

}


/*初始化函数*/
template<typename T,typename Alloc>
_list_node<T>* MENG::list<T,Alloc>::initlize(size_t n,const T&element){
    link_type tmp;
    link_type tmp2;
    _list_node<T>* node=get_node();
    node->data=T();
    node->next=node->pre=node;
    tmp=get_node();
    tmp->data=element;
    node->next=tmp;
    tmp->pre=node;
    tmp->next=nullptr;
    for (size_t i = 2; i <= n; i++){
        tmp2=get_node();
        tmp2->data=element;
        tmp2->pre=tmp;
        tmp->next=tmp2;
        tmp=tmp2;
    }
    node->pre=tmp2;
    tmp2->next=node;

    return node;

}



/*传进一组数据变为双向链表形式*/
template<typename T,typename Alloc>
_list_node<T>* MENG::list<T,Alloc>::initlize(size_t n,const T*array){
    link_type tmp;
    link_type tmp2;
    _list_node<T>* node=get_node();
    node->data=T();
    node->next=node->pre=node;
    tmp=get_node();
    tmp->data=array[0];
    node->next=tmp;
    tmp->pre=node;
    tmp->next=nullptr;

    for (size_t i = 1; i <n; i++){
        tmp2=get_node();
        tmp2->data=array[i];
        tmp2->pre=tmp;
        tmp->next=tmp2;
        tmp=tmp2;
    }
    node->pre=tmp2;
    tmp2->next=node;
    return node;

}



/*分配n个x元素的初始化，也具有n个节点*/
template<typename T,typename Alloc>
MENG::list<T,Alloc>::list(size_t n,const T&element){
    this->node=initlize(n,element);
  }


/*分配n个节点的初始化*/
template<typename T,typename Alloc>
MENG::list<T,Alloc>::list(size_t n){
    this->node=initlize(n,T());
 }

/*给一个array初始化一个链表*/
template<typename T,typename Alloc>
MENG::list<T,Alloc>::list(size_t n,const T*array){
    this->node=initlize(n,array);

}



/*生成n个element元素的双向循环链表*/
template<typename T,typename Alloc>
_list_node<T>* MENG::list<T,Alloc>::fill_initialize(size_t n,const T&element){
return initlize(n,element);
}


/*两个迭代器之间的距离*/
template<typename T,typename Alloc>
size_t  MENG::list<T,Alloc>::distance(iterator  begin,iterator   end){
    size_type distance=0;
    for(auto iter=begin;iter!=end;iter++){distance++;}
    return distance; 
    
}


/*给定一个位置pos，在之前插入元素x，并返回当前指向x的迭代器*/
template<typename T,typename Alloc>
_list_iterator<T> MENG::list<T,Alloc>::insert(iterator postion,const T&x){
     link_type tmp=create_node(x);
     tmp->next=postion.node;
     tmp->pre=postion.node->pre;
     postion.node->pre->next=tmp;
     postion.node->pre=tmp;
     return tmp;
}


/*在pos之后插入[first，last]之间的元素*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::insert(iterator postion,InputIterator first,InputIterator last){
    for(;first!=last;first++){
        insert(postion,*first);
    }

}


/*给定一个位置在之后插入n个元素*/
template<typename T,typename Alloc>
_list_iterator<T> MENG::list<T,Alloc>::insert(iterator postion,size_t n,const T&x){
    for(size_t i=1;i<=n;i++){
        insert(postion,x);
    }

}


/*释放掉某个位置的元素*/
template<typename T,typename Alloc>
_list_iterator<T>   MENG::list<T,Alloc>::erase(iterator postion){
    link_type next_node=postion.node->next;
    link_type pre_node=postion.node->pre;
    pre_node->next=next_node;
    next_node->pre=pre_node;
    _Destroy(postion.node);
    return   next_node;
}


/*释放掉[first，last]之间的元素*/
template<typename T,typename Alloc>
_list_iterator<T>   MENG::list<T,Alloc>::erase(iterator first,iterator last){
    for(;first!=last;first++){
        link_type next_node=first.node->next;
        link_type pre_node=first.node->pre;
        pre_node->next=next_node;
        next_node->pre=pre_node;
        _Destroy(first.node);
    }
    return first;
}


/*判断两个list是否相等*/
template<typename T,typename Alloc>
bool MENG::list<T,Alloc>::operator==(const list<T>& list){
    link_type b1=(*this).node->next;
    link_type b2=list.node->next;
    link_type e1=(*this).node;
    link_type e2=list.node;
    while(b1!=e1&&b2!=e2){
        if(b1->data==b2->data){
            b1=b1->next;
            b2=b2->next;}
        else{
            return 0;
            } 
         }
    if (b1!=e1||b2!=e2)
    {   return 0;
    }
    return 1;
}


/*将list1赋值为list2*/
template<typename T,typename Alloc>
list<T>& MENG::list<T,Alloc>::operator= (const list<T>&list){
    if(*this==list){return *this;}
    else{iterator first_1=this->begin();
         iterator last_1=this->end();
         iterator first_2=list.begin();
         iterator last_2=list.end();

         while (first_1!=last_1 && first_2!=last_2)
         {
            *first_1++=*first_2++;
         }

         if (first_1==last_1){   
            insert(last_1,first_2,last_2);
         }
         if(first_2==last_2){
            erase(first_1,last_1);
         }
    }

}
/*移除掉所有x元素*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::remove(const T&x){
    iterator first=begin();
    iterator last =end();
    while (first!=last){  
        iterator next=first;
        ++next; 
        if (*first==x){   
            erase(first); 
        }
        first=next;
    } 
}

/*消除list连续相同的元素*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::unique(){
    iterator first=begin();
    iterator last=end();

    if (first==last) return;
    iterator next=first;
    while(++next!=last){
        if(*first==*next){
            erase(next);
        }
        else{
            first=next;
        }
        next=first;
    } 
}

/*消除掉所有相同的元素，并按递增顺序排列*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::unique(int x){
    q_sort();
    unique();
}

/*连接list1中first1指向的元素与list2中first2指向的元素*/
template<typename T,typename Alloc>
_list_iterator<T> MENG::list<T,Alloc>::transfer(iterator first1,iterator first2){
    iterator first_1=first1;
    iterator first_2=first2;
    iterator next=first1;
    ++next;
    first_1.node->next=first_2.node;
    first_2.node->pre=first_1.node;
    return next;
}

/*将[first，last)内的元素移动到pos请前面*/
template<typename T,typename Alloc>
void  MENG::list<T,Alloc>::transfer(iterator pos,iterator first,iterator last){
    if (pos!=last){
        last.node->pre->next=pos.node;
        first.node->pre->next=last.node;
        pos.node->pre->next=first.node;
        link_type tmp=pos.node->pre;
        pos.node->pre=last.node->pre;
        last.node->pre=first.node->pre;
        first.node->pre=tmp;
    }


}

/*两个递增list合并为一个list后依然递增*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::merge(const list<T>& list){
    iterator first_1=this->begin();
    iterator last_1=this->end();
    iterator first_2=list.begin();
    iterator last_2 =list.end();

    while (first_1!=last_1 && first_2!=last_2){
        if (*first_1<=*first_2){
            _list_iterator< T> next=transfer(first_1,first_2);
            first_1=next;
        }
        else{
            _list_iterator<T> next=transfer(first_2,first_1);
            first_2=next;
        }
        
    }
    if (first_1==last_1){
        first_1.node->pre=first_2.node;
        _Destroy(first_2.node->next);
        first_2.node->next=first_1.node;
        
    }
    if (first_2==last_2){
        first_2.node->pre=first_1.node;
        _Destroy(first_1.node->next);
        first_1.node->next=first_2.node;
        
    }
}

template<typename T,typename Alloc>
void MENG::list<T,Alloc>::swap(iterator first,iterator next){
        T tmp;
        tmp=first.node->data;
        first.node->data=next.node->data;
        next.node->data=tmp;
}

template<typename T,typename Alloc>
void MENG::list<T,Alloc>::reverse(){
    iterator first=begin();
    iterator last=--end();
    size_t n=size();

    for (size_t i = 1; i <= n/2; i++){
        swap(first++,last--);
    }
    
}

/*重新更改list的大小*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::resize(size_type new_size,const T&x){
    iterator first=begin();
    size_type len=1;
    for(;first!=end()&&len<=size();++first,++len);
    if (len==new_size){
        erase(first,end());
    }
    if(first==end()){
        insert(end(),new_size-size(),x);
    }
}

/*在pos之后插入list*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::splice(iterator pos,list<T>&list){
    iterator first=list.begin();
    iterator last=list.end();
    iterator next=transfer(pos,first);
    next.node->pre=last.node->pre;
    last.node->pre->next=next.node;
    _Destroy(last.node);
}
 
/*在pos之后插入iter指向的元素*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::splice(iterator pos,list<T>&list,iterator iter){
    iterator first=list.begin();
    iterator last=list.end();
    iterator pre_iter=iter;
    iterator next_iter=iter;
    iterator next_pos=pos;
    ++next_pos;
    --pre_iter;
    ++next_iter;
    transfer(pos,iter);
    iter.node->next=next_pos.node;
    next_pos.node->pre=iter.node;
    for(auto iter_1=pre_iter;iter_1!=list.end();--iter_1){
        _Destroy(iter_1.node);
    }
    for(auto iter_2=next_iter;iter_2!=list.end();++iter_2){
        _Destroy(iter_2.node);
    }    
}

/*在pos之后插入[first，last]之间的元素*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::splice(iterator pos,list<T>&list,iterator first,iterator last){
    iterator next_pos=pos;
    ++next_pos;
    iterator pre_first=first;
    --pre_first;
    iterator next_last=last;
    ++next_last;
    transfer(pos,first);
    transfer(last,next_pos);
    for(auto iter_1=pre_first;iter_1!=list.end();--iter_1){
        _Destroy(iter_1.node);
    }
    for(auto iter_2=next_last;iter_2!=list.end();++iter_2){
        _Destroy(iter_2.node);
    }     
}

int partition(int *r,int low,int high){
    int pivot=r[low];
    while(low<high){
        while(low<high && r[high]>pivot){
            --high;
        }
        if(low<high){
            std::swap(r[low++],r[high]);
        }
        while(low<high && r[low]<pivot){
            ++low;
        }
        if(low<high){
            std::swap(r[high--],r[low]);
        }
    }
    return high;
}

void quick_sort(int *r,int low,int high){
    int pivotpos;
    if(low<high){
        pivotpos=partition(r,low,high);
        quick_sort(r,low,pivotpos-1);
        quick_sort(r,pivotpos+1,high);
    }
}

/*对一个链表使用快速排序*/
template<typename T,typename Alloc>
void MENG::list<T,Alloc>::q_sort(){
    size_t size=this->size();
    T array[size];
    size_t  i=0;
    for(auto iter=begin();iter!=end();++iter,++i){array[i]=*iter;}
    quick_sort(array,0,size-1);
    clear();
    this->node=initlize(size,array);
    }
}

#endif