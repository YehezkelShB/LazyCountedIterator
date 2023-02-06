#pragma once

// lazy_take - An implementation of the proposed std::ranges::lazy_take (P2406R3)
// Written in 2023 by Yehezkel Bernat (YehezkelShB@gmail.com, @yehezkelshb)
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Parts of this work are based on Microsoft STL code
// which has the following copyright notice:
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <ranges>
#include <algorithm>

namespace yb::ranges {

template <bool Const, class T>
using maybe_const = std::conditional_t<Const, const T, T>;

template<class R>
concept simple_view =
std::ranges::view<R> && std::ranges::range<const R> &&
std::same_as<std::ranges::iterator_t<R>, std::ranges::iterator_t<const R>> &&
std::same_as<std::ranges::sentinel_t<R>, std::ranges::sentinel_t<const R>>;

template<std::ranges::view V>
class lazy_take_view : public std::ranges::view_interface<lazy_take_view<V>>
{
private:
	V base_ = V();
	std::ranges::range_difference_t<V> count_ = 0;

	template<bool Const>
	class sentinel
	{
	private:
		template<bool>
		friend class sentinel;

		using Base = maybe_const<Const, V>;
		template<bool OtherConst>
		using CI = lazy_counted_iterator<std::ranges::iterator_t<maybe_const<OtherConst, V>>>;
		std::ranges::sentinel_t<Base> end_ = std::ranges::sentinel_t<Base>();

	public:
		sentinel() = default;
		constexpr explicit sentinel(std::ranges::sentinel_t<Base> end)
			: end_(std::move(end))
		{
		}
		constexpr sentinel(sentinel<!Const> s)
			requires Const&& std::convertible_to<std::ranges::sentinel_t<V>, std::ranges::sentinel_t<Base>>
		: end_(std::move(s.end_))
		{
		}

		constexpr std::ranges::sentinel_t<Base> base() const
		{
			return end_;
		}

		friend constexpr bool operator==(const CI<Const>& y, const sentinel& x)
		{
			return y.count() == 0 || y.private_base() == x.end_;
		}

		template<bool OtherConst = !Const>
			requires std::sentinel_for<std::ranges::sentinel_t<Base>, std::ranges::iterator_t<maybe_const<OtherConst, V>>>
		friend constexpr bool operator==(const CI<OtherConst>& y, const sentinel& x)
		{
			return y.count() == 0 || y.private_base() == x.end_;
		}
	};

	
public:
	lazy_take_view() requires std::default_initializable<V> = default;
	constexpr lazy_take_view(V base, std::ranges::range_difference_t<V> count)
		: base_(std::move(base)), count_ { count } {}

	constexpr V base() const& requires std::copy_constructible<V> { return base_; }
	constexpr V base()&&
	{
		return std::move(base_);
	}

	constexpr auto begin() requires (!simple_view<V>)
	{
		if constexpr (std::ranges::sized_range<V>)
		{
			if constexpr (std::ranges::random_access_range<V>)
			{
				return std::ranges::begin(base_);
			}
			else
			{
				auto sz = std::ranges::range_difference_t<V>(size());
				return lazy_counted_iterator(std::ranges::begin(base_), sz);
			}
		}
		else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<V>, std::ranges::iterator_t<V>>)
		{
			auto it = std::ranges::begin(base_);
			auto sz = std::min(count_, std::ranges::end(base_) - it);
			return lazy_counted_iterator(std::move(it), sz);
		}
		else
		{
			return lazy_counted_iterator(std::ranges::begin(base_), count_);
		}
	}

	constexpr auto begin() const requires std::ranges::range<const V> {
		if constexpr (std::ranges::sized_range<const V>)
		{
			if constexpr (std::ranges::random_access_range<const V>)
			{
				return std::ranges::begin(base_);
			}
			else
			{
				auto sz = std::ranges::range_difference_t<const V>(size());
				return lazy_counted_iterator(std::ranges::begin(base_), sz);
			}
		}
		else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<const V>, std::ranges::iterator_t<const V>>)
		{
			auto it = std::ranges::begin(base_);
			auto sz = std::min(count_, std::ranges::end(base_) - it);
			return lazy_counted_iterator(std::move(it), sz);
		}
		else
		{
			return lazy_counted_iterator(std::ranges::begin(base_), count_);
		}
	}

	constexpr auto end() requires (!simple_view<V>)
	{
		if constexpr (std::ranges::sized_range<V>)
		{
			if constexpr (std::ranges::random_access_range<V>)
				return std::ranges::begin(base_) + std::ranges::range_difference_t<V>(size());
			else
				return std::default_sentinel;
		}
		else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<V>, std::ranges::iterator_t<V>>)
		{
			return std::default_sentinel;
		}
		else
		{
			return sentinel<false>{std::ranges::end(base_)};
		}
	}

	constexpr auto end() const requires std::ranges::range<const V> {
		if constexpr (std::ranges::sized_range<const V>)
		{
			if constexpr (std::ranges::random_access_range<const V>)
				return std::ranges::begin(base_) + std::ranges::range_difference_t<const V>(size());
			else
				return std::default_sentinel;
		}
		else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<const V>, std::ranges::iterator_t<const V>>)
		{
			return std::default_sentinel;
		}
		else
		{
			return sentinel<true>{std::ranges::end(base_)};
		}
	}

	constexpr auto size() requires std::ranges::sized_range<V> {
		auto n = std::ranges::size(base_);
		return std::ranges::min(n, static_cast<decltype(n)>(count_));
	}

	constexpr auto size() const requires std::ranges::sized_range<const V> {
		auto n = std::ranges::size(base_);
		return std::ranges::min(n, static_cast<decltype(n)>(count_));
	}
};

template<class R>
lazy_take_view(R&&, std::ranges::range_difference_t<R>) -> lazy_take_view<std::views::all_t<R>>;

} // namespace yb::ranges

template<class T>
constexpr bool std::ranges::enable_borrowed_range<yb::ranges::lazy_take_view<T>> = std::ranges::enable_borrowed_range<T>;

// TODO: lazy_take
