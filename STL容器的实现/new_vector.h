/**This is a vector source code from GCC_2.9.1 */

#include <xmemory>
#include <memory>
#include <vector>
#include "VectorSettingSizeException.h"
#include <ostream>

using std::allocator;
using std::uninitialized_fill_n;
using std::size_t;
using std::intptr_t;
using std::copy;
using std::ostream;



template<typename T,typename Allocator=std::allocator<T>>
class Vector {

	friend std::ostream& operator<<(std::ostream& os, Vector<T>& rhs_obj) {
		for (auto iterator = rhs_obj.begin(); iterator != rhs_obj.end(); ++iterator) {

			os << *iterator << " ";

		}
		return os;
	}
	public:
		typedef T			Value_Type;
		typedef T*			Iterator;
		typedef size_t		size_uint_type;
		typedef intptr_t	size_int_type;
		typedef T&			Reference;
		typedef const T&    Const_Reference;
	private:
		Iterator			start_ ;
		Iterator			finish_;
		Iterator			last_  ;
	public:
		explicit Vector() :start_(nullptr), finish_(nullptr), last_(nullptr) {}

		explicit Vector(size_uint_type n) :Vector() {

			if (n == 0) { return; }
			fillInitialize(n);
		}

		explicit Vector(size_int_type n) :Vector() {
		
			if (n < 0)  { throw VectorSettingSizeException();}
			if (n == 0) { return; }
			fillInitialize(n);
		};

		explicit Vector(size_uint_type n, Reference elements) :Vector() {

			
			fillInitialize(n, elements);
		}

		explicit Vector(size_int_type n, Reference elements) :Vector(){
		
			
			fillInitialize(n, elements);
		}

		explicit Vector(size_uint_type n, Const_Reference elements) :Vector() {

		
			fillInitialize(n, elements);
		}

		explicit Vector(size_int_type n, Const_Reference elements) :Vector(){

			fillInitialize(n, elements);
		}

		explicit Vector(const Vector& rhs_obj) :Vector(){

			fillInitialize(rhs_obj.capcity());
			std::copy(rhs_obj.begin(), rhs_obj.end(), this->start_);
			this->last_ = this->start_ + rhs_obj.size();
		}

		explicit Vector(Vector&&) {

		}

		~Vector() { 

			free();
		}
	private:
		template<typename N_SIZE_TYPE>
		void   fillInitialize(N_SIZE_TYPE n) {
		
			this->start_  = Allocator().allocate(n);
			this->last_   = this->start_;
			this->finish_ = this->start_ + n;
		}

	template<typename N_SIZE_TYPE,typename DATA_TYPE>
	void fillInitialize(N_SIZE_TYPE n, DATA_TYPE elements) {

			this->start_ = Allocator().allocate(n);
			std::uninitialized_fill_n(this->start_, n, elements);
			this->finish_ = this->start_ + n;
			this->last_ = this->finish_;
		}

		void   free() {

			if (this->capcity() == 0) {

				return;
			}

			if (this->size() == 0) {

				Allocator().deallocate(this->start_, this->finish_-this->start_);
			}
			else
			{
				std::_Destroy_range(this->start_, this->last_);
				Allocator().deallocate(this->start_, this->finish_-this->start_);
			}
		}
	public:
		inline size_uint_type  size()	 const   { return last_ - start_; }
		inline size_uint_type  capcity() const   { return finish_ - start_; }
		inline Iterator        begin()   const   { return start_; }
		inline Iterator        end()     const   { return last_; }
		inline Reference       front()	 const   { return *start_; }
		inline Reference       back()    const   { return *(last_-1); };
	public:
		inline void			   push_back(const T& element) { insert(element); }
		inline void            pop()					   { erase(*(this->last_ - 1)); }
	private:
		void insertAuxiliary(Iterator iterator, const T& element) {
			size_uint_type old_size = this->size();
			size_uint_type factor   = 2;
			size_uint_type new_size = (old_size == 0) ? 2 : factor * old_size;

			Iterator new_start = std::allocator<T>().allocate(new_size);
			Iterator new_last, new_finish;
			new_finish = new_last = new_start;

			Iterator new_start_copy = new_start;
			if (this->size() != 0) {

				std::copy(this->start_, this->last_, new_start_copy);
				new_finish = new_start + new_size;
				new_last   = new_start + old_size;
				free();
			}

			std::allocator<T>().construct(new_last++, element);
			this->start_  = new_start;
			this->last_   = new_last;
			this->finish_ = new_finish;
		}
	public:
		inline void insert(const T& element) {
			if (this->last_ != this->finish_) {
				std::allocator<T>().construct(this->last_++, element);
			}
			else
			{
				insertAuxiliary(this->last_, element);
			}
		}

		inline void insert(Iterator postion, const T& element) {
			if (postion != this->last_ ) { return; }
			if (postion != this->finish_) {

				std::allocator<T>().construct(this->last_++, element);
			}
			else
			{
				insertAuxiliary(this->last_, element);
			}
		}

		inline void insert(Iterator postion,size_int_type n,const T& element) {

			for (; n > 0; --n) {
				insert(postion++, element);
			}
		}

		inline void insert(Iterator firstIterator, Iterator lastIterator) {

		}
		public:
			void erase(Iterator postion) {

				if (postion == this->finish_) { return; }
				
				if (postion == (this->last_-1)) {

					std::_Destroy_in_place(--(this->last_));
				}
				else
				{
					std::copy(postion + 1, last_, postion);
					--this->last_;
					std::_Destroy_in_place(last_);
				}
			}

			void erase(Iterator firstIterator, Iterator lastIterator) {

				auto move_n_steps = lastIterator - firstIterator;
				if ( move_n_steps== 0) { return; }
				std::copy(lastIterator, this->last_, firstIterator);
				std::_Destroy_range(firstIterator + move_n_steps, this->last_);
				this->last_ = firstIterator + move_n_steps;
			}
};
