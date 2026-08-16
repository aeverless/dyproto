#ifndef DYPROTO_WRITER_HPP
#define DYPROTO_WRITER_HPP

#include "traits.hpp"

#include <expected>
#include <array>
#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>
#include <ranges>
#include <span>
#include <system_error>

namespace dyproto
{
using write_result = std::expected<std::size_t, std::error_code>;

namespace details
{}

struct [[nodiscard]] writer
{
	constexpr explicit writer(std::span<std::byte> buf) noexcept
		: buf_{buf}
	{}

	template <traits::argument... Ts>
		requires (sizeof...(Ts) > 1)
	constexpr auto write(std::type_identity_t<Ts> const&... values) -> write_result
	{
		static constexpr auto size_of_argument = []<traits::argument A>(A const& arg) -> std::size_t
		{
			if constexpr (traits::trivial_argument<A>)
			{
				return sizeof(A);
			}
			else
			{
				return 1 + std::ranges::size(arg) * sizeof(typename A::value_type);
			}
		};

		static constexpr auto is_argument_valid = []<traits::argument A>(A const& arg) -> bool
		{
			if constexpr (traits::trivial_argument<A>)
			{
				return true;
			}
			else
			{
				static constexpr std::uint8_t max_array_size = 255;
				return std::ranges::size(arg) <= max_array_size;
			}
		};

		if (!(is_argument_valid(values) && ...))
		{
			return std::unexpected{std::make_error_code(std::errc::value_too_large)};
		}

		std::size_t const size = (size_of_argument(values) + ...);

		if (buf_.size() < size)
		{
			return std::unexpected{std::make_error_code(std::errc::no_buffer_space)};
		}

		write_result::error_type err;

		std::size_t written = 0;

		auto const write_and_accumulate = [&]<typename T>(T const& value)
		{
			if (err != std::errc{})
			{
				return false;
			}

			write_result const result = write<T>(value);

			if (!result)
			{
				err = result.error();
				return false;
			}

			written += result.value();
			return true;
		};

		if (bool const ok = (write_and_accumulate(values) && ...); !ok)
		{
			return std::unexpected{err};
		}

		return written;
	}

	template <traits::trivial_argument T>
	constexpr auto write(std::type_identity_t<T> const& value) -> write_result
	{
		using representation_type = std::array<std::byte, sizeof(T)>;

		static constexpr std::size_t size = sizeof(T);

		if (buf_.size() < size)
		{
			return std::unexpected{std::make_error_code(std::errc::no_buffer_space)};
		}

		auto bytes = std::bit_cast<representation_type>(value);

		if (std::endian::native == std::endian::little)
		{
			std::ranges::reverse(bytes);
		}

		std::ranges::copy(bytes, buf_.begin());

		buf_ = buf_.subspan<size>();

		return size;
	}

	template <traits::container_argument T>
	constexpr auto write(std::type_identity_t<T> const& container) -> write_result
	{
		static constexpr std::uint8_t max_array_size = 255;

		if (buf_.empty())
		{
			return std::unexpected{std::make_error_code(std::errc::no_buffer_space)};
		}

		std::size_t const size = std::ranges::size(container);
		if (size > max_array_size || size * sizeof(typename T::value_type) > buf_.size() - 1)
		{
			return std::unexpected{std::make_error_code(std::errc::value_too_large)};
		}

		// Write the container's size into the buffer.
		buf_.front() = std::byte(size);
		buf_         = buf_.subspan<1>();

		std::size_t written = 1;

		for (auto const& element : container)
		{
			write_result const subresult = write<typename T::value_type>(element);

			if (!subresult.has_value())
			{
				return subresult;
			}

			written += subresult.value();
		}

		return written;
	}

private:
	std::span<std::byte> buf_;
};
}

#endif
