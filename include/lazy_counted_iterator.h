#pragma once

// lazy_counted_iterator - An implementation of the proposed std::lazy_counted_iterator (P2406R3)
// Written in 2023 by Yehezkel Bernat (YehezkelShB@gmail.com, @yehezkelshb)
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Parts of this work are based on Microsoft STL code
// which has the following copyright notice:
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iterator>
#include <concepts>

namespace yb {

template<class T> using with_reference = T&;

template<class T>
concept can_reference = requires { typename with_reference<T>; };

template<class T>
concept dereferenceable = requires(T & t)
{
	{ *t } -> can_reference;
};

template <class I>
struct Lazy_counted_iterator_category_base
{
};

template <class I>
	requires requires { typename std::iterator_traits<I>::iterator_category; }
struct Lazy_counted_iterator_category_base<I>
{
	using iterator_category = std::conditional_t<std::derived_from<typename std::iterator_traits<I>::iterator_category, std::forward_iterator_tag>,
		std::forward_iterator_tag, typename std::iterator_traits<I>::iterator_category>;
};

template<std::input_iterator I>
class lazy_counted_iterator : public Lazy_counted_iterator_category_base<I>
{
public:
	using iterator_type = I;
	using value_type = std::iter_value_t<I>;
	using difference_type = std::iter_difference_t<I>;

private:
	static constexpr auto get_iter_concept()
	{
		if constexpr (std::forward_iterator<I>)
		{
			return std::forward_iterator_tag {};
		}
		else
		{
			return std::input_iterator_tag {};
		}
	}

public:
	using iterator_concept = decltype(get_iter_concept());

	constexpr lazy_counted_iterator() requires std::default_initializable<I> = default;

	constexpr lazy_counted_iterator(I x, std::iter_difference_t<I> n)
		: current(std::move(x)), length(n)
	{
	}

	template<class I2>
		requires std::convertible_to<const I2&, I>
	constexpr lazy_counted_iterator(const lazy_counted_iterator<I2>& x)
		: current(x.private_base()), length(x.count())
	{
	}

	template<class I2>
		requires std::assignable_from<I&, const I2&>
	constexpr lazy_counted_iterator& operator=(const lazy_counted_iterator<I2>& x)
	{
		current = x.private_base();
		length = x.count();
		return *this;
	}

	// lazy_counted_iterator has no base() member; provide an internal one for implementation
	[[nodiscard]] constexpr const I& private_base() const&
	{
		return current;
	}

	[[nodiscard]] constexpr I private_base()&&
	{
		return std::move(current);
	}

	constexpr std::iter_difference_t<I> count() const noexcept
	{
		return length;
	}

	constexpr decltype(auto) operator*()
	{
		return *current;
	}

	constexpr decltype(auto) operator*() const
		requires dereferenceable<const I>
	{
		return *current;
	}

	constexpr lazy_counted_iterator& operator++()
	{
		if (length > 1) ++current;
		--length;
		return *this;
	}

	constexpr void operator++(int)
	{
		++*this;
	}

	constexpr lazy_counted_iterator operator++(int)
		requires std::forward_iterator<I>
	{
		lazy_counted_iterator tmp = *this;
		++*this;
		return tmp;
	}

	template<std::common_with<I> I2>
	friend constexpr std::iter_difference_t<I2> operator-(
		const lazy_counted_iterator& x, const lazy_counted_iterator<I2>& y)
	{
		return y.count() - x.count();
	}

	friend constexpr std::iter_difference_t<I> operator-(
		const lazy_counted_iterator& x, std::default_sentinel_t)
	{
		return -x.count();
	}

	friend constexpr std::iter_difference_t<I> operator-(
		std::default_sentinel_t, const lazy_counted_iterator& y)
	{
		return y.count();
	}


	template<std::common_with<I> I2>
	friend constexpr bool operator==(
		const lazy_counted_iterator& x, const lazy_counted_iterator<I2>& y)
	{
		return x.count() == y.count();
	}

	friend constexpr bool operator==(
		const lazy_counted_iterator& x, std::default_sentinel_t)
	{
		return x.count() == 0;
	}

	template<std::common_with<I> I2>
	friend constexpr std::strong_ordering operator<=>(
		const lazy_counted_iterator& x, const lazy_counted_iterator<I2>& y)
	{
		return y.count() <=> x.count();
	}

	friend constexpr std::iter_rvalue_reference_t<I> iter_move(const lazy_counted_iterator& i)
		noexcept(noexcept(std::ranges::iter_move(i.current)))
	{
		return std::ranges::iter_move(i.current);
	}

	template<std::indirectly_swappable<I> I2>
	friend constexpr void iter_swap(const lazy_counted_iterator& x, const lazy_counted_iterator<I2>& y)
		noexcept(noexcept(std::ranges::iter_swap(x.current, y.current)))
	{
		std::ranges::iter_swap(x.current, y.private_base());
	}

private:
	I current = I();
	std::iter_difference_t<I> length = 0;
};

} // namespace yb

