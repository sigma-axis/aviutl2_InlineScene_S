/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <algorithm>
#include <string>

namespace InlineScene_S::Settings
{
	inline constinit struct Setting {
		template<class T, T def_val, T min_val, T max_val>
		struct prop_num {
			using type = T;
			constexpr static type def = def_val, min = min_val, max = max_val;
			type value = def;
			type get() const { return value; }
			void set(type val) { value = val; }
			prop_num& clamp() { value = std::clamp(value, min, max); return *this; }
			prop_num() = default;
			prop_num(type const& val) : value{ val } {}
		};

		prop_num<int32_t, 2, 1, 60> cache_expire_minutes{};
		prop_num<int64_t, 16'000'000, 16'000'000, 512'000'000> cache_total_dots{};

		Setting& clamp() {
			cache_expire_minutes.clamp();
			cache_total_dots.clamp();
			return *this;
		}
	} settings = {};

	/**
	* @brief Loads a `Setting` struct from a json file.
	* @param file_path The path to the json file to load from.
	*/
	Setting load(std::wstring const& file_path);
	/**
	* @brief Saves a `Setting` struct to a json file.
	* @param file_path The path to the json file to save to.
	* @param settings The reference to the `Setting` struct to save.
	*/
	void save(std::wstring const& file_path, Setting const& settings);
}
