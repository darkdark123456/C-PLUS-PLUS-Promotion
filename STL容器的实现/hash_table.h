#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H




#include<memory>
#include<vector>
#include"hash_fun.h"
using std::lower_bound;
using std::size_t;
using std::allocator;
using std::uninitialized_fill_n;
using std::uninitialized_copy;
using std::_Destroy;
using std::copy;
using std::uninitialized_copy_n;
using std::swap;
using MENG::hash;
using std::pair;

namespace MENG{

template<typename Value>
struct _hashtable_node
{   _hashtable_node *next;
    Value val;
};

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
class hashtable;

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
struct _hashtable_iterator;

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
struct _hashtable_const_iterator;

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc=allocator<Value>>
struct _hashtable_iterator{
    typedef hashtable<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> hashtable;
    typedef _hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> iterator;
    typedef _hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> const_iterator;
    typedef _hashtable_node<Value> node;
    typedef Value value_type;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef Value* pointer;
    typedef Value& reference;
    node *cur;
    hashtable *ht;
    public:
        _hashtable_iterator()=default;
        _hashtable_iterator(node*x,hashtable *h):cur(x),ht(h){}
        reference operator*() const{return cur->val;}
        pointer    operator->() const{return &(cur->val);}
        iterator &operator++();
        iterator operator++(int);
        bool operator==(const iterator&it) const{return (*this).cur==it.cur;}
        bool operator!=(const iterator&it) const{return (*this).cur!=it.cur;}
};


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc=allocator<Value>>
struct _hashtable_const_iterator{
    typedef hashtable<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> hashtable;
    typedef _hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> iterator;
    typedef _hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> const_iterator;
    typedef _hashtable_node<Value> node;
    typedef Value value_type;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef const Value* pointer;
    typedef const Value& reference;
    node *cur;
    hashtable *ht;
    public:
        _hashtable_const_iterator()=default;
        _hashtable_const_iterator(const node*x,const hashtable *h):cur(x),ht(h){}
        reference operator*() const{return cur->val;}
        pointer    operator->() const{return &(cur->val);}
        const_iterator&operator++();
        const_iterator operator++(int);
        bool operator==(const_iterator&it) const{return (*this).cur==it.cur;}
        bool operator!=(const_iterator&it) const{return (*this).cur!=it.cur;}



};

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>& MENG::_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>::operator++(){
    const node*old=cur;
    cur=cur->next;
    if (!cur){
        size_type bucket=ht->bkt_num(old->val);
        for(;bucket<ht->buckets.size();++bucket){
            cur=ht->buckets[bucket];
            if (cur){
                break;
            }
        }
    }
    return *this;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> MENG::_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>::operator++(int){
    typedef typename _hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>::iterator iterator;
    iterator tmp=*this;
    ++*this;
    return tmp;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>& MENG::_hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>::operator++(){
    const node*old=cur;
    cur=cur->next;
    if (!cur){
        size_type bucket=ht->bkt_num(old->val);
        for(;bucket<ht->buckets.size();++bucket){
            cur=ht->buckets[bucket];
            if (cur){
                break;
            }
        }
    }
    return *this;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> MENG::_hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>::operator++(int){
    typedef typename _hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>::const_iterator const_iterator;
    const_iterator tmp=*this;
    ++*this;
    return tmp;
}


static const int _stl_num_primes=28;

static const unsigned long _stl_prime_list[_stl_num_primes]{
  53,         97,         193,       389,       769,
  1543,       3079,       6151,      12289,     24593,
  49157,      98317,      196613,    393241,    786433,
  1572869,    3145739,    6291469,   12582917,  25165843,
  50331653,   100663319,  201326611, 402653189, 805306457,
  1610612741, 3221225473ul, 4294967291ul

};

//////////////////////////////////////////////////////////
/*返回第一个>=n的素数*/
inline unsigned long _stl_next_prime(unsigned long n){
    const unsigned long *first=_stl_prime_list;
    const unsigned long *last=_stl_prime_list+_stl_num_primes;
    const unsigned long *pos=lower_bound(first,last,n);
    return pos==last?*(last-1) :*pos;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc=std::allocator<Value>>
class hashtable{
        typedef Key key_type;
        typedef Value value_type;
        typedef HashFuc hasher;
        typedef EqualKey key_equal;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef value_type* pointer;
        typedef value_type& reference;
        typedef const value_type&const_reference;
        typedef _hashtable_node<Value> node;
        typedef _hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> iterator;
        typedef _hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> const_iterator;
        typedef Alloc node_allocator;
        friend struct      _hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>;
        friend struct      _hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>;
        friend bool operator==(const hashtable&ht1,const hashtable&ht2){
            if(ht1.buckets.size()!=ht2.buckets.size()){
                return false;
            }
            for(size_t n=0;n<ht1.buckets.size();++n){
                node *cur1=ht1.buckets[n];
                node *cur2=ht2.buckets[n];
                for(;cur1&&cur2 && cur1->val==cur2->val;cur1=cur1->next,cur2=cur2->next){}
                if(cur1 || cur2){
                    return false;
                }
            }
            return true;
        }
    protected:
        hasher hash_funct() const {return hash;}
        key_equal key_eq() const {return equals;}
        size_t next_size(size_type n){return _stl_next_prime(n);}
        void initialize_buckets(size_t n);
        void copy_from(const hashtable &ht);
        void delete_node(node *cur);
        void clean();
        node* new_node(const value_type &x);
    private:
        hasher hash;
        key_equal equals;
        ExtractKey get_key;
        std::vector<node*,Alloc> buckets;
        size_type num_elements;
    public:
        hashtable& operator=(const hashtable&ht);
        void swap(hashtable&ht);
    public:
        hashtable(size_t n,const HashFuc& hf,const EqualKey& eql,const ExtractKey& ext):hash(hf),equals(eql),get_key(ext),num_elements(n){
            initialize_buckets(n);
        }
        hashtable(size_t n,const HashFuc& hf,const EqualKey& eql):hash(hf),equals(eql),get_key(ExtractKey()),num_elements(n){
            initialize_buckets(n);
        }
        hashtable(const hashtable&ht):hash(ht.hash),equals(ht.equals),get_key(ht.get_key),num_elements(0){
            copy_from(ht);
        }
        ~hashtable() {clean();}
    public:
        size_type size() const {return num_elements;}
        size_type max_size() const {return size_type(-1);}
        bool empty() const {return size()==0;}
        iterator begin();
        iterator end(){return iterator(nullptr,this);}
        const_iterator begin() const;
        size_type bucket_count() const{return buckets.size();}
        size_type max_bucket_count() const {return _stl_prime_list[_stl_num_primes-1];}
        size_type elems_in_buckets(size_type bucket) const;
        pair<typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator,bool>
        insert_unique(const value_type &obj);
        pair<typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator,bool>
        insert_unique_noresize(const value_type &obj);
        iterator insert_equal(const value_type &obj);
        iterator insert_equal_noresize(const value_type &obj);
        void insert_unique(iterator first,iterator last);
        void resize(size_type n);
        pair<typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator,typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator> 
        equal_range(const key_type &key);
        pair<typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::const_iterator,typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::const_iterator> 
        equal_range(const key_type &key)const;
        reference find_or_insert(const value_type &obj);
        size_type erase(const key_type&key);
        void erase(const iterator&it);
        void erase(const const_iterator& it);
        void erase(iterator first,iterator last);
        void erase(const_iterator first,const_iterator last) const;
        void erase_bucket(const size_t n,node*last);
        void erase_bucket(const size_t n,node*first,node*last);
        void erase(const_iterator first,const_iterator last);
        void swap(hashtable&ht1,hashtable&h2);
        size_type count(const key_type &key) const;
        iterator find(const key_type &key);
        const_iterator find(const key_type &key)const;
    protected:
        size_t bkt_num_key(const key_type &key,size_t n) const{return hash(key)%n;}
        size_t bkt_num_key(const key_type &key) const{return hash(key)%buckets.size();}
        size_t bkt_num(const value_type & obj ) const {return bkt_num_key(get_key(obj));}
        size_t bkt_num(const value_type & obj,size_t n ) const {return bkt_num_key(get_key(obj),n);}
};

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::initialize_buckets(size_type n){
    const size_type n_buckets=next_size(n);
    buckets.reserve(n_buckets);
    buckets.insert(buckets.end(),n_buckets,(node*)0);
    num_elements=0;
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::clean(){
    for (size_t i = 0; i < buckets.size(); i++){
        node *cur=buckets[i];
        while(cur!=nullptr){
            node *next=cur->next;
            delete_node(cur);
            cur=next;   
        }
        buckets[i]=nullptr;
    }
    num_elements=0;
    
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::copy_from(const hashtable&ht){
    buckets.clear();
    buckets.reserve(ht.buckets.size());
    buckets.insert(buckets.end(),ht.buckets.size(),(node*)0);
    //复制元素
    for (size_t i = 0; i < ht.buckets.size(); i++){
        if(const node*cur=ht.buckets[i]){
            node*copy=new_node(cur->val);
            buckets[i]=copy;
            for(node*next=cur->next;next;cur=next,next=cur->next){
                copy->next=new_node(next->val);
                copy=copy->next;
            }
        }   
    }
    num_elements=ht.num_elements;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>& MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::operator=(const hashtable&ht){
    if(&ht!=this){
        this->clean();
        this->hash=ht.hash;
        this->equals=ht.equals;
        this->get_key=ht.get_key;
        copy_form(ht);
    }
    return *this;
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void  MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::swap(hashtable&ht){
    std::swap(hash,ht.hash);
    std::swap(get_key,ht.get_key);
    std::swap(equals,ht.equals);
    std::swap(num_elements,ht.num_elements);
    buckets.swap(ht.buckets);
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::begin(){
    for (size_t i = 0; i <buckets.size(); i++){
        if(buckets[i]){
            return iterator(buckets[i],this);
        }
    }
    return end();
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>  MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::begin() const {
    for (size_t i = 0; i <buckets.size(); i++){
        if(buckets[i]){
            return const_iterator(buckets[i],this);
        }
    }
    return end();
}

/*某个篮子里元素的个数*/
template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
size_t   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::elems_in_buckets(size_type bucket)const{
    size_type sum=0;
    for(node*cur=buckets[bucket];cur;cur=cur->next){
        sum+=1;
    }
    return sum;
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
pair<typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator,bool> MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::insert_unique(const value_type  &obj){
    resize(num_elements+1);
    return insert_unique_noresize(obj);
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void  MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::resize(size_type n){
    const size_type old_n=buckets.size();
    if(num_elements>old_n){
        const size_type x=next_size(n);
        if(n>old_n){
            std::vector<node*,Alloc> tmp(n,(node*)0);
            for (size_t i = 0; i < buckets.size(); i++){
                node *first=buckets[i];
                while(first){
                    size_type new_i=bkt_num(first->val,n);
                    buckets[i]=first->next; 
                    first->next=tmp[new_i];
                    tmp[new_i]=first;
                    first=buckets[new_i];
                }
            }  
            buckets.swap(tmp);
        }
    } 
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
pair<typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator,bool> MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::insert_unique_noresize(const value_type&obj){
    const size_type pos=bkt_num(obj);
    node *first=buckets[pos];
    for(node *cur=first;cur;cur=cur->next){
        if(equals(get_key(cur->val),get_key(obj)))
             return pair<iterator,bool>(iterator(cur,this),false);
    }
    node *tmp=new_node(obj);
    tmp->next=first;
    buckets[pos]=tmp;
    ++num_elements;
    return pair<iterator,bool>(iterator(tmp,this),true);
    }


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::insert_equal(const value_type &obj){
    resize(num_elements+1);
    return insert_equal_noresize(obj);

}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator  MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::insert_equal_noresize(const value_type &obj){
    const size_type pos=bkt_num(obj);
    node *first=buckets[pos];
    for(node *cur=first;cur;cur=cur->next){
        if(equals(get_key(cur->val),get_key(obj))){
                node *tmp=new_node(obj);
                tmp->next=cur->next;
                cur->next=tmp;
                ++num_elements;
                return iterator(tmp,this);
        }
    }
    node *tmp=new_node(obj);
    tmp->next=first;
    buckets[pos]=tmp;
    ++num_elements;
    return iterator(tmp,this) ;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
Value&  MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::find_or_insert(const value_type &obj){
    resize(num_elements+1);
    size_type pos=buckets[obj];
    node *first=buckets[pos];
    for(node*cur=first;cur;cur=cur->next){
        if(equals(get_key(obj),get_key(cur->val))){
            return cur->val;
        }
    }
    node *tmp=new_node(obj);
    tmp->next=first;
    buckets[pos]=tmp;
    ++num_elements;
    return tmp->val;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
pair<typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator,typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator> MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::equal_range(const key_type &key ) {
    typedef pair<_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>,_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>> pii;
    const size_t pos=bkt_num_key(key);
    for(node *first=buckets[pos];first;first=first->next){
        if(equals(key,get_key(first->val))){
            for(node*cur=first->next;cur;cur=cur->next){
                if(!equals(get_key(cur->val,key))){
                    return pii(iterator(first,this),iterator(cur,this));
                }
            for(size_type i=pos+1;i<buckets.size();++i){
                if (buckets[i]){
                    return pii(iterator(first,this),iterator(buckets[i],this));
                }    
            }
            return pii(iterator(first,this),end());
            }
        
        }
    return iterator(end(),end());
    }
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
pair<typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::const_iterator,typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::const_iterator>MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::equal_range(const key_type &key )const {
    typedef pair<_hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>,_hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>> pii;
    const size_t pos=bkt_num_key(key);
    for(node *first=buckets[pos];first;first=first->next){
        if(equals(key,get_key(first->val))){
            for(node*cur=first->next;cur;cur=cur->next){
                if(!equals(get_key(cur->val,key))){
                    return pii(const_iterator(first,this),const_iterator(cur,this));
                }
            for(size_type i=pos+1;i<buckets.size();++i){
                if (buckets[i]){
                    return pii(const_iterator(first,this),const_iterator(buckets[i],this));
                }    
            }
            return pii(const_iterator(first,this),end());
            }
        
        }
    return const_iterator(end(),end());
    }
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
size_t  MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::erase(const key_type&key){
    const size_type n=bkt_num(key);
    node*frist=buckets[n];
    size_type erased=0;
    if(frist){
        node*cur=frist;
        node *next=frist->next;
        while (next){
            if(equals(get_key(next->val),key)){
                cur->next=next->next;
                delete_node(next);
                next=next->next;
                ++erased;
                --num_elements;
            }
            else{
                cur=next;
                next=next->next;
            }
        }
        if(equals(get_key(frist->val),key)){
            buckets[n]=frist->next;
            delete_node(frist);
            ++erased;
            --num_elements;
        }  
    }
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::erase(const iterator &it){
    if(node*p=it.cur){
        const size_type n=bkt_num(p->val);
        node*cur=buckets[n];
        if(cur==p){
            buckets[n]=cur->next;
            delete_node(cur);
            --num_elements;
        }
        else{
            node *next=cur->next;
            while(next){
                if(next==p){
                    cur->next=next->next;
                    delete_node(next);
                    --num_elements;
                    break;
                }
                else{
                    cur=next;
                    next=cur->next;
                }

            }
        }
    }
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::erase(iterator first,iterator last){
    size_type f_buckets=first.cur?bkt_num(first.cur->val):buckets.size();
    size_type l_buckets=last.cur?bkt_num(last.cur->val):buckets.size();
    if (first.cur==last.cur){
        return;
    }
    else if(f_buckets==l_buckets){
        erase_bucket(f_buckets,first.cur,last.cur);}
    else{
        erase_bucket(f_buckets,first.cur,nullptr);
        for (size_t i = f_buckets+1; i<l_buckets; i++) {
            erase(i,nullptr);
        }
        if(l_buckets!=buckets.size()){
            erase(l_buckets,last.cur);
        }
    }
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::erase_bucket(const size_type n,node*last){
    node *cur=buckets[n];
    while(cur!=last){
        node *next=cur->next;
        delete_node(cur);
        cur=next;
        buckets[n]=cur;
        --num_elements;
    }
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::erase_bucket(const size_type n,node *first,node *last){
    node *cur=buckets[n];
    if(cur==first){
        erase(n,last);
    }
    else{
        node *next;
        for(next=cur->next;next!=first;cur=next,next=cur->next);
        while(next){
            cur->next=next->next;
            delete_node(next);
            next=cur->next;
            --num_elements;
        }
    }
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::erase(const_iterator first,const_iterator last){
    erase(_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>(first.cur,first.ht),_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc>(last.cur,last.ht));
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::erase(const const_iterator &it){
    erase(iterator(it.cur,it.ht));
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::swap(hashtable&ht1,hashtable&ht2){
    ht1.swap(ht2);
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::erase(const_iterator first,const_iterator last)const{
      erase(iterator(const_cast<node*>(first.cur),const_cast<hashtable*>(first.ht)),
        iterator(const_cast<node*>(last.cur), const_cast<hashtable*>(last.ht)));
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_node<Value>* MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::new_node(const value_type &x){
    typedef typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::node_allocator node_allocator;
    node *n=node_allocator().allocate();
    n->next=nullptr;
    node_allocator().construct(&n->val,x);
    return n;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::delete_node(node *cur){
    typedef typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::node_allocator node_allocator;
    _Destroy(&cur->val);
    node_allocator().deallocate();
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
size_t  MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::count(const key_type &key) const{
    const size_type pos=bkt_num_key(key);
    size_type sum=0;
    for(const node*cur=buckets[pos];cur;cur=cur->next){
        if (equals(get_key(cur->val),key)){
            ++sum;
        }   
    }
    return sum;
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::find(const key_type &key){
    typedef typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::iterator iterator;
    size_type pos=bkt_num_key(key);
    node *first=buckets[pos];
    while (first&&!equals(get_key(first->val),key)){
        first=first->next;
    }
    return iterator(first,this);
}


template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
_hashtable_const_iterator<Value,Key,HashFuc,ExtractKey,EqualKey,Alloc> MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::find(const key_type &key)const{
    typedef typename hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::const_iterator const_iterator;
    size_type pos=bkt_num_key(key);
    node *first=buckets[pos];
    while (first&&!equals(get_key(first->val),key)){
        first=first->next;
    }
    return const_iterator(first,this);
}

template<typename Value,typename Key,typename HashFuc,typename ExtractKey,typename EqualKey,typename Alloc>
void   MENG::hashtable<Value,Key, HashFuc, ExtractKey,EqualKey, Alloc>::insert_unique(iterator first,iterator last){
    for(;first!=last;++first){
        insert_unique(*first);
    }
}

}

#endif