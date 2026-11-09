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
#include <config2.h>

#include "log_messages.hpp"
using namespace InlineScene_S::Messages;

#include "InlineScene_S.hpp"
#define HEADER_NAME (L"Plugin." PLUGIN_NAME L".Message")
using AviUtl2::translate;

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B
////////////////////////////////
// core functionalities.
////////////////////////////////
std::wstring format_time(double time)
{
	[[assume(time >= 0)]];
	int mn = static_cast<int>(std::floor(time / 60)); time -= 60 * mn;
	int hr = mn / 60; mn -= 60 * hr;
	if (hr > 0) return std::format(L"{}:{:02d}:{:5.2f}", hr, mn, time);
	else if (mn > 0) return std::format(L"{}:{:5.2f}", mn, time);
	else return std::format(L"{:.2f} s", time);
}
std::wstring format_time(int frame, int rate, int scale)
{
	return format_time(static_cast<int64_t>(frame) * scale / static_cast<double>(rate));
}

ANON_NS_E

////////////////////////////////
// exports.
////////////////////////////////
wchar_t const* list::error_cache_not_found()
{
	return translate(L"指定された名前のキャッシュが見つかりません！", HEADER_NAME);
}
wchar_t const* list::error_cache_deleted()
{
	return translate(
		L"指定された名前のキャッシュは削除されています．キャッシュサイズが上限に達した可能性があります．\n"
		LR"(設定ファイル "system.conf" 内の "TemporaryImageCacheSize" を増やしてみてください．)", HEADER_NAME);
}
wchar_t const* list::error_managed_cache_deleted()
{
	return translate(
		L"指定された名前のキャッシュは削除されています．キャッシュサイズが上限に達しました．\n"
		LR"(設定 → プラグイン設定 → InlineScene_S から "キャッシュの総ドット数" を増やしてみてください．)", HEADER_NAME);
}
wchar_t const* list::error_stack_modified()
{
	return translate(L"復元用のフレームバッファのキャッシュに予期せぬ変更が加えられていました！", HEADER_NAME);
}

wchar_t const* list::error_none_opened()
{
	return translate(L"Inline Scene が開始していない状態で終了しようとしました！", HEADER_NAME);
}
wchar_t const* list::error_left_unclosed()
{
	return translate(L"Inline Scene が正しく閉じられませんでした！", HEADER_NAME);
}
wchar_t const* list::error_multi_obj()
{
	return translate(L"Inline Scene の開始終了は，個別オブジェクトには適用しないでください！", HEADER_NAME);
}
wchar_t const* list::error_camera_mode()
{
	return translate(L"Inline Scene の開始終了は，カメラ制御下には配置しないでください！", HEADER_NAME);
}
wchar_t const* list::error_clipping_obj()
{
	return translate(L"Inline Scene の開始終了は，"
		LR"("クリッピングオブジェクト" や "上のオブジェクトでクリッピング" を適用したり，)"
		L"その影響下に配置しないでください！", HEADER_NAME);
}
wchar_t const* list::word_cache_name()
{
	return translate(L"キャッシュ名", HEADER_NAME);
}
wchar_t const* list::word_position_at()
{
	return translate(L"場所", HEADER_NAME);
}
wchar_t const* list::word_detected_at()
{
	return translate(L"検出箇所", HEADER_NAME);
}
wchar_t const* list::word_video()
{
	return translate(L"画像", HEADER_NAME);
}
wchar_t const* list::word_audio()
{
	return translate(L"音声", HEADER_NAME);
}

wchar_t const* helper::effect_name(::OBJECT_INFO const* object, ::EDIT_SECTION const* edit)
{
	auto const raw_name = edit->get_effect_name(object->effect_handle);
	return translate(raw_name, raw_name);
}

std::wstring helper::frame_layer(int frame, int rate, int scale, wchar_t const* scene_name, int layer)
{
	return std::format(L"{} F ({}) / Layer {} ({})",
		frame + 1, format_time(frame, rate, scale), layer, scene_name);
}

std::wstring helper::frame_layers(int frame, int rate, int scale, wchar_t const* scene_name, std::vector<int> const& layers)
{
	[[assume(layers.size() > 0)]];
	if (layers.size() == 1) return frame_layer(frame, rate, scale, scene_name, layers.back());
	else return std::format(L"{} F ({}) / Layer {} ({})",
		frame + 1, format_time(frame, rate, scale), layers, scene_name);
}
