/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <cmath>
#include <numbers>
#include <memory>
#include <vector>
#include <string>
using namespace std::string_view_literals;

#include <format>
#include <ranges>
#include <algorithm>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filter2.h>
#include <plugin2.h>

#include "logging.hpp"
#include "finalizing.hpp"
#include "d3d_service.hpp"
using D3D = d3d_service::D3D;

#include "find_bound.hpp"
#include "InlineScene_S.hpp"
#include "InlineScene_Cache.hpp"
using namespace InlineScene_S::Cache;

#include "logging.hpp"
namespace logging = AviUtl2::logging;
#include "log_messages.hpp"
namespace Mes = InlineScene_S::Messages;

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B;
////////////////////////////////
// Resource managements.
////////////////////////////////
constinit AviUtl2::finalizing::helpers::init_state init_state{};
void quit()
{
	Image::scene_cache.clear();
	Image::managed_cache.clear();
	Sound::scene_cache.clear();

	init_state.clear();
}
bool init()
{
	init_state.init(&quit, []{ return true; });
	return init_state;
}


////////////////////////////////
// helper functions.
////////////////////////////////
struct angle_euler { double x, y, z; }; // in degree; rotation in the order z->y->x.
angle_euler composite_euler_angles(angle_euler const& rot2, angle_euler const& rot1)
{
	double a, c, s, qr, qi, qj, qk, Qr, Qi, Qj, Qk;
	a = std::numbers::pi / 360 * rot1.z;
	c = std::cos(a); s = std::sin(a);
	qr = c; qk = s;

	a = std::numbers::pi / 360 * rot1.y;
	c = std::cos(a); s = std::sin(a);
	Qr = c * qr; Qi = s * qk; Qj = s * qr; Qk = c * qk;

	a = std::numbers::pi / 360 * rot1.x;
	c = std::cos(a); s = std::sin(a);
	qr = c * Qr - s * Qi; qi = c * Qi + s * Qr;
	qj = c * Qj - s * Qk; qk = c * Qk + s * Qj;

	a = std::numbers::pi / 360 * rot2.z;
	c = std::cos(a); s = std::sin(a);
	Qr = c * qr - s * qk; Qi = c * qi - s * qj;
	Qj = c * qj + s * qi; Qk = c * qk + s * qr;

	a = std::numbers::pi / 360 * rot2.y;
	c = std::cos(a); s = std::sin(a);
	qr = c * Qr - s * Qj; qi = c * Qi + s * Qk;
	qj = c * Qj + s * Qr; qk = c * Qk - s * Qi;

	a = std::numbers::pi / 360 * rot2.x;
	c = std::cos(a); s = std::sin(a);
	Qr = c * qr - s * qi; Qi = c * qi + s * qr;
	Qj = c * qj - s * qk; Qk = c * qk + s * qj;

	// cf.) https://github.com/sigma-axis/sigma_aviutl_scripts/blob/79bce1098a69b1e8da7cb77811aa3356a411ee2d/sigma_rot_helper.lua#L80-L94
	auto calc_angle = [](double x1, double y1, double x2, double y2) {
		if (x1 * x1 + y1 * y1 < x2 * x2 + y2 * y2) { x1 = x2; y1 = y2; }
		if (x1 < 0) { x1 = -x1; y1 = -y1; }
		return 2 * std::atan2(y1, x1);
	};
	a = std::atan2(2 * (Qr * Qi - Qj * Qk),
		Qr * Qr - Qi * Qi - Qj * Qj + Qk * Qk);
	c = std::cos(a / 2); s = std::sin(a / 2);
	qr = c * Qr + s * Qi; qi = c * Qi - s * Qr;
	qj = c * Qj + s * Qk; qk = c * Qk - s * Qj;
	c = calc_angle(qr, qj, qk, qi); s = calc_angle(qr, qk, qj, qi);
	return {
		180 / std::numbers::pi * a,
		180 / std::numbers::pi * c,
		180 / std::numbers::pi * s,
	};
}

::OBJECT_IMAGE_PARAM get_object_image_param(::FILTER_PROC_VIDEO* video, bool is_obj = true)
{
	::OBJECT_IMAGE_PARAM ret{
		.x = 0, .y = 0, .z = 0,
		.rx = 0, .ry = 0, .rz = 0,
		.sx = 1, .sy = 1, .sz = 1,
		.cx = 0, .cy = 0, .cz = 0,
		.alpha = 1,
	};
	if (is_obj) {
		ret = *video->param;
		if (::OBJECT_IMAGE_PARAM p_out;
			video->get_output_image_param(video->object->handle, 0, &p_out, sizeof(p_out)))
			ret = {
				.x = ret.x + p_out.x, .y = ret.y + p_out.y, .z = ret.z + p_out.z,
				.rx = ret.rx + p_out.rx, .ry = ret.ry + p_out.ry, .rz = ret.rz + p_out.rz,
				.sx = ret.sx * p_out.sx, .sy = ret.sy * p_out.sy, .sz = ret.sz * p_out.sz,
				.cx = ret.cx + p_out.cx, .cy = ret.cy + p_out.cy, .cz = ret.cz + p_out.cz,
				.alpha = ret.alpha * p_out.alpha,
			};
	}
	return ret;
}

::OBJECT_AUDIO_PARAM get_object_audio_param(::FILTER_PROC_AUDIO* audio, bool is_obj = true)
{
	::OBJECT_AUDIO_PARAM ret{
		.vol_l = 1, .vol_r = 1,
	};
	if (is_obj) {
		ret = *audio->param;
		if (::OBJECT_AUDIO_PARAM p_out;
			audio->get_output_audio_param(audio->object->handle, 0, &p_out, sizeof(p_out)))
			ret = {
				.vol_l = ret.vol_l * p_out.vol_l,
				.vol_r = ret.vol_r * p_out.vol_r,
			};
	}
	return ret;
}

////////////////////////////////
// core functionalities.
////////////////////////////////
// Image::scene_cache_list
void trim_scene_cache_image(::FILTER_PROC_VIDEO* video, Image::scene_cache_list& self, bool is_ref, int64_t render_id)
{
	if (!is_ref || self.ref_render_id != render_id) {
		self.ref_render_id = render_id;
		self.ref.clear();
	}
	if (!is_ref && self.main_render_id != render_id) {
		self.main_pos = video->object->origin_frame;
		self.main_render_id = render_id;
		self.main.clear();
	}
}

bool save_scene_cache_image(::FILTER_PROC_VIDEO* video, Image::scene_cache_list& self, bool is_ref, int64_t render_id,
	std::wstring const& name, std::wstring const& source)
{
	auto& map = is_ref ? self.ref : self.main;

	auto const params = get_object_image_param(video, source == L"object"sv);
	int width, height;
	bool success = true;
	success &= video->get_image_resource_size(source.c_str(), &width, &height);
	success &= video->copy_image_resource(name.c_str(), source.c_str());

	if (success) map[name] = { .width = width, .height = height, .params = params };
	return success;
}
bool load_scene_cache_image(::FILTER_PROC_VIDEO* video, Image::scene_cache_list& list, bool is_ref, int64_t render_id,
	std::wstring const& name, std::wstring const& dest, bool restore_params, bool composite_rot)
{
	auto& map = is_ref ? list.ref : list.main;
	auto const it = map.find(name);
	if (it == map.end()) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {:?}, {}: {}.",
			Mes::list::error_cache_not_found(),
			Mes::helper::effect_name(video), Mes::list::word_video(),
			Mes::list::word_cache_name(), name,
			Mes::list::word_position_at(), Mes::helper::frame_layer(video,
				video->object->origin_frame, video->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		return false;
	}
	auto& cache = it->second;

	if (!video->copy_image_resource(dest.c_str(), name.c_str())) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {:?}, {}: {}.",
			Mes::list::error_cache_deleted(),
			Mes::helper::effect_name(video), Mes::list::word_video(),
			Mes::list::word_cache_name(), name,
			Mes::list::word_position_at(), Mes::helper::frame_layer(video,
				video->object->origin_frame, video->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		return false;
	}

	if (dest == L"object"sv && restore_params) {
		*video->param = cache.params;
		if (::OBJECT_IMAGE_PARAM p_out;
			composite_rot &&
			video->get_output_image_param(video->object->handle, 0, &p_out, sizeof(p_out))) {
			auto [rx, ry, rz] = composite_euler_angles(
				{ p_out.rx, p_out.ry, p_out.rz },
				{ cache.params.rx, cache.params.ry, cache.params.rz });
			video->param->rx = static_cast<float>(rx - p_out.rx);
			video->param->ry = static_cast<float>(ry - p_out.ry);
			video->param->rz = static_cast<float>(rz - p_out.rz);
		}
	}
	return true;
}

// Image::scene_stack
void trim_stack_image(::FILTER_PROC_VIDEO* video, Image::scene_stack& self, bool is_ref, int64_t render_id)
{
	if (!is_ref || self.ref_render_id != render_id) {
		self.ref_render_id = render_id;
		self.ref_count = 0;
	}
	if (!is_ref && self.main_render_id != render_id) {
		if (self.main.size() > 0) {
			auto const mes = std::format(L"{}\n> [{} / {}] {}: {}, {}: {}.",
				Mes::list::error_left_unclosed(),
				Mes::helper::effect_name(video), Mes::list::word_video(),
				Mes::list::word_position_at(), Mes::helper::frame_layers(video, self.main_pos, self.main),
				Mes::list::word_detected_at(), Mes::helper::frame_layer(video,
					video->object->origin_frame, video->object->effect_layer + 1));
			AviUtl2::logging::warn(mes.c_str());
		}
		self.main_pos = video->object->origin_frame;
		self.main_render_id = render_id;
		self.main.clear();
	}
}

bool push_stack_image(::FILTER_PROC_VIDEO* video, Image::scene_stack& self, bool is_ref, int64_t render_id)
{
	size_t const index = is_ref ?
		(++self.ref_count) :
		(self.main.push_back(video->object->effect_layer + 1),
			static_cast<int>(self.main.size()));

	auto const cache_name = Image::scene_stack::get_cache_name(self.scene_id, is_ref, index);
	bool success = video->copy_image_resource(cache_name.c_str(), L"framebuffer");
	auto const framebuffer = video->get_framebuffer_texture2d();
	D3D::cxt->ClearRenderTargetView(D3D::to_render_target_view(framebuffer).Get(), D3D::zero_color);
	return success;
}

enum class pop_mode {
	next,
	to_obj,
	others,
};
bool pop_next_stack_image(::FILTER_PROC_VIDEO* video, Image::scene_stack& self, bool is_ref, int64_t render_id,
	pop_mode pop_mode, std::wstring_view const& name, Image::cache_type type, Image::crop_desc const& crop)
{
	size_t const index = is_ref ? self.ref_count : self.main.size();
	if (index == 0) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {}.",
			Mes::list::error_none_opened(),
			Mes::helper::effect_name(video), Mes::list::word_video(),
			Mes::list::word_position_at(), Mes::helper::frame_layer(video,
				video->object->origin_frame, video->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		return false;
	}
	if (pop_mode != pop_mode::next) {
		if (is_ref) self.ref_count--;
		else self.main.pop_back();
	}

	auto const framebuffer = video->get_framebuffer_texture2d();
	int pad_l = crop.pad_l, pad_r = crop.pad_r, pad_t = crop.pad_t, pad_b = crop.pad_b;
	bool is_empty = false;
	if (crop.crop_l || crop.crop_r || crop.crop_t || crop.crop_b) {
		auto bb = InlineScene_S::FindBound::Find(framebuffer, video->scene->width, video->scene->height);
		if (bb.is_empty()) is_empty = true;
		else {
			if (crop.crop_l) pad_l -= bb.left;
			if (crop.crop_r) pad_r -= video->scene->width - bb.right;
			if (crop.crop_t) pad_t -= bb.top;
			if (crop.crop_b) pad_b -= video->scene->height - bb.bottom;
		}
	}
	if (video->scene->width + pad_l <= 0 || video->scene->width + pad_r <= 0 ||
		video->scene->width + pad_l + pad_r <= 0 ||
		video->scene->height + pad_t <= 0 || video->scene->height + pad_b <= 0 ||
		video->scene->height + pad_t + pad_b <= 0) is_empty = true;
	D3D::clamp_extension_2d(pad_l, pad_r, video->scene->width);
	D3D::clamp_extension_2d(pad_t, pad_b, video->scene->height);

	float const cx = (pad_l - pad_r) / 2.0f, cy = (pad_t - pad_b) / 2.0f;
	int const width = video->scene->width + pad_l + pad_r, height = video->scene->height + pad_t + pad_b;
	::OBJECT_IMAGE_PARAM const params = {
		.x = 0, .y = 0, .z = 0,
		.rx = 0, .ry = 0, .rz = 0,
		.sx = 1, .sy = 1, .sz = 1,
		.cx = cx, .cy = cy, .cz = 0,
		.alpha = 1,
	};
	bool success = true;
	if (!is_empty && pad_l == 0 && pad_r == 0 && pad_t == 0 && pad_b == 0) {
		if (pop_mode == pop_mode::to_obj)
			success &= video->copy_image_resource(L"object", L"framebuffer");
		else if (type == Image::cache_type::Managed) {
			auto const now = Image::managed_cache_item::clock::now();
			auto& cache = Image::managed_cache.list[std::wstring{ name }];
			if (Image::managed_cache.save_type == Image::managed_cache_type::gpu_mem) {
				if (cache.buffer != nullptr && cache.width == width && cache.height == height)
					D3D::cxt->CopyResource(cache.buffer.Get(), framebuffer);
				else cache = {
					.buffer = D3D::clone(framebuffer),
					.width = width, .height = height,
				};
				cache.data = nullptr;
			}
			else {
				if (cache.data != nullptr && cache.width == width && cache.height == height) /* empty */;
				else cache = {
					.data = std::make_unique<uint64_t[]>(((width + 1) & (~1)) * height),
					.width = width, .height = height,
				};
				cache.buffer = nullptr;
				video->get_image_resource_data(L"framebuffer",
					cache.data.get(), width, height,
					sizeof(uint64_t) * ((width + 1) & (~1)), ::OUTPUT_PIXEL_FORMAT::HF64);
			}
			cache.params = params;
			cache.last_used = now;
			Image::managed_cache.trim(now);
		}
		else {
			auto const cache_name = std::format(L"cache:{}", name);
			success &= video->copy_image_resource(cache_name.c_str(), L"framebuffer");
			if (success) {
				auto& list = Image::scene_cache[self.scene_id];
				trim_scene_cache_image(video, list, is_ref, render_id);
				auto& map = is_ref ? list.ref : list.main;
				map[cache_name] = {
					.width = width, .height = height,
					.params = params,
				};
			}
		}
	}
	else {
		D3D::ComPtr<ID3D11Texture2D> buffer;
		uint64_t* mem_ptr = nullptr;
		if (pop_mode == pop_mode::to_obj) {
			video->create_image_resource(L"object", nullptr, width, height);
			buffer = video->get_image_texture2d();
		}
		else if (type == Image::cache_type::Managed) {
			auto const now = Image::managed_cache_item::clock::now();
			auto& cache = Image::managed_cache.list[std::wstring{ name }];
			if (Image::managed_cache.save_type == Image::managed_cache_type::gpu_mem) {
				if (cache.buffer != nullptr && cache.width == width && cache.height == height) /*empty*/;
				else cache = {
					.buffer = D3D::create_texture(::DXGI_FORMAT_R16G16B16A16_FLOAT, width, height),
					.width = width, .height = height,
				};
				cache.data = nullptr;
				buffer = cache.buffer;
			}
			else {
				if (cache.data != nullptr && cache.width == width && cache.height == height) /*empty*/;
				else cache = {
					.data = std::make_unique<uint64_t[]>(((width + 1) & (~1)) * height),
					.width = width, .height = height,
				};
				cache.buffer = nullptr;
				mem_ptr = cache.data.get();
				buffer = D3D::create_texture(::DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, true);
			}
			cache.params = params;
			cache.last_used = now;
			Image::managed_cache.trim(now);
		}
		else {
			auto const cache_name = std::format(L"cache:{}", name);
			video->create_image_resource(cache_name.c_str(), nullptr, width, height);
			buffer = video->get_image_resource_texture2d(cache_name.c_str());
			success &= buffer != nullptr;
			if (success) {
				auto& list = Image::scene_cache[self.scene_id];
				trim_scene_cache_image(video, list, is_ref, render_id);
				auto& map = is_ref ? list.ref : list.main;
				map[cache_name] = {
					.width = width, .height = height,
					.params = params,
				};
			}
		}
		if (success && buffer != nullptr) {
			if (is_empty) D3D::cxt->ClearRenderTargetView(D3D::to_render_target_view(buffer.Get()).Get(), D3D::zero_color);
			else {
				D3D11_BOX const box{
					.left = static_cast<UINT>(std::max(-pad_l, 0)),
					.top = static_cast<UINT>(std::max(-pad_t, 0)),
					.front = 0,
					.right = static_cast<UINT>(video->scene->width - std::min(pad_r, 0)),
					.bottom = static_cast<UINT>(video->scene->width - std::min(pad_b, 0)),
					.back = 1,
				};
				D3D::cxt->CopySubresourceRegion(buffer.Get(), 0,
					std::max(pad_l, 0), std::max(pad_t, 0), 0,
					framebuffer, 0, &box);
			}

			if (pop_mode != pop_mode::to_obj && type == Image::cache_type::Managed &&
				Image::managed_cache.save_type != Image::managed_cache_type::gpu_mem &&
				mem_ptr != nullptr) {
				if (::D3D11_MAPPED_SUBRESOURCE mapped{};
					S_OK != D3D::cxt->Map(buffer.Get(), 0, D3D11_MAP_READ, 0, &mapped)) {
					logging::error(L"Failed to read from texture!");
					Image::managed_cache.list.erase(std::wstring{ name });
					success = false;
				}
				else {
					int const pitch = (width + 1) & (~1);
					for (int y = 0; y < height; y++) {
						std::memcpy(&mem_ptr[y * pitch],
							&reinterpret_cast<uint8_t*>(mapped.pData)[y * mapped.RowPitch],
							sizeof(uint64_t) * width);
					}
					D3D::cxt->Unmap(buffer.Get(), 0);
				}
			}
		}
	}
	if (!success) return false;
	if (pop_mode == pop_mode::to_obj) {
		video->param->cx += cx;
		video->param->cy += cy;
	}

	if (pop_mode == pop_mode::next)
		D3D::cxt->ClearRenderTargetView(D3D::to_render_target_view(framebuffer).Get(), D3D::zero_color);
	else {
		auto const cache_name = Image::scene_stack::get_cache_name(self.scene_id, is_ref, index);
		auto const cache_buff = video->get_image_resource_texture2d(cache_name.c_str());
		if (cache_buff == nullptr) {
			auto const mes = std::format(L"{}\n> [{} / {}] {}: {:?}, {}: {}.",
				Mes::list::error_cache_deleted(),
				Mes::helper::effect_name(video), Mes::list::word_video(),
				Mes::list::word_cache_name(), cache_name,
				Mes::list::word_position_at(), Mes::helper::frame_layer(video,
					video->object->origin_frame, video->object->effect_layer + 1));
			AviUtl2::logging::warn(mes.c_str());
			return false;
		}

		if (auto const desc = D3D::get_desc(cache_buff);
			static_cast<int>(desc.Width) != video->scene->width ||
			static_cast<int>(desc.Height) != video->scene->height) {
			auto const mes = std::format(L"{}\n> [{} / {}] {}: {:?}, {}: {}.",
				Mes::list::error_stack_modified(),
				Mes::helper::effect_name(video), Mes::list::word_video(),
				Mes::list::word_cache_name(), cache_name,
				Mes::list::word_position_at(), Mes::helper::frame_layer(video,
					video->object->origin_frame, video->object->effect_layer + 1));
			AviUtl2::logging::warn(mes.c_str());
			return false;
		}

		D3D::cxt->CopyResource(framebuffer, cache_buff);
	}

	return true;
}

// Sound::cache_item
void save_sound_core(::FILTER_PROC_AUDIO* audio, Sound::cache_item& self)
{
	auto const params = get_object_audio_param(audio);
	size_t const N = audio->object->sample_num;
	self.buffer[0].resize(N);
	audio->get_audiobuffer_data(self.buffer[0].data(), 0);
	self.buffer[1].resize(N);
	audio->get_audiobuffer_data(self.buffer[1].data(), 1);
	self.params = params;
}

bool load_sound_core(::FILTER_PROC_AUDIO* audio, Sound::cache_item& self, bool restore_params)
{
	size_t const N = audio->object->sample_num;
	if (self.buffer[0].size() != N || self.buffer[1].size() != N) [[unlikely]] {
		AviUtl2::logging::error(L"Number of samples does not match!");
		return false;
	}

	audio->set_sample_data(self.buffer[0].data(), 0);
	audio->set_sample_data(self.buffer[1].data(), 1);
	if (restore_params) *audio->param = self.params;
	return true;
}

// Sound::scene_cache_list
void trim_scene_cache_sound(::FILTER_PROC_AUDIO* audio, Sound::scene_cache_list& self, bool is_ref, int64_t render_id)
{
	if (!is_ref || self.ref_render_id != render_id) {
		self.ref_render_id = render_id;
		self.ref.clear();
	}
	if (!is_ref && self.main_render_id != render_id) {
		self.main_pos = audio->object->origin_frame;
		self.main_render_id = render_id;
		self.main.clear();
	}
}
void save_scene_cache_sound(::FILTER_PROC_AUDIO* audio, Sound::scene_cache_list& self, bool is_ref, int64_t render_id,
	std::wstring const& name)
{
	auto& map = is_ref ? self.ref : self.main;
	save_sound_core(audio, map[name]);
}
bool load_scene_cache_sound(::FILTER_PROC_AUDIO* audio, Sound::scene_cache_list& self, bool is_ref, int64_t render_id,
	std::wstring const& name, bool restore_params)
{
	auto& map = is_ref ? self.ref : self.main;
	auto const it = map.find(name);
	if (it == map.end()) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {:?}, {}: {}.",
			Mes::list::error_cache_not_found(),
			Mes::helper::effect_name(audio), Mes::list::word_audio(),
			Mes::list::word_cache_name(), name,
			Mes::list::word_position_at(), Mes::helper::frame_layer(audio,
				audio->object->origin_frame, audio->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		return false;
	}
	return load_sound_core(audio, it->second, restore_params);
}

// Sound::scene_stack
void trim_stack_sound(::FILTER_PROC_AUDIO* audio, Sound::scene_stack& self, bool is_ref, int64_t render_id)
{
	if (!is_ref || self.ref_render_id != render_id) {
		self.ref_render_id = render_id;
		self.ref.clear();
	}
	if (!is_ref && self.main_render_id != render_id) {
		if (self.main.size() > 0) {
			auto const mes = std::format(L"{}\n> [{} / {}] {}: {}, {}: {}.",
				Mes::list::error_left_unclosed(),
				Mes::helper::effect_name(audio), Mes::list::word_audio(),
				Mes::list::word_position_at(), Mes::helper::frame_layers(audio, self.main_pos,
					{ std::from_range, self.main | std::views::transform([](auto x) {return x.layer; }) }),
				Mes::list::word_detected_at(), Mes::helper::frame_layer(audio,
					audio->object->origin_frame, audio->object->effect_layer + 1));
			AviUtl2::logging::warn(mes.c_str());
		}
		self.main_pos = audio->object->origin_frame;
		self.main_render_id = render_id;
		self.main.clear();
	}
}

void push_stack_sound(::FILTER_PROC_AUDIO* audio, Sound::scene_stack& self, bool is_ref, int64_t render_id)
{
	size_t const N = audio->object->sample_num;
	auto& tgt = is_ref ? self.ref : self.main;
	tgt.emplace_back(audio->object->effect_layer + 1);
	auto& item = tgt.back();

	item.buffer[0].resize(N);
	audio->get_audiobuffer_data(item.buffer[0].data(), 0);
	item.buffer[1].resize(N);
	audio->get_audiobuffer_data(item.buffer[1].data(), 1);

	std::vector<float> buff = {}; buff.resize(N);
	audio->set_audiobuffer_data(buff.data(), 0);
	audio->set_audiobuffer_data(buff.data(), 1);
}
bool pop_next_stack_sound(::FILTER_PROC_AUDIO* audio, Sound::scene_stack& self, bool is_ref, int64_t render_id,
	pop_mode pop_mode, std::wstring const& name)
{
	size_t const N = audio->object->sample_num;
	auto& tgt = is_ref ? self.ref : self.main;
	if (tgt.empty()) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {}.",
			Mes::list::error_none_opened(),
			Mes::helper::effect_name(audio), Mes::list::word_audio(),
			Mes::list::word_position_at(), Mes::helper::frame_layer(audio,
				audio->object->origin_frame, audio->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		return false;
	}
	auto const& item = tgt.back();
	if (item.buffer[0].size() != N || item.buffer[1].size() != N) [[unlikely]] {
		AviUtl2::logging::error(L"Number of samples does not match!");
		return false;
	}

	if (pop_mode == pop_mode::to_obj) {
		std::vector<float> buff{}; buff.resize(N);
		audio->get_audiobuffer_data(buff.data(), 0);
		audio->set_sample_data(buff.data(), 0);
		audio->set_audiobuffer_data(item.buffer[0].data(), 0);
		audio->get_audiobuffer_data(buff.data(), 1);
		audio->set_sample_data(buff.data(), 1);
		audio->set_audiobuffer_data(item.buffer[1].data(), 1);
	}
	else {
		auto& list = Sound::scene_cache[audio->scene->id];
		list.scene_id = audio->scene->id;
		trim_scene_cache_sound(audio, list, is_ref, render_id);
		auto& map = is_ref ? list.ref : list.main;

		auto& cache = map[name];
		cache.buffer[0].resize(N);
		audio->get_audiobuffer_data(cache.buffer[0].data(), 0);
		audio->set_audiobuffer_data(item.buffer[0].data(), 0);
		cache.buffer[1].resize(N);
		audio->get_audiobuffer_data(cache.buffer[1].data(), 1);
		audio->set_audiobuffer_data(item.buffer[1].data(), 1);
		cache.params = get_object_audio_param(audio, false);
	}

	if (pop_mode != pop_mode::next) tgt.pop_back();
	return true;
}


ANON_NS_E

////////////////////////////////
// Exported functions.
////////////////////////////////

// Image::scene_stack
void Image::scene_stack::trim(::FILTER_PROC_VIDEO* video)
{
	init();
	auto const is_ref = video->scene->is_reference_frame();
	auto const render_id = video->scene->rendering_id;
	trim_stack_image(video, *this, is_ref, render_id);
}

bool Image::scene_stack::push(::FILTER_PROC_VIDEO* video)
{
	init();
	auto const is_ref = video->scene->is_reference_frame();
	auto const render_id = video->scene->rendering_id;
	trim_stack_image(video, *this, is_ref, render_id);
	return push_stack_image(video, *this, is_ref, render_id);
}
bool Image::scene_stack::pop(::FILTER_PROC_VIDEO* video, crop_desc const& crop)
{
	init();
	auto const is_ref = video->scene->is_reference_frame();
	auto const render_id = video->scene->rendering_id;
	trim_stack_image(video, *this, is_ref, render_id);
	return pop_next_stack_image(video, *this, is_ref, render_id, pop_mode::to_obj, L""sv, {}, crop);
}
bool Image::scene_stack::pop(::FILTER_PROC_VIDEO* video, std::wstring_view const& name, cache_type type, crop_desc const& crop)
{
	init();
	auto const is_ref = video->scene->is_reference_frame();
	auto const render_id = video->scene->rendering_id;
	trim_stack_image(video, *this, is_ref, render_id);
	return pop_next_stack_image(video, *this, is_ref, render_id, pop_mode::others, name, type, crop);
}
bool Image::scene_stack::next(::FILTER_PROC_VIDEO* video, std::wstring_view const& name, cache_type type, crop_desc const& crop)
{
	init();
	auto const is_ref = video->scene->is_reference_frame();
	auto const render_id = video->scene->rendering_id;
	trim_stack_image(video, *this, is_ref, render_id);
	return pop_next_stack_image(video, *this, is_ref, render_id, pop_mode::next, name, type, crop);
}
std::wstring Image::scene_stack::get_cache_name(int scene_id, bool is_ref, size_t index)
{
	return std::format(L"cache:InlineScene_S/stk#{:x}/{}#{:x}",
		scene_id, is_ref ? L"ref"sv : L"main"sv, index);
}

// scene_cache_list
void Image::scene_cache_list::trim(::FILTER_PROC_VIDEO* video)
{
	init();
	auto const is_ref = video->scene->is_reference_frame();
	auto const render_id = video->scene->rendering_id;
	trim_scene_cache_image(video, *this, is_ref, render_id);
}
bool Image::scene_cache_list::save(::FILTER_PROC_VIDEO* video, std::wstring_view const& name, std::wstring const& source)
{
	init();
	auto const is_ref = video->scene->is_reference_frame();
	auto const render_id = video->scene->rendering_id;
	trim_scene_cache_image(video, *this, is_ref, render_id);
	return save_scene_cache_image(video, *this, is_ref, render_id,
		std::format(L"cache:{}", name), source);
}
bool Image::scene_cache_list::load(::FILTER_PROC_VIDEO* video, std::wstring_view const& name,
	std::wstring const& dest, bool restore_params, bool composite_rot)
{
	init();
	auto const is_ref = video->scene->is_reference_frame();
	auto const render_id = video->scene->rendering_id;
	trim_scene_cache_image(video, *this, is_ref, render_id);
	return load_scene_cache_image(video, *this, is_ref, render_id,
		std::format(L"cache:{}", name), dest, restore_params, composite_rot);
}

// Image::managed_cache_item
void Image::managed_cache_list::trim(clock::time_point now)
{
	init();

	// drop old cache.
	auto const limit = now - duration;
	int64_t total_dots = 0;
	for (auto it = list.begin(); it != list.end();) {
		if (it->second.last_used < limit) list.erase(it++);
		else {
			auto const& cache = it->second;
			if (!cache.is_deleted())
				total_dots += cache.width * cache.height;
			it++;
		}
	}

	// drop if exceeded size cap.
	if (auto rest = total_dots - max_total_dots; rest > 0 && list.size() > 1) {
		auto keys = std::vector<std::pair<std::wstring, clock::time_point>>{
			std::from_range, list
			| std::views::filter([](auto const& c) { return !c.second.is_deleted(); })
			| std::views::transform([](auto const& c) { return std::make_pair(c.first, c.second.last_used); })
		};
		if (keys.size() > 1) {
			std::sort(keys.begin(), keys.end(), [](auto const& l, auto const& r) {
				return l.second < r.second;
			});
			keys.pop_back(); // avoid dropping the latest one.
			for (auto const& [name, t] : keys) {
				auto const it = list.find(name);
				if (it != list.end()) {
					auto& cache = it->second;
					rest -= cache.width * cache.height;
					cache.buffer = nullptr;
					cache.data = nullptr;
				}
				if (rest <= 0) break;
			}
		}
	}
}
bool Image::managed_cache_list::save(::FILTER_PROC_VIDEO* video, std::wstring const& name, std::wstring const& source)
{
	init();
	auto const now = clock::now();
	managed_cache_item* cache = nullptr;
	if (save_type == managed_cache_type::gpu_mem) {
		if (auto const buffer = video->get_image_resource_texture2d(source.c_str());
			buffer != nullptr) {
			auto const desc = D3D::get_desc(buffer);
			int const width = static_cast<int>(desc.Width), height = static_cast<int>(desc.Height);
			cache = &list[name];
			if (cache->buffer != nullptr && cache->width == width && cache->height == height)
				D3D::cxt->CopyResource(cache->buffer.Get(), buffer);
			else *cache = {
				.buffer = D3D::clone(buffer),
				.width = width,
				.height = height,
			};
			cache->data = nullptr;
		}
	}
	else {
		if (int width, height; video->get_image_resource_size(source.c_str(), &width, &height)) {
			cache = &list[name];
			if (cache->data != nullptr && cache->width == width && cache->height == height) /* emtpy */;
			else *cache = {
				.data = std::make_unique<uint64_t[]>(((width + 1) & (~1)) * height),
				.width = width,
				.height = height,
			};
			cache->buffer = nullptr;
			if (!video->get_image_resource_data(source.c_str(),
				cache->data.get(), width, height,
				sizeof(uint64_t) * ((width + 1) & (~1)), ::OUTPUT_PIXEL_FORMAT::HF64)) {
				cache = nullptr;
				list.erase(name);
			}
		}
	}
	if (cache != nullptr) {
		cache->data = nullptr;
		cache->params = get_object_image_param(video, source == L"object"sv);
		cache->last_used = now;
	}

	switch (AviUtl2::edit_handle->get_edit_state()) {
	case EDIT_HANDLE::EDIT_STATE_PLAY:
	case EDIT_HANDLE::EDIT_STATE_SAVE:
		break;
	default:
	{
		trim(now);
		break;
	}
	}
	return cache != nullptr;
}
bool Image::managed_cache_list::load(::FILTER_PROC_VIDEO* video, std::wstring const& name,
	std::wstring const& dest, bool restore_params, bool composite_rot)
{
	init();
	bool success = true;
	auto const now = clock::now();
	auto const it = list.find(name);
	if (it == list.end()) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {:?}, {}: {}.",
			Mes::list::error_cache_not_found(),
			Mes::helper::effect_name(video), Mes::list::word_video(),
			Mes::list::word_cache_name(), name,
			Mes::list::word_position_at(), Mes::helper::frame_layer(video,
				video->object->origin_frame, video->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		success = false;
	}
	else if (auto& cache = it->second; cache.is_deleted()) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {:?}, {}: {}.",
			Mes::list::error_managed_cache_deleted(),
			Mes::helper::effect_name(video), Mes::list::word_video(),
			Mes::list::word_cache_name(), name,
			Mes::list::word_position_at(), Mes::helper::frame_layer(video,
				video->object->origin_frame, video->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		success = false;
	}
	else {
		cache.last_used = now;

		if (cache.buffer != nullptr) {
			video->create_image_resource(dest.c_str(), nullptr, cache.width, cache.height);
			auto const buffer = video->get_image_resource_texture2d(dest.c_str());
			success &= buffer != nullptr;
			if (success) D3D::cxt->CopyResource(buffer, cache.buffer.Get());
		}
		else {
			success &= video->set_image_resource_data(dest.c_str(),
				cache.data.get(), cache.width, cache.height,
				sizeof(uint64_t) * ((cache.width + 1) & (~1)),
				::INPUT_PIXEL_FORMAT::HF64);
		}

		if (success && dest == L"object"sv) {
			*video->param = cache.params;
			if (::OBJECT_IMAGE_PARAM p_out;
				composite_rot &&
				video->get_output_image_param(video->object->handle, 0, &p_out, sizeof(p_out))) {
				auto [rx, ry, rz] = composite_euler_angles(
					{ p_out.rx, p_out.ry, p_out.rz },
					{ cache.params.rx, cache.params.ry, cache.params.rz });
				video->param->rx = static_cast<float>(rx - p_out.rx);
				video->param->ry = static_cast<float>(ry - p_out.ry);
				video->param->rz = static_cast<float>(rz - p_out.rz);
			}
		}
	}

	switch (AviUtl2::edit_handle->get_edit_state()) {
	case EDIT_HANDLE::EDIT_STATE_PLAY:
	case EDIT_HANDLE::EDIT_STATE_SAVE:
		break;
	default:
	{
		trim(now);
		break;
	}
	}
	return success;
}

// Sound::scene_stack
void Sound::scene_stack::trim(::FILTER_PROC_AUDIO* audio)
{
	init();
	auto const is_ref = audio->scene->is_reference_frame();
	auto const render_id = audio->scene->rendering_id;
	trim_stack_sound(audio, *this, is_ref, render_id);
}
void Sound::scene_stack::push(::FILTER_PROC_AUDIO* audio)
{
	init();
	auto const is_ref = audio->scene->is_reference_frame();
	auto const render_id = audio->scene->rendering_id;
	trim_stack_sound(audio, *this, is_ref, render_id);
	push_stack_sound(audio, *this, is_ref, render_id);
}
bool Sound::scene_stack::pop(::FILTER_PROC_AUDIO* audio)
{
	init();
	auto const is_ref = audio->scene->is_reference_frame();
	auto const render_id = audio->scene->rendering_id;
	trim_stack_sound(audio, *this, is_ref, render_id);
	return pop_next_stack_sound(audio, *this, is_ref, render_id, pop_mode::to_obj, {});
}
bool Sound::scene_stack::pop(::FILTER_PROC_AUDIO* audio, std::wstring const& name)
{
	init();
	auto const is_ref = audio->scene->is_reference_frame();
	auto const render_id = audio->scene->rendering_id;
	trim_stack_sound(audio, *this, is_ref, render_id);
	return pop_next_stack_sound(audio, *this, is_ref, render_id, pop_mode::others, name);
}
bool Sound::scene_stack::next(::FILTER_PROC_AUDIO* audio, std::wstring const& name)
{
	init();
	auto const is_ref = audio->scene->is_reference_frame();
	auto const render_id = audio->scene->rendering_id;
	trim_stack_sound(audio, *this, is_ref, render_id);
	return pop_next_stack_sound(audio, *this, is_ref, render_id, pop_mode::next, name);
}

// Sound::scene_cache_list
void Sound::scene_cache_list::trim(::FILTER_PROC_AUDIO* audio)
{
	init();
	auto const is_ref = audio->scene->is_reference_frame();
	auto const render_id = audio->scene->rendering_id;
	trim_scene_cache_sound(audio, *this, is_ref, render_id);
}
void Sound::scene_cache_list::save(::FILTER_PROC_AUDIO* audio, std::wstring const& name)
{
	init();
	auto const is_ref = audio->scene->is_reference_frame();
	auto const render_id = audio->scene->rendering_id;
	trim_scene_cache_sound(audio, *this, is_ref, render_id);
	save_scene_cache_sound(audio, *this, is_ref, render_id, name);
}
bool Sound::scene_cache_list::load(::FILTER_PROC_AUDIO* audio, std::wstring const& name, bool restore_params)
{
	init();
	auto const is_ref = audio->scene->is_reference_frame();
	auto const render_id = audio->scene->rendering_id;
	trim_scene_cache_sound(audio, *this, is_ref, render_id);
	return load_scene_cache_sound(audio, *this, is_ref, render_id, name, restore_params);
}
