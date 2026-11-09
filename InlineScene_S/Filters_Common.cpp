/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <string>
#include <set>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filter2.h>
#include <plugin2.h>

#include "Filters_Common.hpp"
namespace Common = InlineScene_S::Filters::Common;

#include "logging.hpp"
#include "log_messages.hpp"
namespace Mes = InlineScene_S::Messages;

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B;
////////////////////////////////
// state holders.
////////////////////////////////

// state holder that prevents spamming warnings about multi-objects.
struct {
	int64_t render_id;
	int obj_layer;
	std::set<EFFECT_HANDLE> effect_hanldes;
	void reset(int64_t render_id, int obj_layer)
	{
		if (this->render_id != render_id ||
			this->obj_layer != obj_layer) {
			this->render_id = render_id;
			this->obj_layer = obj_layer;
			effect_hanldes.clear();
		}
	}
	bool push(EFFECT_HANDLE effect_handle)
	{
		// returns true if the insertion took place.
		return effect_hanldes.insert(effect_handle).second;
	}
} mult_obj_states{};

ANON_NS_E

////////////////////////////////
// exports.
////////////////////////////////
bool Common::verify_open_close_image(::FILTER_PROC_VIDEO* video)
{
	// cannot apply to multi-objects.
	if (video->object->num != 1) {
		mult_obj_states.reset(video->scene->rendering_id, video->object->layer);
		if (mult_obj_states.push(video->object->effect_handle)) {
			auto const mes = std::format(L"{}\n> [{} / {}] {}: {}.",
				Mes::list::error_multi_obj(),
				Mes::helper::effect_name(video), Mes::list::word_video(),
				Mes::list::word_position_at(), Mes::helper::frame_layer(video,
					video->object->origin_frame, video->object->effect_layer + 1));
			AviUtl2::logging::warn(mes.c_str());
		}
		return false;
	}

	// cannot apply to objects under camera control.
	if (video->get_camera_mode() != CAMERA_MODE::OFF) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {}.",
			Mes::list::error_camera_mode(),
			Mes::helper::effect_name(video), Mes::list::word_video(),
			Mes::list::word_position_at(), Mes::helper::frame_layer(video,
				video->object->origin_frame, video->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		return false;
	}

	// cannot apply to clipping objects and under those effects.
	if (video->edit->get_object_flag(video->object->handle, OBJECT_FLAG_TYPE::CLIPPING_OBJECT) ||
		video->edit->get_object_flag(video->object->handle, OBJECT_FLAG_TYPE::CLIPPING_UPPER_OBJECT)) {
		auto const mes = std::format(L"{}\n> [{} / {}] {}: {}.",
			Mes::list::error_clipping_obj(),
			Mes::helper::effect_name(video), Mes::list::word_video(),
			Mes::list::word_position_at(), Mes::helper::frame_layer(video,
				video->object->origin_frame, video->object->effect_layer + 1));
		AviUtl2::logging::warn(mes.c_str());
		return false;
	}

	return true;
}
