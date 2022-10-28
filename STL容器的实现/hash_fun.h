#ifndef _HASH_FUN_H
#define _HASH_FUNC_H
#include<string.h>
using std::string;

namespace MENG{
template<typename Key> struct hash{};

inline size_t _stl_hash_string(const char *s){
    unsigned long h=0;
    for(;*s;++s){
        h=5*h+*s;
    }
    return size_t(h);
}

inline size_t _stl_hash_string(string s){
    unsigned long h=0;
    for (size_t i = 0; i < s.length(); i++){
        h=5*h+s[i];
    }
    return size_t(h);
}

template<>
struct hash<char*>{
    size_t operator()(const char *s) const {
        return _stl_hash_string(s);
    }
};

template<>
struct hash<const char*>{
    size_t operator()(const char *s) const {
        return _stl_hash_string(s);
    }
};

template<>
struct hash<char>{
    size_t operator()( char x) const{
        return x;
    }
};

template<>
struct hash<unsigned char>{
    size_t operator()(unsigned char x) const{
        return x;
    }
};

template<>
struct hash<signed char>{
    size_t operator()(signed char x) const{
        return x;
    }
};

template<>
struct hash<short>{
    size_t operator()(short x) const{
        return x;
    }
};

template<>
struct hash<unsigned short>{
    size_t operator()(unsigned short x) const{
        return x;
    }
};

template<>
struct hash<int >{
    size_t operator()(int x) const{
        return x;
    }
};

template<>
struct hash<long>{
    size_t operator()(long x) const{
        return x;
    }
};


template<>
struct hash<unsigned long>{
    size_t operator()(unsigned long x) const{
        return x;
    }
};

template<>
struct hash<string>{
    size_t operator()(string x) const{
        return _stl_hash_string(x);
    }
};

 };
#endif