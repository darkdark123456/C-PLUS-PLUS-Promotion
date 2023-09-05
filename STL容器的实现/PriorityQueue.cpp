#include  <iostream>
#include  <algorithm>
#include  <vector>


template <typename T,typename Sequence=std::vector<T>,typename Compare=std::less<typename Sequence::value_type>>
class PriorityQueue{
    public:
        typedef typename Sequence::value_type value_type;
        typedef typename Sequence::size_type size_type;
        typedef typename Sequence::reference reference;
        typedef typename Sequence::const_reference const_reference;
    private:
        Sequence sequence; // 定义一个容器对象
        Compare compare;   // 定义一个函数对象
    public:
        PriorityQueue() : Sequence() {}
        
        explicit PriorityQueue(const Compare& _compare) : Sequence(),compare(_compare) {}

        template<typename InputIterator> // 这个模板参数会在构造的时候根据迭代器的类型自动进行实例化
        PriorityQueue(InputIterator first,InputIterator last) : sequence(first,last){
            std::make_heap(sequence.begin(),sequence.end());

        }

    public:
        inline bool empty() const {
            return sequence.empty();
        }

        inline size_type size() const {
            return sequence.size();
        }

        const_reference top() const{
            return sequence.front();
        }

        void push(const value_type& x){
            sequence.push_back(x);
            std::push_heap(sequence.begin(),sequence().end(),compare);
        }

        void pop() {
            std::pop_heap(sequence.begin(),sequence.end()); // 在树上看将堆顶元素和最下面的叶子节交换，这个堆顶元素被弃用 ，
                                                            // 在顺序容器上看 堆顶元素被放到最后面被弃用  ，之后将剩下的元素重新排列成堆 
            sequence.pop_back(); // 将被弃用的元素弹出
        }



};


int main(){
    int a[10]={0,1,2,3,4,8,9,3,5};
    PriorityQueue<int> priorityQueue(a,a+9);

    std::cout<<priorityQueue.size()<<std::endl;

    while (!priorityQueue.empty()){
        auto& value=priorityQueue.top();
        std::cout<<value<<" ";
        priorityQueue.pop();
    }

    std::cout<<std::endl;
    std::cout<<priorityQueue.size();
    
     return 1;
}
