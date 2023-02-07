#include "lazy_counted_iterator.h"
#include "lazy_take.h"

#include <ranges>
#include <iostream>
#include <sstream>
#include <cassert>
#include <future>

namespace rn = std::ranges;
namespace rv = rn::views;

void input_iterator_case();
void forward_iterator_case_lazy();
void forward_iterator_case_nonlazy();

int main()
{
	input_iterator_case();
	forward_iterator_case_lazy();

	auto fut = std::async(std::launch::async, forward_iterator_case_nonlazy);
	using namespace std::chrono_literals;
	if (fut.wait_for(10s) == std::future_status::timeout)
	{
		std::cout << "timeout while waiting for forward_iterator_case_nonlazy()\nExit with ^C\n";
	}
}

void input_iterator_case()
{
	auto iss = std::istringstream("0 1 2");
	for (auto i : yb::ranges::lazy_take_view(rn::istream_view<int>(iss), 1))
		std::cout << i << '\n';
	auto i = 0;
	iss >> i;
	std::cout << i << std::endl; // flush it in case the assert fails
	assert(i == 1);
}

void forward_iterator_case_lazy()
{
	for (auto i : yb::ranges::lazy_take_view(rv::iota(0)
		| rv::filter([](auto i)
			{
				return i < 11;
			}), 11))
		std::cout << i << '\n';
}

void forward_iterator_case_nonlazy()
{
	for (auto i : std::ranges::take_view(rv::iota(0)
		| rv::filter([](auto i)
			{
				return i < 11;
			}), 11))
		std::cout << i << '\n';
}
