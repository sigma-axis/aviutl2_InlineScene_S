/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filter2.h>

#include "InlineScene_Cache.hpp"

namespace InlineScene_S::Filters::Common
{
	struct cache_type {
		constexpr static auto option_name = L"モード";
		using id = Cache::Image::cache_type;
		using enum id;
		constexpr static id default_value = Shared;
		constexpr static wchar_t const* names[] = {
			L"一時キャッシュ",
			L"長期キャッシュ",
		};

		inline constinit static ::FILTER_ITEM_SELECT::ITEM list[] = {
			{ names[0], static_cast<int>(Shared) },
			{ names[1], static_cast<int>(Managed) },
			{ nullptr, },
		};
	};

	struct crop_mode_image {
		constexpr static auto option_name = L"余白除去";
		enum id : int {
			crop = 0,
			leave = 1,
			per_edges = 2,
		};
		constexpr static id default_value = id::crop;
		constexpr static wchar_t const* names[] = {
			L"除去する",
			L"除去しない",
			L"上下左右個別に指定",
		};

		inline constinit static ::FILTER_ITEM_SELECT::ITEM list[] = {
			{ names[0], id::crop },
			{ names[1], id::leave },
			{ names[2], id::per_edges },
			{ nullptr, },
		};
	};

	bool verify_open_close_image(::FILTER_PROC_VIDEO* video);
}