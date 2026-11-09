/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <string>
using namespace std::string_literals;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filter2.h>

#include "InlineScene_S.hpp"
#include "InlineScene_Cache.hpp"
#include "Filters_Common.hpp"
#include "Filter_Save.hpp"
using namespace InlineScene_S;
namespace Common = InlineScene_S::Filters::Common;

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B
////////////////////////////////
// core functionalities.
////////////////////////////////
bool filter_image_core(::FILTER_PROC_VIDEO* video,
	std::wstring_view const& name, Cache::Image::cache_type cache_type)
{
	switch (cache_type) {
	default:
	case Common::cache_type::Shared:
	{
		auto& list = Cache::Image::scene_cache[video->scene->id];
		list.scene_id = video->scene->id;
		return list.save(video, name, L"object"s);
	}
	case Common::cache_type::Managed:
		return Cache::Image::managed_cache.save(video, std::wstring{ name }, L"object"s);
	}
}
void filter_sound_core(::FILTER_PROC_AUDIO* audio, std::wstring const& name)
{
	auto& list = Cache::Sound::scene_cache[audio->scene->id];
	list.scene_id = audio->scene->id;
	list.save(audio, name);
}

////////////////////////////////
// setting parameters.
////////////////////////////////
namespace params
{
	::FILTER_ITEM_SELECT cache_type{ Common::cache_type::option_name, static_cast<int>(Common::cache_type::default_value), Common::cache_type::list };
	::FILTER_ITEM_STRING cache_name{ L"保存名", L"scene1" };
	::FILTER_ITEM_CHECK_SECTION suppress{ L"表示・再生しない", false, false };

	namespace hide_rules
	{
		using enum ::FILTER_ITEM_HIDE_RULE::OPERATOR;
		::FILTER_ITEM_HIDE_RULE hide_out_cache_type{ cache_type.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_cache_type{ cache_type.name, L"input", EQUAL, 2 };
	}

	constexpr void* all[] = {
		&cache_type,
		&cache_name,
		&suppress,

		&hide_rules::hide_out_cache_type,
		&hide_rules::hide_in_cache_type,
		nullptr,
	};
}

////////////////////////////////
// entry points.
////////////////////////////////
bool filter_image(::FILTER_PROC_VIDEO* video)
{
	using namespace params;
	return filter_image_core(video, cache_name.value,
		static_cast<Cache::Image::cache_type>(cache_type.value))
		&& !suppress.value;
}
bool filter_sound(::FILTER_PROC_AUDIO* audio)
{
	using namespace params;
	filter_sound_core(audio, cache_name.value);
	return !suppress.value;
}

ANON_NS_E

////////////////////////////////
// exports.
////////////////////////////////
#define FILTER_NAME	L"InlineScene_S単品保存"
constinit ::FILTER_PLUGIN_TABLE InlineScene_S::Filters::save_anm{
	.flag = ::FILTER_PLUGIN_TABLE::FLAG_VIDEO | ::FILTER_PLUGIN_TABLE::FLAG_AUDIO,
	.name = FILTER_NAME,
	.label = PLUGIN_NAME,
	.information = PLUGIN_INFO_FMT(FILTER_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR),
	.items = const_cast<void**>(params::all),
	.func_proc_video = &filter_image,
	.func_proc_audio = &filter_sound,
};
