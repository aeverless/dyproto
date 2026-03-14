#ifndef DYPROTO_READER_HPP
#define DYPROTO_READER_HPP

#include "traits.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <span>
#include <ranges>
#include <tuple>

namespace dyproto
{
struct [[nodiscard]] reader
{
	constexpr explicit reader(std::span<std::byte const> buf) noexcept
		: buf_{buf}
	{}

	template <traits::argument... Ts>
		requires (sizeof...(Ts) > 1)
	constexpr auto read() -> std::tuple<Ts...>
	{
		return std::make_tuple(read<Ts>()...);
	}

	template <traits::trivial_argument T>
	constexpr auto read() -> T
	{
		using representation_type = std::array<std::byte, sizeof(T)>;

		// It's the user's responsibility to keep the buffer size intact.
		assert(buf_.size() >= sizeof(T));

		representation_type bytes{};

		std::ranges::copy(buf_ | std::views::take(sizeof(T)), bytes.data());

		if (std::endian::native == std::endian::little)
		{
			std::ranges::reverse(bytes);
		}

		buf_ = buf_.subspan<sizeof(T)>();

		return std::bit_cast<T>(bytes);
	}

	template <traits::container_argument T>
	constexpr auto read() -> T
	{
		static constexpr std::uint8_t max_array_size = 255;

		assert(buf_.size() >= 1);

		auto const n = std::min(max_array_size, static_cast<std::uint8_t>(buf_.front()));

		buf_ = buf_.subspan<1>();

		T value;

		if constexpr (traits::reservable_container_argument<T>)
		{
			value.reserve(n);
		}

		for (std::uint8_t i = 0; i < n; ++i)
		{
			value.push_back(read<typename T::value_type>());
		}

		return value;
	}

private:
	std::span<std::byte const> buf_;
};
}

#endif
