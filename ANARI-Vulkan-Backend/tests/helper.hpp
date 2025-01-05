import <utility>;

template<typename Fn>
struct ScopeExitCallback
{
	ScopeExitCallback(Fn&& fn) : m_fn(std::move(fn)) {}
	~ScopeExitCallback() { m_fn(); }
	Fn m_fn;
};

template<typename Fn>
ScopeExitCallback<Fn> operator+(nullptr_t, Fn&& fn)
{
	return ScopeExitCallback<Fn>(std::move(fn));
}

import <stdint.h>;
import <array>;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usize = size_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

using b8 = bool;
static_assert(sizeof(b8) == sizeof(uint8_t), "b8 must be 1 byte");

template<typename T, usize N>
using arr = std::array<T, N>;

template<typename typ, usize arr_size>
[[nodiscard]]
constexpr arr<typ, arr_size> operator+ (arr<typ, arr_size> arr1, arr<typ, arr_size> arr2) {
	arr<typ, arr_size> result = {};
	for (usize i = 0; i < arr_size; i++) {
		result[i] = arr1[i] + arr2[i];
	}
	return result;
}

template<typename typ, usize arr_size>
[[nodiscard]]
constexpr arr<typ, arr_size> operator- (const arr<typ, arr_size>& arr1, const arr<typ, arr_size>& arr2) {
	arr<typ, arr_size> result = {};
	for (usize i = 0; i < arr_size; i++) {
		result[i] = arr1[i] - arr2[i];
	}
	return result;
}

template<typename typ, usize arr_size>
[[maybe_unused]]
constexpr arr<typ, arr_size>& operator+= (arr<typ, arr_size>& self, const arr<typ, arr_size>&& arr) {
	for (usize i = 0; i < arr_size; i++) {
		self[i] += arr[i];
	}
	return self;
}

template<typename typ, usize arr_size>
[[maybe_unused]]
constexpr arr<typ, arr_size>& operator-= (arr<typ, arr_size>& self, const arr<typ, arr_size>&& arr) {
	for (usize i = 0; i < arr_size; i++) {
		self[i] -= arr[i];
	}
	return self;
}

template<typename typ, usize arr_size>
[[nodiscard]]
constexpr b8 operator== (const arr<typ, arr_size>& arr1, const arr<typ, arr_size>& arr2) {
	u32 result = 0;
	for (usize i = 0; i < arr_size; i++) {
		result += arr1[i] == arr2[i];
	}
	return result == arr_size;
}

template<typename T>
union vector2
{
	struct { T x, y; };
	struct { T r, g; };
	arr<T, 2> data;

	[[nodiscard]]
	T& operator[](u8 i) { return data[i]; }

	[[nodiscard]]
	auto operator-() const { return std::remove_pointer_t<decltype(this)>(-data); }
	[[nodiscard]]
	auto operator+(const auto&& rhs) const { return data + rhs.data; }
	[[nodiscard]]
	auto operator-(const auto&& rhs) const { return data - rhs.data; }

	[[maybe_unused]]
	auto& operator+=(const auto&& rhs) { data += rhs.data; return *this; }
	[[maybe_unused]]
	auto& operator-=(const auto&& rhs) { data -= rhs.data; return *this; }

	[[nodiscard]]
	b8 operator==(const auto&& rhs) { return data == rhs.data; }
};

template<typename T>
union vector3
{
	struct { T x, y, z; };
	struct { T r, g, b; };
	arr<T, 3> data;

	[[nodiscard]]
	T& operator[](u8 i) { return data[i]; }

	[[nodiscard]]
	auto operator-() const { return std::remove_pointer_t<decltype(this)>(-data); }
	[[nodiscard]]
	auto operator+(const auto&& rhs) const { return data + rhs.data; }
	[[nodiscard]]
	auto operator-(const auto&& rhs) const { return data - rhs.data; }

	[[maybe_unused]]
	auto& operator+=(const auto&& rhs) { data += rhs.data; return *this; }
	[[maybe_unused]]
	auto& operator-=(const auto&& rhs) { data -= rhs.data; return *this; }

	[[nodiscard]]
	b8 operator==(const auto&& rhs) { return data == rhs.data; }
};

template<typename T>
union vector4
{
	struct { T x, y, z, w; };
	struct { T r, g, b, a; };
	arr<T, 4> data;

	[[nodiscard]]
	T& operator[](u8 i) { return data[i]; }

	[[nodiscard]]
	auto operator-() const { return std::remove_pointer_t<decltype(this)>(-data); }
	[[nodiscard]]
	auto operator+(const auto&& rhs) const { return data + rhs.data; }
	[[nodiscard]]
	auto operator-(const auto&& rhs) const { return data - rhs.data; }

	[[maybe_unused]]
	auto& operator+=(const auto&& rhs) { data += rhs.data; return *this; }
	[[maybe_unused]]
	auto& operator-=(const auto&& rhs) { data -= rhs.data; return *this; }

	[[nodiscard]]
	b8 operator==(const auto&& rhs) { return data == rhs.data; }
};

using vec2 = vector2<f32>;
using vec3 = vector3<f32>;
using vec4 = vector4<f32>;

using ivec2 = vector2<i32>;
using ivec3 = vector3<i32>;
using ivec4 = vector4<i32>;

using uvec2 = vector2<u32>;
using uvec3 = vector3<u32>;
using uvec4 = vector4<u32>;

using mat2 = arr<vec2, 2>;
using mat3 = arr<vec3, 3>;
using mat4 = arr<vec4, 4>;
