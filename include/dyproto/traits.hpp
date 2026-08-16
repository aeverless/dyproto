#ifndef DYPROTO_TRAITS_HPP
#define DYPROTO_TRAITS_HPP

#include <ranges>
#include <type_traits>

namespace dyproto::traits
{
template <typename T>
concept trivial_argument = std::is_trivially_copyable_v<T> && !std::is_function_v<T> && !std::is_pointer_v<T>;

template <typename T>
concept container_argument = std::ranges::sized_range<T> && std::default_initializable<T> && trivial_argument<typename T::value_type> && requires (T& t) {
	typename T::value_type;
	t.push_back(std::declval<typename T::value_type>());
};

template <typename T>
concept reservable_container_argument = container_argument<T> && requires (T& t) { t.reserve(0uz); };

template <typename T>
concept argument = trivial_argument<T> || container_argument<T>;
}

#endif
