#include <iostream>
#include <iterator>
#include <vector>


/**
 * @param first      指向首元素的迭代器
 * @param hole_index 当前调整的元素的位置
 * @param len        堆的大小
 * @param value      当前调整的元素的值
*/
template<typename RandomAccessIterator, typename T, typename Distance>
void _adjust_heap(RandomAccessIterator first, Distance hole_index, Distance len, T value) {

    std::cout << "目前调整的元素的index位于向量 " << hole_index << std::endl;
    std::cout << "堆的大小 " << len << std::endl;
    std::cout << "目前调整的元素 " << value << std::endl;
}


template<typename RandomAccessIterator, typename T, typename Distance>
inline void _make_heap(RandomAccessIterator first, RandomAccessIterator last) {
    if (last - first < 2) { return; }
    Distance length = last - first;
    Distance parent = (length - 2) / 2;
    _adjust_heap(first, parent, length, T(*(first + parent)));
}


template<typename T>
void swap(T& a, T& b) {
    T tmp = a;
    a = b;
    b = tmp;
}

template<typename RandomAccessIterator, typename T, typename Distance>
inline void adjust_local_heap(RandomAccessIterator first, RandomAccessIterator last) {


}


template<typename RandomAccessIterator>
inline void make_heap(RandomAccessIterator first, RandomAccessIterator last) {

    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T_;
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type Distance_;
    if (last - first < 2) { return; }
    Distance_ length = last - first;
    Distance_ adjust_element_index = (length - 2) / 2;
    Distance_ right_child_index = adjust_element_index * 2 + 2;
    T_ max_value;
    T_ current_value;


    //1 对于第一次计算 当前要调整的节点必定含左孩子 先进行一次局部调整
    /**当前要调整的元素有右孩子*/
    if (right_child_index < length) {
        max_value = std::max(*(first + right_child_index), *(first + right_child_index - 1));
        current_value = *(first + adjust_element_index);
        if (current_value < max_value) {
            if (*(first + right_child_index) >= *(first + right_child_index - 1)) {

                ::swap(*(first + adjust_element_index), *(first + right_child_index));
            }
            else {
                ::swap(*(first + adjust_element_index), *(first + right_child_index - 1));
            }

        }

    }

    /**当前调整的只有左孩子*/
    else if (right_child_index - 1 < length) {
        max_value = *(first + right_child_index - 1);
        if (*(first + adjust_element_index) < max_value) {

            ::swap(*(first + adjust_element_index), *(first + right_child_index - 1));
        }
    }
    /**什么也不用做*/
    else {

    }

    //! 经过初步调整 
    std::cout << "第一次调整后" << std::endl;
    for (auto iter = first; iter != last; ++iter) {

        std::cout << *iter << " ";
    }

    --adjust_element_index;
    for (int current_index = adjust_element_index; current_index >= 0; current_index--) { /*往前需要调整的序列号*/
        adjust_element_index = current_index; /*目前需要调整的序列号*/
        while (adjust_element_index < length) { /*对于每一个需要调整的元素从上到下的调整*/
            right_child_index = adjust_element_index * 2 + 2;
            if (right_child_index < length) {
                max_value = std::max(*(first + right_child_index), *(first + right_child_index - 1));
                current_value = *(first + adjust_element_index);
                if (current_value < max_value) {
                    if (*(first + right_child_index) >= *(first + right_child_index - 1)) {

                        ::swap(*(first + adjust_element_index), *(first + right_child_index));
                        adjust_element_index = right_child_index;
                    }
                    else {
                        ::swap(*(first + adjust_element_index), *(first + right_child_index - 1));
                        adjust_element_index = right_child_index - 1;
                    }
                }
            }

            /**当前调整的只有左孩子*/
            else if (right_child_index - 1 < length) {
                max_value = *(first + right_child_index - 1);
                if (*(first + adjust_element_index) < max_value) {

                    ::swap(*(first + adjust_element_index), *(first + right_child_index - 1));
                    adjust_element_index = right_child_index - 1;
                }
            }
            /**什么也不用做*/
            else {

                break;
            }

        }

        std::cout << std::endl;
        for (auto iter = first; iter != last; ++iter) {

            std::cout << *iter << " ";
        }

    }

}

template<typename RandomAccessIterator>
void pop_heap(RandomAccessIterator first,RandomAccessIterator last) {
    
    if (first == last) {

        return;
    }

    if (last- first<= 1) { return; } /*heap里只有1个元素 只需返回 在pop_heap函数后面获得容器的元素 并弹出*/
    //将待弹出的元素放入到最后面
    ::swap(*first, *(last-1));
    
    //只需调整[first,last-1]为一个新堆即可，只有首部的堆的局部性被破坏了
   int  adjust_element_index = 0;
   int length = last- first-1;
   int right_child_index;
   int max_value;

   while (adjust_element_index<length){
       right_child_index = 2 * adjust_element_index + 2;
        if (right_child_index < length) {
            max_value = std::max(*(first + right_child_index), *(first+right_child_index - 1));
            if (*(first + adjust_element_index) < max_value) {

                if (*(first + right_child_index) >= *(first + right_child_index - 1)) {
                    ::swap(*(first+right_child_index), *(first+adjust_element_index));
                    adjust_element_index = right_child_index;
                }
                else
                {
                    ::swap(*(first+right_child_index-1), *(first+adjust_element_index));
                    adjust_element_index = right_child_index - 1;
                }
            }

            else
            {
                break;
            }


        }

        else if (right_child_index - 1 < length) {
            if (*(first + right_child_index - 1) > *(first + adjust_element_index)) {

                ::swap(*(first + right_child_index - 1), *(first + adjust_element_index));
                adjust_element_index = right_child_index - 1;
            }

            else
            {
                break;
            }

        }

        else
        {
            break;
        }

    }

   std::cout << std::endl;
   std::cout << "排序结果" << std::endl;
   for (auto iter = first; iter != last; iter++) {

       std::cout << *iter << " ";
   }
}



template<typename RandomAccessIterator>
void heap_sort(RandomAccessIterator first,RandomAccessIterator last) {

    auto last_ = last;
    auto length = last - first;
    pop_heap(first, last--);
    pop_heap(first, last--);
    pop_heap(first, last--);
    pop_heap(first, last--);
    pop_heap(first, last--);
    pop_heap(first, last--);
    pop_heap(first, last--);
    pop_heap(first, last--);
    pop_heap(first, last--);

}



int main() {
    int array[9] = { 0,1,2,3,4,8,9,3,5 };
    std::vector<int> vec(array, array + 9);
    make_heap(vec.begin(), vec.end());
    std::cout << std::endl;
    std::cout << "******************" << std::endl;


    heap_sort(vec.begin(), vec.end());

    return 1;
}
