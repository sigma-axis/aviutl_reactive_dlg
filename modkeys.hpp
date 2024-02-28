/*
The MIT License (MIT)

Copyright (c) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <cstring>
#include <concepts>
#include <utility>

////////////////////////////////
// Combinations of modifier keys.
////////////////////////////////
struct modkeys {
	enum class key : uint8_t {
		none = 0,
		ctrl = 1 << 0,
		shift = 1 << 1,
		alt = 1 << 2,
	};
	using enum key;

	constexpr modkeys() : val{} {}
	constexpr modkeys(key val) : val{ val } {}
	constexpr operator const key& () const { return val; }
	constexpr operator key& () { return val; }

	constexpr modkeys(bool ctrl, bool shift, bool alt) : val{ from_bools(ctrl, shift, alt) } {}
	constexpr modkeys(const char* str, modkeys def) : val{ (parse(str, def), def) } {}

	// flag testing functions.
	template<std::convertible_to<modkeys>... TArgs>
	constexpr bool has_flags_all(TArgs... flags) const {
		if constexpr (sizeof...(TArgs) > 0) return val >= (flags | ...);
		else return true;
	}
	template<std::convertible_to<modkeys>... TArgs>
	constexpr bool has_flags_any(TArgs... flags) const {
		if constexpr (sizeof...(TArgs) > 0) return (val & (flags | ...)) != none;
		else return val != none;
	}
	// an alias of has_flags_any().
	template<std::convertible_to<modkeys>... TArgs>
	constexpr bool has_flags(TArgs... flags) const { return has_flags_any(flags...); }

	// string convertions.
	/// <param name="ret">keeps original value unless true is returned.</param>
	static constexpr bool parse(const char* str, modkeys& ret);

	// returns canonical names for each state of flags.
	constexpr const char* canon_name() const;

private:
	key val = none;
	static constexpr key from_bools(bool ctrl, bool shift, bool alt);

	// parsing from strings.
	static constexpr bool parse_one(const char* str, key& ret)
	{
		constexpr auto tolower = [](char c) -> char {
			// https://blog.yimmo.org/posts/faster-tolower-in-c.html
			// needs to shift one more bit than the reference;
			// c = '\xc1' would fail otherwise.
			return c ^ ((((0x40 - c) ^ (0x5a - c)) >> 3) & 0x20);
		};
		// case-insensitve truncated string comparison.
		constexpr auto comp = []<size_t N>(const char* s1, const char(&s2)[N]) {
			for (size_t i = 0; i < N - 1; i++) {
				if (tolower(s1[i]) != s2[i]) return false;
			}
			return true;
		};

		if (str[0] == '\0' || comp(str, "none")) ret = none;
		else if (comp(str, "ctrl" )) ret = ctrl;
		else if (comp(str, "shift")) ret = shift;
		else if (comp(str, "alt"  )) ret = alt;
		else return false;
		return true;
	}
};

// bitwise operators.
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
constexpr static modkeys operator&(L l, R r) { return static_cast<modkeys::key>(std::to_underlying(static_cast<modkeys::key>(l)) & std::to_underlying(static_cast<modkeys::key>(r))); }
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
constexpr static modkeys operator|(L l, R r) { return static_cast<modkeys::key>(std::to_underlying(static_cast<modkeys::key>(l)) | std::to_underlying(static_cast<modkeys::key>(r))); }
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
constexpr static modkeys operator^(L l, R r) { return static_cast<modkeys::key>(std::to_underlying(static_cast<modkeys::key>(l)) ^ std::to_underlying(static_cast<modkeys::key>(r))); }
template<std::convertible_to<modkeys::key> K>
constexpr static modkeys operator~(K k) { return static_cast<modkeys::key>(~std::to_underlying(static_cast<modkeys::key>(k))); }

// comparison operators represent the inclusion of sets, regarding modkeys as a set of flags.
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
constexpr static bool operator<=(L l, R r) { return (l & ~r) == modkeys::none; }
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
constexpr static bool operator<(L l, R r) { return (l != r) && (l <= r); }
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
constexpr static bool operator>=(L l, R r) { return r <= l; }
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
constexpr static bool operator>(L l, R r) { return r < l; }

// bitwise assignment operators.
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
	requires(std::convertible_to<modkeys::key, L>)
constexpr static L& operator&=(L& l, R r) { return l = static_cast<L>(l & r); }
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
	requires(std::convertible_to<modkeys::key, L>)
constexpr static L& operator|=(L& l, R r) { return l = static_cast<L>(l | r); }
template<std::convertible_to<modkeys::key> L, std::convertible_to<modkeys::key> R>
	requires(std::convertible_to<modkeys::key, L>)
constexpr static L& operator^=(L& l, R r) { return l = static_cast<L>(l ^ r); }

inline constexpr modkeys::key modkeys::from_bools(bool ctrl, bool shift, bool alt)
{
	auto ret = none;
	if (ctrl) ret |= key::ctrl;
	if (shift) ret |= key::shift;
	if (alt) ret |= key::alt;
	return ret;
}

inline constexpr bool modkeys::parse(const char* str, modkeys& ret)
{
	if (str == nullptr) return false;

	key val = none;
	for (auto p = str - 1; p != nullptr; p = std::strpbrk(p, "+-*/|&:;,")) {
		while (*++p == ' ');
		if (key k; parse_one(p, k)) val |= k;
		else return false;
	}
	ret = val;
	return true;
}

inline constexpr const char* modkeys::canon_name() const
{
	switch (val) {
	case none: return "none";
	case ctrl: return "ctrl";
	case shift: return "shift";
	case alt: return "alt";
	case ctrl | shift: return "ctrl+shift";
	case ctrl | alt: return "ctrl+alt";
	case shift | alt: return "shift+alt";
	case ctrl | shift | alt: return "ctrl+shift+alt";
	}
	return "";
}
