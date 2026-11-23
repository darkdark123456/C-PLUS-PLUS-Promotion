     int main(int* arg,int **args){
     
     // 推导原则一 
     // 如果是给新开辟的内存空间初始化赋值
    // 编译器希望这块内存具有较大的权限，auto会自动删除const & 这些限制属性

    // 推导原则二
    // 如果用新的引用或者指针绑定在已有的分配内存上，系统不希望它们超越原来的权限
    
    const int ci = 42;
    const int& cr = ci;

    auto a = ci;  //a 是新开辟的空间 本来是auto 本来推导为const int 但是auto删除const限制 提高其权限 auto->int 
    auto b = cr;  //b 是新开辟的空间 本来是const int& ，但是为了提高其权限，会删除const& 限制，auto->int
    const auto c = ci;// c是新开辟的空间，本来auto推导为const int 但是为了提高其权限，会删除 const限制，auto->int

    auto& d = ci; // d是绑定在ci上的引用，不会超过原有空间的权限，auto->const int
    const auto& e = ci;//e是绑定在ci上的引用，不会超过ci原有的权限，auto->int ,e是const int&

    decltype(ci) x = 10; // 什么也不会丢弃 是const int
    decltype(cr) y = x;  // 什么也不会丢弃 是const int&
    decltype((ci)) z = ci; // --->decltype(const int)--->const int&

    int pass;
    
    }
