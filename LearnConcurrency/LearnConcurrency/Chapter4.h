#pragma once
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <memory>//std::shared_ptr
#include <future>

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

	//using std::future to get the return value of an asynchronous task

	int find_the_anwser_to_ltuae();

	void do_other_stuff();

	void test();
}

namespace Note4dot2dot1 {
	//struct X
	//{
	//	void foo(int, std::string const&);
	//	std::string bar(std::string const&);
	//};
	//X x;
	//auto f1 = std::async(&X::foo, &x, 42, "hello");//calls p->foo(42, "hello"), where p is &x
	//auto f2 = std::async(&X::bar, x, "goodbye");//calls tmpx.bar("goodbye"), where tmpx is a copy of x
	//struct Y
	//{
	//	double operator()(double);
	//};
	//Y y;
	//auto f3 = std::async(Y(), 3.141);//calls tmpy(3.141) where tmpy is move-constructed from Y()
	//auto f4 = std::async(std::ref(y), 2.718);//calls y(2.718)
	//X baz(X&);
	//std::async(baz, std::ref(x));//calls baz(x)
	//struct move_only
	//{
	//public:
	//	move_only();
	//	move_only(move_only&&);
	//	move_only(move_only const&) = delete;
	//	move_only& operator=(move_only&&);
	//	move_only& operator=(move_only const&) = delete;

	//	void operator() ();
	//};
	//auto f5 = std::async(move_only());//calls tmp() where tmp is constructed from std::move(move_only())

	//auto f6 = std::async(std::launch::async, Y(), 1.2);//run in new thread
	//auto f7 = std::async(std::launch::deferred, baz, std::ref(x));//run in wait() or get()
	//auto f8 = std::async(std::launch::async | std::launch::deferred, baz, std::ref(x));//implementation chooses
	//auto f9 = std::async(baz, std::ref(x));//implementation chooses
	//f7.wait();//invoke deferred function

}

namespace Note4dot2dot2 {
	//std::mutex m;
	//std::deque<std::packaged_task<void()>> tasks;

	//bool gui_shutdown_message_received();
	//void get_and_process_gui_message();

	//void gui_thread()
	//{
	//	while (!gui_shutdown_message_received())//when gui received message, the gui will shutdown
	//	{
	//		get_and_process_gui_message();//repeatedly polling for gui messaged to handle
	//		//such as user clicks, and for tasks on the task queue
	//		//if there are no tasks on the queue, it loops again
	//		//otherwise, it extracts the task from the queue
	//		std::packaged_task<void()> task;
	//		{
	//			std::lock_guard<std::mutex> lk(m);
	//			if (tasks.empty())
	//				continue;
	//			task = std::move(tasks.front());//extract the task from the queue
	//			tasks.pop_front();
	//		}
	//		task();//run the task, future associated with the task, will then be made ready when the task completes
	//	}
	//}

	//std::thread gui_bg_thread(gui_thread);

	//template<typename Func>
	//std::future<void> post_task_for_gui_thread(Func f)
	//{
	//	std::packaged_task<void()> task(f);
	//	std::future<void> res = task.get_future();
	//	std::lock_guard<std::mutex> lk(m);
	//	tasks.push_back(std::move(task));
	//	return res;
	//}
}

namespace Chapter4 {
	//using namespace Note4dot2;
	
	void M_Test();
}
