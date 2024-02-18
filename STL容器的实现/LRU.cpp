 #include <iostream>
#include <unordered_map>
#include <list>

/**
 * 通过一个哈希表和一个优先级列表来模拟LRU算法 
 * 通过更新优先级列表来实现元素的访问的最新情况，访问的新旧自左至右一次递减
*/


class LRU_Cache{
private:
    int capacity;
    std::list<std::pair<char*,int>> lru_list;
    std::unordered_map<char*,int> cache;
public:
    LRU_Cache():capacity(0){}
    LRU_Cache(const int& capacity_) :capacity(capacity_){}
    ~LRU_Cache(){}
    LRU_Cache(const LRU_Cache& rhs)=delete;
    LRU_Cache(LRU_Cache&& rhs)=delete;
public:
    void put(char* name,int school_number){
        std::unordered_map<char*,int>::iterator iter=cache.find(name);
        if(iter!=cache.end()){             // 如果put的元素的key已经在cache里了 那么直接更新它就好了
            
            iter->second=school_number;                         //  更新key对应的value
            auto   postion=find_element(name);                 //  更新优先级列表 将<key,value>组成的元素提到最前面
            auto   before_postion=postion;                    //   记住前一个指针
            std::advance(postion,1);                          //   往后移
            lru_list.splice(lru_list.begin(),lru_list,before_postion,postion); //将刚访问的元素放到最前面
        }
        else{                                                // key 不在cache里 
            if(cache.size()>=capacity){                     // 开始置换
                char* key_to_remove=lru_list.back().first; // 寻找到优先级最小的key
                cache.erase(key_to_remove);               // 从cache中移除<key,value>
                lru_list.pop_back();                     // 从优先级列表里删除最近使用次数最少的元素
            }
            lru_list.push_front(std::make_pair(name,school_number));
            cache[name]=school_number;
        }
    }
public:
    std::list<std::pair<char*,int>>::iterator find_element(char* key){
    for(auto iter=lru_list.begin();iter!=lru_list.end();++iter){

        if(iter->first==key){

            return iter; }
    }
    return lru_list.end();
    }
public:
    int get(char* key){
        auto iterator=cache.find(key);
        if(iterator!=cache.end()){
            // 最新的元素被访问了 将它置于前面
            // 返回key对应的元素
            auto postion=find_element(key);
            auto   before_postion=postion;                    //模拟指针++
            std::advance(postion,1);                          
            lru_list.splice(lru_list.begin(),lru_list,before_postion,postion); // 将最新的元素置于前面 
        }
        return -1;
    }
};




int main(){


    return ;
}
