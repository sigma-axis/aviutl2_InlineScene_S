/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <chrono>

#include "d3d_service.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filter2.h>

namespace InlineScene_S::Cache
{
	namespace Image
	{
		enum class cache_type {
			Shared = 0,
			Managed = 1,
		};
		struct crop_desc {
			bool crop_l, crop_r, crop_t, crop_b;
			int pad_l, pad_r, pad_t, pad_b;
		};

		struct scene_stack {
			int scene_id{};
			int main_pos{}; // ::OBJECT_INFO::origin_frame
			int64_t main_render_id{};
			std::vector<int> main{}; // array of ::OBJECT_INFO::effect_layer + 1.
			int64_t ref_render_id{};
			size_t ref_count{};

			void trim(::FILTER_PROC_VIDEO* video);
			bool push(::FILTER_PROC_VIDEO* video);
			bool pop(::FILTER_PROC_VIDEO* video, crop_desc const& crop);
			bool pop(::FILTER_PROC_VIDEO* video, std::wstring_view const& name, cache_type type, crop_desc const& crop);
			bool next(::FILTER_PROC_VIDEO* video, std::wstring_view const& name, cache_type type, crop_desc const& crop);
			static std::wstring get_cache_name(int scene_id, bool is_ref, size_t index);
		};
		inline std::map<int, scene_stack> scene_stacks = {};

		struct scene_cache_item {
			int width{}, height{};
			::OBJECT_IMAGE_PARAM params{};
		};
		struct scene_cache_list {
			int scene_id{};
			int main_pos{}; // ::OBJECT_INFO::origin_frame
			int64_t main_render_id{};
			std::map<std::wstring, scene_cache_item> main{};
			int64_t ref_render_id{};
			std::map<std::wstring, scene_cache_item> ref{};

			void trim(::FILTER_PROC_VIDEO* video);
			bool save(::FILTER_PROC_VIDEO* video, std::wstring_view const& name, std::wstring const& source);
			bool load(::FILTER_PROC_VIDEO* video, std::wstring_view const& name, std::wstring const& dest, bool restore_params, bool composite_rot);
		};
		inline std::map<int, scene_cache_list> scene_cache = {};

		struct managed_cache_item {
			using clock = std::chrono::steady_clock;
			Microsoft::WRL::ComPtr<::ID3D11Texture2D> buffer{};
			int width{}, height{};
			::OBJECT_IMAGE_PARAM params{};
			clock::time_point last_used{};
			constexpr bool is_deleted() const { return buffer == nullptr; }
		};
		struct managed_cache_list {
			using clock = managed_cache_item::clock;
			std::map<std::wstring, managed_cache_item> list{};
			clock::duration duration{};
			int64_t max_total_dots{};
			void trim(clock::time_point now);
			void trim() { trim(clock::now()); };
			void clear() { list.clear(); }
			bool save(::FILTER_PROC_VIDEO* video, std::wstring const& name, std::wstring const& source);
			bool load(::FILTER_PROC_VIDEO* video, std::wstring const& name, std::wstring const& dest, bool restore_params, bool composite_rot);
		};
		inline managed_cache_list  managed_cache = {
			.duration = std::chrono::minutes(1),
			.max_total_dots = 1,
		};
	}

	namespace Sound
	{
		struct scene_stack_item {
			int layer{}; // ::OBJECT_INFO::effect_layer + 1.
			std::vector<float> buffer[2]{};
		};
		struct scene_stack {
			int scene_id{};
			int main_pos{}; // ::OBJECT_INFO::origin_frame
			int64_t main_render_id{};
			std::vector<scene_stack_item> main{};
			int64_t ref_render_id{};
			std::vector<scene_stack_item> ref{};

			void trim(::FILTER_PROC_AUDIO* audio);
			void push(::FILTER_PROC_AUDIO* audio);
			bool pop(::FILTER_PROC_AUDIO* audio);
			bool pop(::FILTER_PROC_AUDIO* audio, std::wstring const& name);
			bool next(::FILTER_PROC_AUDIO* audio, std::wstring const& name);
		};
		inline std::map<int, scene_stack> scene_stacks = {};

		struct cache_item {
			std::vector<float> buffer[2]{};
			::OBJECT_AUDIO_PARAM params{};
		};
		struct scene_cache_list {
			int scene_id{};
			int main_pos{}; // ::OBJECT_INFO::origin_frame
			int64_t main_render_id{};
			std::map<std::wstring, cache_item> main{};
			int64_t ref_render_id{};
			std::map<std::wstring, cache_item> ref{};

			void trim(::FILTER_PROC_AUDIO* audio);
			void save(::FILTER_PROC_AUDIO* audio, std::wstring const& name);
			bool load(::FILTER_PROC_AUDIO* audio, std::wstring const& name, bool restore_params);
		};
		inline std::map<int, scene_cache_list> scene_cache = {};
	}
}
