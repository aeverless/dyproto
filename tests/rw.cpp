#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <concepts>
#include <list>
#include <ranges>
#include <algorithm>

#include "dyproto/reader.hpp"
#include "dyproto/traits.hpp"
#include "dyproto/writer.hpp"

namespace
{
struct A
{
	std::int64_t  x;
	std::uint32_t y;
	float         z;

	[[nodiscard]]
	constexpr auto operator==(A const&) const noexcept -> bool = default;

	[[nodiscard]]
	constexpr auto operator!=(A const&) const noexcept -> bool = default;
};

struct B
{
	float         x;
	std::byte     y;
	std::uint16_t z;

	[[nodiscard]]
	constexpr auto operator==(B const&) const noexcept -> bool = default;

	[[nodiscard]]
	constexpr auto operator!=(B const&) const noexcept -> bool = default;
};

struct Empty
{
	// This constructor is to allow generating expressions of type `C` down the line.
	[[nodiscard]]
	constexpr explicit Empty(...) noexcept
	{}

	[[nodiscard]]
	constexpr auto operator==(Empty const&) const noexcept -> bool = default;

	[[nodiscard]]
	constexpr auto operator!=(Empty const&) const noexcept -> bool = default;
};

static_assert(dyproto::traits::trivial_argument<A>);
static_assert(dyproto::traits::trivial_argument<B>);
static_assert(dyproto::traits::trivial_argument<Empty>);

template <dyproto::traits::argument T>
	requires std::equality_comparable<T>
[[nodiscard]]
auto is_argument_io_coherent(T const& argument, std::size_t times = 1) -> bool
{
	static constexpr std::size_t buffer_size = 65'536;

	if (times == 0)
	{
		return true;
	}

	std::array<std::byte, buffer_size> bytes{};

	dyproto::writer w{bytes};

	for (std::size_t i = 0; i < times; ++i)
	{
		w.write<T>(argument);
	}

	dyproto::reader r{bytes};

	for (std::size_t i = 0; i < times; ++i)
	{
		auto const read_argument = r.read<T>();

		if (argument != read_argument)
		{
			return false;
		}
	}

	return true;
}

template <dyproto::traits::argument T, std::size_t TakeMultiplier = 4>
[[nodiscard]]
auto generate_random_argument() -> T
{
	static constexpr double max_abs_random = 2'147'483'648.0;

	T argument{};

	if constexpr (dyproto::traits::trivial_argument<T>)
	{
		argument = static_cast<T>(GENERATE(take(16 * TakeMultiplier, random(-max_abs_random, max_abs_random))));
	}
	else if constexpr (dyproto::traits::container_argument<T>)
	{
		std::size_t const n = GENERATE(take(8 * TakeMultiplier, random(0, 255)));
		argument.reserve(n);

		for (std::size_t i = 0; i < n; ++i)
		{
			argument.push_back(static_cast<T::value_type>(GENERATE(take(1, random(-max_abs_random, max_abs_random)))));
		}
	}
	else
	{
		static_assert(false, "unsupported argument type");
	}

	return argument;
}

#define ARGUMENTS                                                                                                                           \
	std::byte, std::int32_t, std::int64_t, float, double, std::vector<std::byte>, std::vector<std::int32_t>, std::vector<std::int64_t>, \
		std::vector<float>, std::vector<double>, A, B, Empty

TEMPLATE_TEST_CASE("a single argument can be written to and read from byte arrays", "[argument][io]", ARGUMENTS)
{
	if constexpr (!dyproto::traits::container_argument<TestType>)
	{
		static_assert(dyproto::traits::reservable_container_argument<std::vector<TestType>>);
		static_assert(dyproto::traits::container_argument<std::list<TestType>>);
	}

	auto const argument = generate_random_argument<TestType>();

	REQUIRE(is_argument_io_coherent(argument));
}

TEMPLATE_TEST_CASE("a single argument can be written to and read from byte arrays several times in a row", "[argument][io]", ARGUMENTS)
{
	auto const argument = generate_random_argument<TestType>();

	std::size_t const n = GENERATE(range(0, 8));

	REQUIRE(is_argument_io_coherent(argument, n));
}

TEMPLATE_TEST_CASE("several arguments can be written to and read from byte arrays at once", "[argument][io]", ARGUMENTS)
{
	static constexpr std::size_t buffer_size = 8192;

	// It doesn't really matter here if arguments differ from each other or not.
	auto const  a1 = generate_random_argument<TestType, 1>();
	auto const& a2 = a1;
	auto const& a3 = a2;

	std::array<std::byte, buffer_size> bytes{};

	dyproto::writer w{bytes};
	dyproto::reader r{bytes};

	SECTION("writing and reading arguments at once")
	{
		REQUIRE(w.write<TestType, TestType, TestType>(a1, a2, a3));
		REQUIRE(std::tie(a1, a2, a3) == r.read<TestType, TestType, TestType>());
	}

	SECTION("writing and reading arguments sequentially")
	{
		REQUIRE(w.write<TestType>(a1));
		REQUIRE(w.write<TestType>(a2));
		REQUIRE(w.write<TestType>(a3));

		REQUIRE(a1 == r.read<TestType>());
		REQUIRE(a2 == r.read<TestType>());
		REQUIRE(a3 == r.read<TestType>());
	}
}

TEST_CASE("several arguments of different types can be written to and read from byte arrays at once", "[argument][io]")
{
	static constexpr std::size_t buffer_size = 8192;

	auto const a1 = generate_random_argument<std::byte, 1>();
	auto const a2 = generate_random_argument<std::int32_t, 1>();
	auto const a3 = generate_random_argument<std::vector<std::int64_t>, 1>();
	auto const a4 = generate_random_argument<double, 1>();
	auto const a5 = generate_random_argument<std::vector<float>, 1>();

	std::array<std::byte, buffer_size> bytes{};

	dyproto::writer w{bytes};
	dyproto::reader r{bytes};

	REQUIRE(w.write<std::byte, std::int32_t, std::vector<std::int64_t>, double, std::vector<float>>(a1, a2, a3, a4, a5));
	REQUIRE(std::tie(a1, a2, a3, a4, a5) == r.read<std::byte, std::int32_t, std::vector<std::int64_t>, double, std::vector<float>>());
}

TEST_CASE("out-of-space errors can be handled", "[argument][io]")
{
	static constexpr std::size_t buffer_size = 8;

	auto const size_1 = generate_random_argument<std::byte, 1>();
	auto const size_4 = generate_random_argument<std::int32_t, 1>();
	auto const size_8 = generate_random_argument<std::int64_t, 1>();

	std::array<std::byte, buffer_size> bytes{};

	dyproto::writer w{bytes};

	SECTION("writing arguments sequentially")
	{
		// The first two will be written successfully, but the last one will not.
		REQUIRE(w.write<std::byte>(size_1) == sizeof(size_1));
		REQUIRE(w.write<std::int32_t>(size_4) == sizeof(size_4));
		REQUIRE_FALSE(w.write<std::int32_t>(size_4));
	}

	SECTION("writing several arguments at once")
	{
		REQUIRE_FALSE(w.write<std::byte, std::int32_t, std::int64_t>(size_1, size_4, size_8));
	}
}
}
