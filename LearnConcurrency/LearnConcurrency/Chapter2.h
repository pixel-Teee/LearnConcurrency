#pragma once

#include <vector>
#include <iterator>
#include <thread>
#include <numeric>
#include <functional>
#include <algorithm>
#include <mutex>
#include <map>

namespace Note2dot8 {
	template<typename Iterator, typename T>
	struct accumulate_block
	{
		void operator()(Iterator first, Iterator last, T& result)
		{
			result = std::accumulate(first, last, result);
		}
	};

	template<typename Iterator, typename T>
	T parallel_accumulate(Iterator first, Iterator last, T init)
	{
		unsigned long const length = std::distance(first, last);

		//if length == 0, return init
		if(!length)
			return init;

		unsigned long const min_per_thread = 25;
		unsigned long const max_threads = 
			(length + min_per_thread - 1) / min_per_thread;

		unsigned long const hardware_threads = 
			(length + min_per_thread - 1) / min_per_thread;

		unsigned long const num_threads = 
			std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

		//number of elements processed per thread
		unsigned long const block_size = length / num_threads;

		std::vector<T> results(num_threads);

		//you already have one main thread
		std::vector<std::thread> threads(num_threads - 1);

		Iterator block_start = first;

		for (unsigned long i = 0; i < (num_threads - 1); ++i)
		{
			Iterator block_end = block_start;

			std::advance(block_end, block_size);

			threads[i] = std::thread(
				accumulate_block<Iterator, T>(),
				block_start, block_end, std::ref(results[i])
			);

			block_start = block_end;
		}

		//let the main thread process the final block
		accumulate_block<Iterator, T>()(
			block_start, last, results[num_threads - 1]);
		
		//join
		std::for_each(threads.begin(), threads.end(),
			std::mem_fn(&std::thread::join));

		return std::accumulate(results.begin(), results.end(), init);
	}
}

namespace Note3do2dot6 {
	//class some_big_object;
	//class X
	//{
	//private:
	//	some_big_object some_detail;
	//	std::mutex m;
	//public:
	//	X(some_big_object const& sd) : some_detail(sd){}

	//	friend void swap(X& lhs, X& rhs)
	//	{
	//		if(&lhs == &rhs)
	//			return;
	//		std::unique_lock<std::mutex> lock_a(lhs.m, std::defer_lock);
	//		std::unique_lock<std::mutex> lock_b(rhs.m, std::defer_lock);//leaves mutexes unlocked
	//		std::lock(lock_a, lock_b);//mutexes are locked here
	//		swap(lhs.some_detail, rhs.some_detail);
	//	}
	//};
}

namespace Note3dot10
{
	class Y
	{
		private:
			int some_detail;
			mutable std::mutex m;

			int get_detail() const
			{
				std::lock_guard<std::mutex> lock_a(m);
				return some_detail;
			}
		public:
			Y(int sd) : some_detail(sd){}

			friend bool operator==(Y const& lhs, Y const& rhs)
			{
				if(&lhs == &rhs)
					return true;
				int const lhs_value = lhs.get_detail();
				int const rhs_value = rhs.get_detail();
				return lhs_value == rhs_value;
			}
	};
}

namespace Note3dot11 {
	//class some_resource;
	//std::shared_ptr<some_resource> resoure_ptr;

	//void foo()
	//{
	//	if (!resoure_ptr)
	//	{
	//		resoure_ptr.reset(new some_resource);//there
	//	}
	//	resoure_ptr->do_something();
	//}

	class some_resource;
	std::shared_ptr<some_resource> resource_ptr;
	std::mutex resource_mutex;
	//void foo()
	//{
	//	std::unique_lock<std::mutex> lk(resource_mutex);//all threads are serailizaed here
	//	if (!resource_ptr)
	//	{
	//		resource_ptr.reset(new some_resource);//only the initialization needs protection
	//	}
	//	lk.unlock();
	//	resource_ptr->do_something();
	//}

	
	//void undefined_behaviour_with_double_checked_locking()
	//{
	//	if (!resource_ptr)//1
	//	{
	//		std::lock_guard<std::mutex> lk(resource_mutex);
	//		if (!resource_ptr)//2
	//		{
	//			resource_ptr.reset(new some_resource);//3
	//		}
	//	}
	//	resource_ptr->do_something();//4
	//}

	//std::shared_ptr<some_resource> resource_ptr;
	//std::once_flag resource_flag;//1

	//void init_resource()
	//{
	//	resource_ptr.reset(new some_resource);
	//}

	//void foo()
	//{
	//	std::call_once(resource_flag, init_resource);//initialization is called exactly once
	//	resource_ptr->do_something();
	//}
}

namespace Note3dot12 {
	//class connection_info;
	//class connection_handle;
	//class X
	//{
	//	private:
	//		connection_info connection_details;
	//		connection_handle connection;

	//		std::once_flag connection_init_flag;

	//		void open_connection()
	//		{
	//			connection = connection_manager.open(connection_details);
	//		}
	//	public:
	//		X(connection_info const& connection_details_) :
	//			connection_details_(connection_details_) {

	//		}
	//		void send_data(data_packet const& data)//1
	//		{
	//			std::call_once(connection_init_flag, &X::open_connection, this);//2
	//			connection.send_data(data);
	//		}
	//		data_packet receive_data()//3
	//		{
	//			std::call_once(connection_init_flag, &X::open_connection, this);//2
	//			return connection.receive_data();
	//		}
	//};
}

namespace Note3dot12 {
	//class dns_entry;

	//class dns_cache
	//{
	//	std::map<std::string, dns_entry> entries;
	//	mutable boost::shared_mutex entry_mutex;

	//public:
	//	dns_entry find_entry(std::string const& domain) const
	//	{
	//		boost::shared_lock<boost::shared_mutex> lk(entry_mutex);//1
	//		std::map<std::string, dns_entry>::const_iterator const it = 
	//			entries.find(domain);
	//		return (it == entries.end()) ? dns_entry() : it->second;
	//	}

	//	void update_or_add_entry(std::string const& domain, dns_entry const& dns_details)
	//	{
	//		std::lock_guard<boost::shared_mutex> lk(entry_mutex);//2
	//		entries[domain] = dns_details
	//	}
	//};
}

namespace Chapter2 {
	using namespace Note2dot8;

	void M_Test() {
		std::vector<int> v;

		for (int i = 0; i < 101; ++i) v.push_back(i);

		std::cout << Note2dot8::parallel_accumulate(v.begin(), v.end(), 0) << std::endl;
	}
}
