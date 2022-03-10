#pragma once

#include <vector>
#include <iterator>
#include <thread>
#include <numeric>
#include <functional>
#include <algorithm>

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

namespace Chapter2 {
	using namespace Note2dot8;

	void M_Test() {
		std::vector<int> v;

		for (int i = 0; i < 101; ++i) v.push_back(i);

		std::cout << Note2dot8::parallel_accumulate(v.begin(), v.end(), 0) << std::endl;
	}
}
