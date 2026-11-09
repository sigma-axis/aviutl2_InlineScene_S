/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filter2.h>

#include "InlineScene_S.hpp"
#include "InlineScene_Cache.hpp"
#include "Filters_Common.hpp"
#include "Filter_Push.hpp"
using namespace InlineScene_S;
namespace Common = InlineScene_S::Filters::Common;

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B
////////////////////////////////
// core functionalities.
////////////////////////////////
bool filter_image(::FILTER_PROC_VIDEO* video)
{
	if (!Common::verify_open_close_image(video)) return false;

	auto& stack = Cache::Image::scene_stacks[video->scene->id];
	stack.scene_id = video->scene->id;
	return stack.push(video);
}
void filter_sound(::FILTER_PROC_AUDIO* audio)
{
	auto& stack = Cache::Sound::scene_stacks[audio->scene->id];
	stack.scene_id = audio->scene->id;
	stack.push(audio);
}

////////////////////////////////
// setting parameters.
////////////////////////////////
namespace params_obj
{
	constexpr void* all[] = {
		nullptr,
	};
}
namespace params_anm
{
	constexpr void* all[] = {
		nullptr,
	};
}

////////////////////////////////
// entry points.
////////////////////////////////
bool filter_obj_image(::FILTER_PROC_VIDEO* video)
{
	filter_image(video);
	return false;
}
bool filter_anm_image(::FILTER_PROC_VIDEO* video)
{
	return filter_image(video);
}
bool filter_obj_sound(::FILTER_PROC_AUDIO* audio)
{
	filter_sound(audio);
	return false;
}
bool filter_anm_sound(::FILTER_PROC_AUDIO* audio)
{
	filter_sound(audio);
	return true;
}

ANON_NS_E

////////////////////////////////
// exports.
////////////////////////////////
#define FILTER_NAME_OBJ	L"InlineScene_Sここから"
constinit ::FILTER_PLUGIN_TABLE InlineScene_S::Filters::push_obj{
	.flag = ::FILTER_PLUGIN_TABLE::FLAG_VIDEO | ::FILTER_PLUGIN_TABLE::FLAG_AUDIO | ::FILTER_PLUGIN_TABLE::FLAG_INPUT,
	.name = FILTER_NAME_OBJ,
	.label = PLUGIN_NAME,
	.information = PLUGIN_INFO_FMT(FILTER_NAME_OBJ, PLUGIN_VERSION, PLUGIN_AUTHOR),
	.items = const_cast<void**>(params_obj::all),
	.func_proc_video = &filter_obj_image,
	.func_proc_audio = &filter_obj_sound,
};

#define FILTER_NAME_ANM	L"InlineScene_Sこのオブジェクトから"
constinit ::FILTER_PLUGIN_TABLE InlineScene_S::Filters::push_anm{
	.flag = ::FILTER_PLUGIN_TABLE::FLAG_VIDEO | ::FILTER_PLUGIN_TABLE::FLAG_AUDIO,
	.name = FILTER_NAME_ANM,
	.label = PLUGIN_NAME,
	.information = PLUGIN_INFO_FMT(FILTER_NAME_ANM, PLUGIN_VERSION, PLUGIN_AUTHOR),
	.items = const_cast<void**>(params_anm::all),
	.func_proc_video = &filter_anm_image,
	.func_proc_audio = &filter_anm_sound,
};
