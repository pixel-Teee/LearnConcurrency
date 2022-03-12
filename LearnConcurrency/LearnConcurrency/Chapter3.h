#pragma once
#include <list>
#include <mutex>
#include <algorithm>
#include <deque>
#include <stack>

namespace Note3dot1 {
	std::list<int> some_list;
	std::mutex some_mutex;

	void add_to_list(int new_value)
	{
		std::lock_guard<std::mutex> guard(some_mutex);
		some_list.push_back(new_value);
	}

	bool list_contains(int value_to_find)
	{
		std::lock_guard<std::mutex> guard(some_mutex);
		return std::find(some_list.begin(), some_list.end(), value_to_find) != some_list.end();
	}
}

namespace Note3dot2 {
	class some_data
	{
		int a;
		std::string b;
	public:
		void do_something();
	};

	class data_wrapper
	{
		private:
			some_data data;
			std::mutex m;
		public:
			template<typename Function>
			void process_data(Function func)
			{
				std::lock_guard<std::mutex> l(m);
				func(data);
			}
	};

	some_data* unprotected;

	void malicious_function(some_data& protected_data)
	{
		unprotected = &protected_data;
	}

	data_wrapper x;

	void foo()
	{
		x.process_data(malicious_function);
		//unprotected->do_something();
	}
}

namespace Note3dot3 {
	//template<typename T, typename Container = std::deque<T>>
	//class stack
	//{
	//	public:
	//		explicit stack(const Container&);
	//		explicit stack(Container&& = Container());
	//		template<class Alloc> explicit stack(const Alloc&);
	//		template<class Alloc> stack(const Container&, const Alloc&);
	//		template<class Alloc> stack(Container&&, const Alloc&);
	//		template<class Alloc> stack(stack&&, const Alloc&);

	//		//empty and size can't be relied on
	//		bool empty() const;
	//		size_t size() const;
	//		T& top();
	//		T const& top() const;
	//		void push(T const&);
	//		void push(T&&);
	//		void pop();
	//		void swap(stack&&);
	//};
}

namespace Note3dot4 {
	//implements option 1 and 3
	struct empty_stack : std::exception
	{
		const char* what() const throw(){ return "is empty"; }
	};

	template<typename T>
	class threadsafe_stack
	{
	private:
		std::stack<T> data;
		mutable std::mutex m;
	public:
		threadsafe_stack(){};
		threadsafe_stack(const threadsafe_stack& other)
		{
			//copy performed in constructor body
			std::lock_guard<std::mutex> lock(other.m);
			data = other.data;
		}
		threadsafe_stack& operator=(const threadsafe_stack&) = delete;

		void push(T new_value)
		{
			std::lock_guard<std::mutex> lock(m);
			data.push(new_value);
		}
		std::shared_ptr<T> pop()
		{
			std::lock_guard<std::mutex> lock(m);
			if(data.empty()) throw empty_stack();//check for empty before trying to pop value
			std::shared_ptr<T> const res(std::make_shared<T>(data.top()));//allocate return value before modifying stack
			data.pop();
			return res;
		}
		void pop(T& value)
		{
			std::lock_guard<std::mutex> lock(m);
			if(data.empty()) throw empty_stack();
			value = data.top();
			data.pop();
		}
		bool empty() const 
		{
			std::lock_guard<std::mutex> lock(m);
			return data.empty();
		}
	};
}

namespace Note3dot6 {
	/*class some_big_object;

	void swap(some_big_object& lhs, some_big_object& rhs);

	class X
	{
	private:
		some_big_object some_detail;
		std::mutex m;
	public:
		X(some_big_object const& sd) : some_detail(sd){}

		friend void swap(X& lhs, X& rhs)
		{
			if (&lhs == &rhs)
				return;
			std::lock(lhs.m, rhs.m);
			std::lock_guard<std::mutex> lock_a(lhs.m, std::adopt_lock);
			std::lock_guard<std::mutex> lock_b(rhs.m, std::adopt_lock);

			swap(lhs.some_detail, rhs.some_detail);
		}
	};*/
}

namespace Note3dot7 {
	class hierarchical_mutex
	{
		std::mutex internal_mutex;
		unsigned long const hierarchy_value;
		unsigned long previous_hierarchy_value;
		//使用this_thread_hierarchy_value表示现在的线程的层级值，它被ULONG_MAX初始化，因此初始地时候，任何mutex都可以被锁定
		//因为它被定义为ithread_local，每个线程都有自己的拷贝
		static thread_local unsigned long this_thread_hierarchy_value;

		void check_for_hierarchy_violation()
		{
			if (this_thread_hierarchy_value <= hierarchy_value)
			{
				throw std::logic_error("mutex hierarchy violated");
			}
		}

		void update_hierarchy_value()
		{
			previous_hierarchy_value = this_thread_hierarchy_value;
			this_thread_hierarchy_value = hierarchy_value;
		}
	public:
		explicit hierarchical_mutex(unsigned long value) : 
		hierarchy_value(value),
		previous_hierarchy_value(0){}

		void lock()
		{
			check_for_hierarchy_violation();
			internal_mutex.lock();
			update_hierarchy_value();
		}

		void unlock()
		{
			this_thread_hierarchy_value = previous_hierarchy_value;
			internal_mutex.unlock();
		}

		bool try_lock()
		{
			check_for_hierarchy_violation();
			if (!internal_mutex.try_lock())
			{
				return false;
			}
			update_hierarchy_value();
			return true;
		}
	};

	thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);

	//using a lock hierarchy to prevent deadlock

	class hierarchical_mutex;

	hierarchical_mutex high_level_mutex(10000);
	hierarchical_mutex low_level_mutex(5000);

	int do_low_level_stuff();

	int low_level_func()
	{
		std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
		return do_low_level_stuff();
	}

	void high_level_stuff(int some_param);

	void high_level_func()
	{
		std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
		high_level_stuff(low_level_func());
	}

	void thread_a()
	{
		high_level_func();
	}

	hierarchical_mutex other_mutex(100);
	void do_other_stuff();

	void other_stuff()
	{
		high_level_func();
		do_other_stuff();
	}

	void thread_b()
	{
		std::lock_guard<hierarchical_mutex> lk(other_mutex);
		other_stuff();
	}
}

namespace Chapter3 {
	using namespace Note3dot4;
	void M_Test() {
		threadsafe_stack<int> s;
		
		try 
		{
			s.pop();
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
}