#include "lazy_counted_iterator.h"
#include "lazy_take.h"

#include <ranges>
#include <iostream>
#include <sstream>
#include <cassert>

namespace rn = std::ranges;
namespace rv = rn::views;

int main()
{
	auto iss = std::istringstream("0 1 2");
	for (auto i : yb::ranges::lazy_take_view(rn::istream_view<int>(iss), 1))
		std::cout << i << '\n';
	auto i = 0;
	iss >> i;
	std::cout << i << std::endl; // flush it in case the assert fails
	assert(i == 1);
}
