#include<memory>
#include"hash_table.h"
using std::allocator;
using std::_Identity;
using std::equal_to;
using MENG::hashtable;
using std::_Select1st;
using MENG::hash;

template <typename Key, typename T, typename HashFcn=hash<Key>, typename EqualKey=equal_to<Key>, typename Alloc = std::allocator<Key>>
class hash_map{
    typedef MENG::hashtable<pair<const Key, T>, Key, HashFcn,_Select1st<pair<const Key, T> >, EqualKey, Alloc> ht;
    private:
        ht rep;
    public:
        typedef typename ht::key_type key_type;
        typedef T data_type;
        typedef T mapped_type;
        typedef typename ht::value_type value_type;
        typedef typename ht::hasher hasher;
        typedef typename ht::key_equal key_equal;
        typedef typename ht::size_type size_type;
        typedef typename ht::difference_type difference_type;
        typedef typename ht::pointer pointer;
        typedef typename ht::const_pointer const_pointer;
        typedef typename ht::reference reference;
        typedef typename ht::const_reference const_reference;
        typedef typename ht::iterator iterator;
        typedef typename ht::const_iterator const_iterator;
    public:
        hasher hash_funct() const { return rep.hash_funct(); }
        key_equal key_eq() const { return rep.key_eq(); }
    public:
        hash_map() : rep(100, hasher(), key_equal()) {}
        explicit hash_map(size_type n) : rep(n, hasher(), key_equal()) {}
        hash_map(size_type n, const hasher& hf) : rep(n, hf, key_equal()) {}
        hash_map(size_type n, const hasher& hf, const key_equal& eql): rep(n, hf, eql) {}
    public:
        template <typename InputIterator>hash_map(InputIterator f, InputIterator l): rep(100, hasher(), key_equal()) { rep.insert_unique(f, l); }
        template <typename InputIterator>hash_map(InputIterator f, InputIterator l, size_type n): rep(n, hasher(), key_equal()) { rep.insert_unique(f, l); }
        template <typename InputIterator>hash_map(InputIterator f, InputIterator l, size_type n,const hasher& hf): rep(n, hf, key_equal()) { rep.insert_unique(f, l); }
        template <typename InputIterator>hash_map(InputIterator f, InputIterator l, size_type n,const hasher& hf, const key_equal& eql): rep(n, hf, eql) { rep.insert_unique(f, l); }
    public:
        size_type size() const { return rep.size(); }
        size_type max_size() const { return rep.max_size(); }
        bool empty() const { return rep.empty(); }
        void swap(hash_map& hs) { rep.swap(hs.rep); }
        friend bool operator==(const hash_map&h1, const hash_map&h2){return h1.rep==h2.rep;}
        iterator begin() { return rep.begin(); }
        iterator end() { return rep.end(); }
        const_iterator begin() const { return rep.begin(); }
        const_iterator end() const { return rep.end(); }
    public:
        pair<iterator, bool> insert(const value_type& obj){ return rep.insert_unique(obj); }
        template <typename InputIterator> void insert(InputIterator f, InputIterator l) { rep.insert_unique(f,l); }
        pair<iterator, bool> insert_noresize(const value_type& obj){ return rep.insert_unique_noresize(obj); } 
        iterator find(const key_type& key) { return rep.find(key); }
        const_iterator find(const key_type& key) const { return rep.find(key); }
        T& operator[](const key_type& key) {return rep.find_or_insert(value_type(key, T())).second;}
        size_type count(const key_type& key) const { return rep.count(key); }
        pair<iterator, iterator> equal_range(const key_type& key){ return rep.equal_range(key); }
        pair<const_iterator, const_iterator> equal_range(const key_type& key) const{ return rep.equal_range(key); }
        size_type erase(const key_type& key) {return rep.erase(key); }
        void erase(iterator it) { rep.erase(it); }
        void erase(iterator f, iterator l) { rep.erase(f, l); }
        void clear() { rep.clear(); }
        void swap(hash_map<Key, T, HashFcn, EqualKey, Alloc>& hm1,hash_map<Key, T, HashFcn, EqualKey, Alloc>& hm2){hm1.swap(hm2);}
};
int main(){
    system("pause");
    return 1;
}