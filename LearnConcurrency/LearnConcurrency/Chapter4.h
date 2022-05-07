#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>//std::shared_ptr

namespace Note4dot1 {
	//std::mutex mut;
	//std::queue<int32_t> data_queue;//you have a queue, pass data between the two threads
	//std::condition_variable data_cond;

	//void data_preparation_thread()
	//{
	//	while (more_data_to_prepare())
	//	{
	//		int32_t const data = prepare_data();
	//		std::lock_guard<std::mutex> lk(mut);
	//		data_queue.push(data);//
	//		data_cond.notify_one();
	//	}
	//}

	//void data_processing_thread()
	//{
	//	while (true)
	//	{
	//		std::unique_lock<std::mutex> lk(mut);
	//		data_cond.wait(lk, [] { return !data_queue.empty(); });//wait the condition_variable
	//		int32_t data = data_queue.front();
	//		data_queue.pop();
	//		lk.unlock();
	//		process(data);
	//		if (is_last_chunk(data))
	//			break;
	//	}
	//}
}

namespace Note4dot2 {
	/*template<class T, class Container = std::deque<T>>
	class queue {
	public:
		explicit queue(const Container&);
		explicit queue(Container && = Container());

		template<class Alloc> explicit queue(const Alloc&);
		template<class Alloc> queue(const Container&, const Alloc&);
		template<class Alloc> queue(Container&&, const Alloc&);
		template<class Alloc> queue(queue&&, const Alloc&);

		void swap(queue& q);

		bool empty() const;
		size_t size() const;

		T& front();
		const T& front() const;

		T& back();
		const T& back() const;

		void push(const T& x);
		void push(T&& x);

		void pop();
		template<class... Args> void emplace(Args... agrs);
	};*/

	template<typename T>
	class threadsafe_queue
	{
	private:
		mutable std::mutex mut;//mutex must be mutable
		std::queue<T> data_queue;
		std::condition_variable data_cond;
	public:
		threadsafe_queue() {}
		threadsafe_queue(const threadsafe_queue& other)
		{
			std::lock_guard<std::mutex> lk(other.mut);
			data_queue = other.data_queue;
		}
		threadsafe_queue& operator=(const threadsafe_queue&) = delete;//disallow assignment for simplicity

		void push(T new_value)
		{
			std::lock_guard<std::mutex> lk(mut);
			data_queue.push(new_value);
			data_cond.notify_one();//notify
		}

		bool try_pop(T& value)
		{
			std::lock_guard<std::mutex> lk(mut);
			if (data_queue.empty())
				return false;
			value = data_queue.front();
			data_queue.pop();
			return true;
		}
		std::shared_ptr<T> try_pop()
		{
			std::lock_guard<std::mutex> lk(mut);
			if (data_queue.empty())
				return false;
			std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
			data_queue.pop();
			return res;
		}

		void wait_and_pop(T& value)
		{
			//std::unqiue_lock could release in halfway
			std::unique_lock<std::mutex> lk(mut);
			data_cond.wait(lk, [this] {return !data_queue.empty(); });
			value = data_queue.front();
			data_queue.pop();
		}

		std::shared_ptr<T> wait_and_pop()
		{
			std::unique_lock<std::mutex> lk(mut);
			data_cond.wait(lk, [this] {return !data_queue.empty(); });
			std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
			data_queue.pop();
			return res;
		}
		bool empty() const
		{
			std::lock_guard<std::mutex> lk(mut);
			return data_queue.empty();
		}
	};

	//threadsafe_queue<int32_t> data_queue;

	//void data_preparation_thread()
	//{
	//	while (more_data_to_prepare())
	//	{
	//		data_chunk const data = prepare_data();
	//		data_queue.push(data);
	//	}
	//}

	//void data_propcessing_thread()
	//{
	//	while (true)
	//	{
	//		data_chunk data;
	//		data_queue.wait_and_pop(data);
	//		process(data);
	//		if (is_last_chunk(data))
	//			break;
	//	}
	//}
}

namespace Chapter4 {
	//using namespace Note4dot2;

	void M_Test() {
		//threadsafe_queue<int32_t> data_queue;
	}
}
