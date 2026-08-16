# dyproto

**dyproto** contains the implementation of a simple binary serialization library for trivially copyable types and containers thereof.

## Primary Features

- **Type safety**, because the reading and writing functions only accept objects that satisfy specific concepts
- **Error handling**, because write functions return write results that may contain the number of bytes written or an error code
- **Customizability**, because you can write your own types that satisfy the aforementioned specific concepts; moreover, you can read and write your own structural types so long as they're trivially copyable

Note that while write functions return write results, read functions return objects of type that you are trying to read and assert that the underlying buffer has enough space to read from.

Also note, structural types **may not** be correctly serialized and deserialized because current endianness conversion does not properly account for member variables.

Do note use this toy library in production.

## Examples

The general prerequisite for all of the examples:

```cpp
#include <dyproto/reader.hpp>
#include <dyproto/writer.hpp>

static constexpr std::size_t buffer_size = 1024;

std::array<std::byte, buffer_size> bytes{};

auto w = dyproto::writer{bytes};
auto r = dyproto::reader{bytes};
```

Reading and writing one object at a time:

```cpp
w.write<std::int32_t>(64);
assert(r.read<std::int32_t>() == 64);

w.write<std::uint8_t>(128);
w.write<std::vector<std::int64_t>>({-67, -65, 12, 42, -42, 91, 67});

assert(128 == r.read<std::uint8_t>());
assert(std::vector<std::int64_t>{-67, -65, 12, 42, -42, 91, 67} == r.read<std::vector<std::int64_t>>());
```

Reading and writing several objects at a time:

```cpp
w.write<
	std::uint8_t,
	std::vector<std::int32_t>,
	std::int64_t,
>(
	255,
	{-1, 0, 1, 2, 3, 4},
	6767,
);

auto const [a, b, c] = r.read<
	std::uint8_t,
	std::vector<std::int32_t>,
	std::int64_t,
>();

assert(a == 255);
assert(b == std::vector<std::int32_t>{-1, 0, 1, 2, 3, 4});
assert(c == 6767);
```

Writing until we run out of space:

```cpp
for (std::size_t i = 0; ; ++i)
{
	auto const result = w.write<std::size_t>(i);

	if (result)
	{
		continue;
	}

	auto const error_code = result.error();
	assert(error_code == std::errc::no_buffer_space);

	break;
}
```

## Concepts

The library contains four library concepts that can be applied to various types:

- `dyproto::traits::argument` models an *argument*,
  i.e. an object that can be read and written that is not a function and not a pointer

- `dyproto::traits::trivial_argument` models a *trivial argument*,
  i.e. a trivially copyable object

- `dyproto::traits::container_argument` models a *container argument*,
  i.e. a container of *trivial arguments*

- `dyproto::traits::reservable_container_argument` models a *reservable container argument*,
  i.e. a *container argument* that also has the *reserve* operation defined

A valid container argument `container` of type `C` is also required to:

- define `typename C::value_type` - the type of contained objects

- for a given rvalue `object` of the type convertible to the type of contained objects,
  ensure `container.push_back(object)` is a valid operation, e.g. by defining
  `void C::push_back(typename C::value_type&&)`

A valid reservable container argument `container` is required to:

- ensure `container.reserve(0uz)` is a valid operation, e.g. by defining
a member function `void C::reserve(std::size_t)`

The following STL containers satisfy the *container argument* requirements:

- `std::vector<T>` for any `T`
- `std::list<T>` for any `T`

Non-default allocators haven't been tested for.

## License

The library is licensed under the [MIT License](LICENSE).

## Authors

Artemy Astakhov (contact at aeverless dot dev)
