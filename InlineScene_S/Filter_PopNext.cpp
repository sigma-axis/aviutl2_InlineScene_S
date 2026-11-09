/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <string>
#include <format>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filter2.h>
#include <plugin2.h>

#include "InlineScene_S.hpp"
#include "InlineScene_Cache.hpp"
#include "Filters_Common.hpp"
#include "Filter_PopNext.hpp"
using namespace InlineScene_S;
namespace Common = InlineScene_S::Filters::Common;

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B
////////////////////////////////
// core functionalities.
////////////////////////////////
enum class pop_type {
	next,
	to_obj,
	others,
};
bool filter_image(::FILTER_PROC_VIDEO* video, pop_type pop_type, std::wstring_view const& name,
	Cache::Image::cache_type cache_type, Cache::Image::crop_desc const& crop)
{
	if (!Common::verify_open_close_image(video)) return false;

	auto& stack = Cache::Image::scene_stacks[video->scene->id];
	stack.scene_id = video->scene->id;
	switch (pop_type) {
	case pop_type::next: return stack.next(video, name, cache_type, crop);
	default:
	case pop_type::to_obj: return stack.pop(video, crop);
	case pop_type::others: return stack.pop(video, name, cache_type, crop);
	}
}
bool filter_sound(::FILTER_PROC_AUDIO* audio, pop_type pop_type,
	std::wstring const& name, Cache::Image::cache_type cache_type)
{
	auto& stack = Cache::Sound::scene_stacks[audio->scene->id];
	stack.scene_id = audio->scene->id;
	switch (pop_type) {
	case pop_type::next: return stack.next(audio, name);
	default:
	case pop_type::to_obj: return stack.pop(audio);
	case pop_type::others: return stack.pop(audio, name);
	}
}

////////////////////////////////
// setting parameters.
////////////////////////////////
namespace params_pop_obj
{
	::FILTER_ITEM_CHECK show_as_obj{ L"オブジェクトとして表示・再生", false };
	::FILTER_ITEM_SELECT cache_type{ Common::cache_type::option_name, static_cast<int>(Common::cache_type::default_value), Common::cache_type::list };
	::FILTER_ITEM_STRING cache_name{ L"保存名", L"scene1" };

	::FILTER_ITEM_SELECT crop_mode{ Common::crop_mode_image::option_name, Common::crop_mode_image::default_value, Common::crop_mode_image::list };
	::FILTER_ITEM_CHECK_SECTION crop_l{ L"crop::左", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_r{ L"crop::右", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_t{ L"crop::上", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_b{ L"crop::下", true, false };

	::FILTER_ITEM_GROUP group_pad{ L"追加余白", false };
	::FILTER_ITEM_TRACK pad_l{ L"pad::左", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_r{ L"pad::右", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_t{ L"pad::上", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_b{ L"pad::下", 0, -4000, 4000, 1, nullptr, 0.05 };

	// hide rules.
	namespace hide_rules
	{
		using enum ::FILTER_ITEM_HIDE_RULE::OPERATOR;
		::FILTER_ITEM_HIDE_RULE hide_cache_type{ cache_type.name, show_as_obj.name, EQUAL, 1 };
		::FILTER_ITEM_HIDE_RULE hide_cache_name{ cache_name.name, show_as_obj.name, EQUAL, 1 };
		::FILTER_ITEM_HIDE_RULE hide_crop_l{ crop_l.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_r{ crop_r.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_t{ crop_t.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_b{ crop_b.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };

		::FILTER_ITEM_HIDE_RULE hide_out_cache_type{ cache_type.name, L"output", EQUAL, 2};
		::FILTER_ITEM_HIDE_RULE hide_out_crop_mode{ crop_mode.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_l{ crop_l.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_r{ crop_r.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_t{ crop_t.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_b{ crop_b.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_group_pad{ group_pad.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_l{ pad_l.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_r{ pad_r.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_t{ pad_t.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_b{ pad_b.name, L"output", EQUAL, 2 };
	}

	constexpr void* all[] = {
		&show_as_obj,
		&cache_type,
		&cache_name,

		&crop_mode,
		&crop_l,
		&crop_r,
		&crop_t,
		&crop_b,

		&group_pad,
		&pad_l,
		&pad_r,
		&pad_t,
		&pad_b,

		&hide_rules::hide_cache_type,
		&hide_rules::hide_cache_name,
		&hide_rules::hide_crop_l,
		&hide_rules::hide_crop_r,
		&hide_rules::hide_crop_t,
		&hide_rules::hide_crop_b,

		&hide_rules::hide_out_cache_type,
		&hide_rules::hide_out_crop_mode,
		&hide_rules::hide_out_crop_l,
		&hide_rules::hide_out_crop_r,
		&hide_rules::hide_out_crop_t,
		&hide_rules::hide_out_crop_b,

		&hide_rules::hide_out_group_pad,
		&hide_rules::hide_out_pad_l,
		&hide_rules::hide_out_pad_r,
		&hide_rules::hide_out_pad_t,
		&hide_rules::hide_out_pad_b,

		nullptr,
	};
}
namespace params_next_obj
{
	::FILTER_ITEM_SELECT cache_type{ Common::cache_type::option_name, static_cast<int>(Common::cache_type::default_value), Common::cache_type::list };
	::FILTER_ITEM_STRING cache_name{ L"保存名", L"scene1" };

	::FILTER_ITEM_SELECT crop_mode{ Common::crop_mode_image::option_name, Common::crop_mode_image::default_value, Common::crop_mode_image::list };
	::FILTER_ITEM_CHECK_SECTION crop_l{ L"crop::左", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_r{ L"crop::右", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_t{ L"crop::上", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_b{ L"crop::下", true, false };

	::FILTER_ITEM_GROUP group_pad{ L"追加余白", false };
	::FILTER_ITEM_TRACK pad_l{ L"pad::左", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_r{ L"pad::右", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_t{ L"pad::上", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_b{ L"pad::下", 0, -4000, 4000, 1, nullptr, 0.05 };

	// hide rules.
	namespace hide_rules
	{
		using enum ::FILTER_ITEM_HIDE_RULE::OPERATOR;
		::FILTER_ITEM_HIDE_RULE hide_crop_l{ crop_l.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_r{ crop_r.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_t{ crop_t.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_b{ crop_b.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };

		::FILTER_ITEM_HIDE_RULE hide_out_cache_type{ cache_type.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_mode{ crop_mode.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_l{ crop_l.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_r{ crop_r.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_t{ crop_t.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_b{ crop_b.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_group_pad{ group_pad.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_l{ pad_l.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_r{ pad_r.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_t{ pad_t.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_b{ pad_b.name, L"output", EQUAL, 2 };
	}

	constexpr void* all[] = {
		&cache_type,
		&cache_name,

		&crop_mode,
		&crop_l,
		&crop_r,
		&crop_t,
		&crop_b,

		&group_pad,
		&pad_l,
		&pad_r,
		&pad_t,
		&pad_b,

		&hide_rules::hide_crop_l,
		&hide_rules::hide_crop_r,
		&hide_rules::hide_crop_t,
		&hide_rules::hide_crop_b,

		&hide_rules::hide_out_cache_type,
		&hide_rules::hide_out_crop_mode,
		&hide_rules::hide_out_crop_l,
		&hide_rules::hide_out_crop_r,
		&hide_rules::hide_out_crop_t,
		&hide_rules::hide_out_crop_b,

		&hide_rules::hide_out_group_pad,
		&hide_rules::hide_out_pad_l,
		&hide_rules::hide_out_pad_r,
		&hide_rules::hide_out_pad_t,
		&hide_rules::hide_out_pad_b,

		nullptr,
	};
}
namespace params_next_anm
{
	::FILTER_ITEM_SELECT cache_type{ Common::cache_type::option_name, static_cast<int>(Common::cache_type::default_value), Common::cache_type::list };
	::FILTER_ITEM_STRING cache_name{ L"保存名", L"scene1" };

	::FILTER_ITEM_SELECT crop_mode{ Common::crop_mode_image::option_name, Common::crop_mode_image::default_value, Common::crop_mode_image::list };
	::FILTER_ITEM_CHECK_SECTION crop_l{ L"crop::左", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_r{ L"crop::右", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_t{ L"crop::上", true, false };
	::FILTER_ITEM_CHECK_SECTION crop_b{ L"crop::下", true, false };

	::FILTER_ITEM_GROUP group_pad{ L"追加余白", false };
	::FILTER_ITEM_TRACK pad_l{ L"pad::左", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_r{ L"pad::右", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_t{ L"pad::上", 0, -4000, 4000, 1, nullptr, 0.05 };
	::FILTER_ITEM_TRACK pad_b{ L"pad::下", 0, -4000, 4000, 1, nullptr, 0.05 };

	// hide rules.
	namespace hide_rules
	{
		using enum ::FILTER_ITEM_HIDE_RULE::OPERATOR;
		::FILTER_ITEM_HIDE_RULE hide_crop_l{ crop_l.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_r{ crop_r.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_t{ crop_t.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };
		::FILTER_ITEM_HIDE_RULE hide_crop_b{ crop_b.name, crop_mode.name, NOT_EQUAL, Common::crop_mode_image::per_edges };

		::FILTER_ITEM_HIDE_RULE hide_out_cache_type{ cache_type.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_mode{ crop_mode.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_l{ crop_l.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_r{ crop_r.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_t{ crop_t.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_crop_b{ crop_b.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_group_pad{ group_pad.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_l{ pad_l.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_r{ pad_r.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_t{ pad_t.name, L"output", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_out_pad_b{ pad_b.name, L"output", EQUAL, 2 };

		::FILTER_ITEM_HIDE_RULE hide_in_cache_type{ cache_type.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_crop_mode{ crop_mode.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_crop_l{ crop_l.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_crop_r{ crop_r.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_crop_t{ crop_t.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_crop_b{ crop_b.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_group_pad{ group_pad.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_pad_l{ pad_l.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_pad_r{ pad_r.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_pad_t{ pad_t.name, L"input", EQUAL, 2 };
		::FILTER_ITEM_HIDE_RULE hide_in_pad_b{ pad_b.name, L"input", EQUAL, 2 };
	}

	constexpr void* all[] = {
		&cache_type,
		&cache_name,

		&crop_mode,
		&crop_l,
		&crop_r,
		&crop_t,
		&crop_b,

		&group_pad,
		&pad_l,
		&pad_r,
		&pad_t,
		&pad_b,

		&hide_rules::hide_crop_l,
		&hide_rules::hide_crop_r,
		&hide_rules::hide_crop_t,
		&hide_rules::hide_crop_b,

		&hide_rules::hide_out_cache_type,
		&hide_rules::hide_out_crop_mode,
		&hide_rules::hide_out_crop_l,
		&hide_rules::hide_out_crop_r,
		&hide_rules::hide_out_crop_t,
		&hide_rules::hide_out_crop_b,

		&hide_rules::hide_out_group_pad,
		&hide_rules::hide_out_pad_l,
		&hide_rules::hide_out_pad_r,
		&hide_rules::hide_out_pad_t,
		&hide_rules::hide_out_pad_b,

		&hide_rules::hide_in_cache_type,
		&hide_rules::hide_in_crop_mode,
		&hide_rules::hide_in_crop_l,
		&hide_rules::hide_in_crop_r,
		&hide_rules::hide_in_crop_t,
		&hide_rules::hide_in_crop_b,

		&hide_rules::hide_in_group_pad,
		&hide_rules::hide_in_pad_l,
		&hide_rules::hide_in_pad_r,
		&hide_rules::hide_in_pad_t,
		&hide_rules::hide_in_pad_b,

		nullptr,
	};
}

////////////////////////////////
// entry points.
////////////////////////////////
bool filter_pop_obj_image(::FILTER_PROC_VIDEO* video)
{
	using namespace params_pop_obj;
	pop_type pop_type = pop_type::others;
	Cache::Image::cache_type const cache_type =
		static_cast<Cache::Image::cache_type>(params_pop_obj::cache_type.value);
	if (show_as_obj.value) pop_type = pop_type::to_obj;
	Cache::Image::crop_desc crop{
		crop_l.value, crop_r.value, crop_t.value, crop_b.value,
		std::lround(pad_l.value), std::lround(pad_r.value),
		std::lround(pad_t.value), std::lround(pad_b.value),
	};
	if (crop_mode.value == Common::crop_mode_image::crop)
		crop.crop_l = crop.crop_r = crop.crop_t = crop.crop_b = true;
	else if (crop_mode.value == Common::crop_mode_image::leave)
		crop.crop_l = crop.crop_r = crop.crop_t = crop.crop_b = false;
	return filter_image(video, pop_type, cache_name.value, cache_type, crop);
}
bool filter_pop_obj_sound(::FILTER_PROC_AUDIO* audio)
{
	using namespace params_pop_obj;
	pop_type pop_type = pop_type::others;
	Cache::Image::cache_type cache_type =
		static_cast<Cache::Image::cache_type>(params_pop_obj::cache_type.value);
	if (show_as_obj.value) pop_type = pop_type::to_obj;
	return filter_sound(audio, pop_type, cache_name.value, cache_type);
}
bool filter_next_obj_image(::FILTER_PROC_VIDEO* video)
{
	using namespace params_next_obj;
	Cache::Image::crop_desc crop{
		crop_l.value, crop_r.value, crop_t.value, crop_b.value,
		std::lround(pad_l.value), std::lround(pad_r.value),
		std::lround(pad_t.value), std::lround(pad_b.value),
	};
	if (crop_mode.value == Common::crop_mode_image::crop)
		crop.crop_l = crop.crop_r = crop.crop_t = crop.crop_b = true;
	else if (crop_mode.value == Common::crop_mode_image::leave)
		crop.crop_l = crop.crop_r = crop.crop_t = crop.crop_b = false;
	return filter_image(video, pop_type::next, cache_name.value,
		static_cast<Cache::Image::cache_type>(cache_type.value), crop);
}
bool filter_next_obj_sound(::FILTER_PROC_AUDIO* audio)
{
	using namespace params_next_obj;
	return filter_sound(audio, pop_type::next, cache_name.value, static_cast<Cache::Image::cache_type>(cache_type.value));
}
bool filter_next_anm_image(::FILTER_PROC_VIDEO* video)
{
	using namespace params_next_anm;
	Cache::Image::crop_desc crop{
		crop_l.value, crop_r.value, crop_t.value, crop_b.value,
		std::lround(pad_l.value), std::lround(pad_r.value),
		std::lround(pad_t.value), std::lround(pad_b.value),
	};
	if (crop_mode.value == Common::crop_mode_image::crop)
		crop.crop_l = crop.crop_r = crop.crop_t = crop.crop_b = true;
	else if (crop_mode.value == Common::crop_mode_image::leave)
		crop.crop_l = crop.crop_r = crop.crop_t = crop.crop_b = false;
	return filter_image(video, pop_type::next, cache_name.value,
		static_cast<Cache::Image::cache_type>(cache_type.value), crop);
}
bool filter_next_anm_sound(::FILTER_PROC_AUDIO* audio)
{
	using namespace params_next_anm;
	return filter_sound(audio, pop_type::next, cache_name.value, static_cast<Cache::Image::cache_type>(cache_type.value));
}

ANON_NS_E

////////////////////////////////
// exports.
////////////////////////////////
#define FILTER_NAME_POP_OBJ	L"InlineScene_Sここまで"
constinit ::FILTER_PLUGIN_TABLE InlineScene_S::Filters::pop_obj{
	.flag = ::FILTER_PLUGIN_TABLE::FLAG_VIDEO | ::FILTER_PLUGIN_TABLE::FLAG_AUDIO | ::FILTER_PLUGIN_TABLE::FLAG_INPUT,
	.name = FILTER_NAME_POP_OBJ,
	.label = PLUGIN_NAME,
	.information = PLUGIN_INFO_FMT(FILTER_NAME_POP_OBJ, PLUGIN_VERSION, PLUGIN_AUTHOR),
	.items = const_cast<void**>(params_pop_obj::all),
	.func_proc_video = &filter_pop_obj_image,
	.func_proc_audio = &filter_pop_obj_sound,
};

#define FILTER_NAME_NEXT_OBJ	L"InlineScene_S次へ"
constinit ::FILTER_PLUGIN_TABLE InlineScene_S::Filters::next_obj{
	.flag = ::FILTER_PLUGIN_TABLE::FLAG_VIDEO | ::FILTER_PLUGIN_TABLE::FLAG_AUDIO | ::FILTER_PLUGIN_TABLE::FLAG_INPUT,
	.name = FILTER_NAME_NEXT_OBJ,
	.label = PLUGIN_NAME,
	.information = PLUGIN_INFO_FMT(FILTER_NAME_NEXT_OBJ, PLUGIN_VERSION, PLUGIN_AUTHOR),
	.items = const_cast<void**>(params_next_obj::all),
	.func_proc_video = &filter_next_obj_image,
	.func_proc_audio = &filter_next_obj_sound,
};

#define FILTER_NAME_NEXT_ANM	L"InlineScene_Sこのオブジェクトで次へ"
constinit ::FILTER_PLUGIN_TABLE InlineScene_S::Filters::next_anm{
	.flag = ::FILTER_PLUGIN_TABLE::FLAG_VIDEO | ::FILTER_PLUGIN_TABLE::FLAG_AUDIO,
	.name = FILTER_NAME_NEXT_ANM,
	.label = PLUGIN_NAME,
	.information = PLUGIN_INFO_FMT(FILTER_NAME_NEXT_ANM, PLUGIN_VERSION, PLUGIN_AUTHOR),
	.items = const_cast<void**>(params_next_anm::all),
	.func_proc_video = &filter_next_anm_image,
	.func_proc_audio = &filter_next_anm_sound,
};
