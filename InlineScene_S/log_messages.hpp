/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct CONFIG_HANDLE;
struct EDIT_SECTION;
struct OBJECT_INFO;

namespace InlineScene_S::Messages
{
	struct list {
		static wchar_t const* error_cache_not_found();
		static wchar_t const* error_cache_deleted();
		static wchar_t const* error_managed_cache_deleted();
		static wchar_t const* error_stack_modified();
		static wchar_t const* error_none_opened();
		static wchar_t const* error_left_unclosed();
		static wchar_t const* error_multi_obj();
		static wchar_t const* error_camera_mode();
		static wchar_t const* error_clipping_obj();
		static wchar_t const* word_cache_name();
		static wchar_t const* word_position_at();
		static wchar_t const* word_detected_at();
		static wchar_t const* word_video();
		static wchar_t const* word_audio();
	};
	struct helper {
		static wchar_t const* effect_name(::OBJECT_INFO const* object, ::EDIT_SECTION const* edit);
		static wchar_t const* effect_name(auto const* proc) {
			return effect_name(proc->object, proc->edit);
		}
		static std::wstring frame_layer(int frame, int rate, int scale, wchar_t const* scene_name, int layer);
		static std::wstring frame_layer(auto const* proc, int frame, int layer) {
			return frame_layer(frame, proc->scene->rate, proc->scene->scale, proc->edit->get_scene_name(), layer);
		}
		static std::wstring frame_layers(int frame, int rate, int scale, wchar_t const* scene_name, std::vector<int> const& layers);
		static std::wstring frame_layers(auto const* proc, int frame, std::vector<int> const& layers) {
			return frame_layers(frame, proc->scene->rate, proc->scene->scale, proc->edit->get_scene_name(), layers);
		}
	};
}
