#include "Chapter4.h"

namespace Chapter4 {
	using namespace Note4dot2;

	void M_Test()
	{
		test();
	}

}

int Note4dot2::find_the_anwser_to_ltuae()
{
	{
		std::vector<int> v = { 1, 2, 3, 4, 5 };
		int32_t sum = 0;
		for (size_t i = 0; i < v.size(); ++i)
		{
			sum += v[i];
		}
		return sum;
	}
}

void Note4dot2::do_other_stuff()
{

}

void Note4dot2::test()
{
	{
		std::future<int> the_answer = std::async(find_the_anwser_to_ltuae);
		do_other_stuff();
		std::cout << "The anwser is " << the_answer.get() << std::endl;
	}
}
