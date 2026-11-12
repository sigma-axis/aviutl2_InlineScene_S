/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11_3.h>

#include <plugin2.h>
#include <config2.h>

#include "logging.hpp"
#include "finalizing.hpp"

#include "InlineScene_Cache.hpp"
#include "Filter_Push.hpp"
#include "Filter_PopNext.hpp"
#include "Filter_Load.hpp"
#include "Filter_Save.hpp"
#include "InlineScene_S.hpp"

#include "Settings.hpp"
#include "Setting_Dlg.hpp"

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B;
////////////////////////////////
// helper functions.
////////////////////////////////
HMODULE dll_hinst = nullptr;
std::wstring setting_file_path(std::wstring_view const& ext) // `ext` contains dot.
{
	std::wstring ret{};
	ret.resize_and_overwrite(2 * MAX_PATH, [](wchar_t* buffer, size_t size) -> size_t {
		return ::GetModuleFileNameW(dll_hinst, buffer, static_cast<DWORD>(size) + 1);
	});
	auto pos = ret.find_last_of(L'.');
	if (pos == std::wstring::npos) pos = ret.size();
	ret.replace(pos, std::wstring::npos, ext);
	return ret;
}

void apply_settings(InlineScene_S::Settings::Setting const& settings)
{
	InlineScene_S::Cache::Image::managed_cache.duration = std::chrono::minutes(settings.cache_expire_minutes.get());
	InlineScene_S::Cache::Image::managed_cache.max_total_dots = settings.cache_total_dots.get();
}

////////////////////////////////
// plugin info.
////////////////////////////////
constinit COMMON_PLUGIN_TABLE plugin_table = {
	.name = PLUGIN_NAME,
	.information = PLUGIN_INFO_FMT(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR),
};

ANON_NS_E

////////////////////////////////
// exported functions.
////////////////////////////////

// DLL main.
extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID reserved)
{
	switch (reason) {
	case DLL_PROCESS_ATTACH:
	{
		dll_hinst = h;
		break;
	}
	default: break;
	}
	return TRUE;
}

wchar_t const* AviUtl2::translate(wchar_t const* str, wchar_t const* section)
{
	if (section == nullptr) return AviUtl2::config->translate(AviUtl2::config, str);
	return AviUtl2::config->get_language_text(AviUtl2::config, section, str);
}

// least version (since AviUtl2 beta33).
extern "C" __declspec(dllexport) DWORD RequiredVersion()
{
	return AviUtl2::least_aviutl2_ver_num;
}

// least version (in case of AviUtl2 before beta33).
extern "C" __declspec(dllexport) bool InitializePlugin(DWORD version)
{
	if (version >= AviUtl2::least_aviutl2_ver_num) return true;

	AviUtl2::logging::error(L"Requires AviUtl ExEdit2 version " LEAST_AVIUTL2_VER_STR L" or later!");
	::MessageBoxW(nullptr,
		PLUGIN_NAME L" は AviUtl ExEdit2 version " LEAST_AVIUTL2_VER_STR L" 以降のバージョンが必要です！ "
		L"AviUtl2 の最新版を確認してください．\n"
		PLUGIN_NAME L" requires AviUtl ExEdit2 version " LEAST_AVIUTL2_VER_STR L" or later! "
		L"Make sure your AviUtl2 is the latest.",
		PLUGIN_NAME, MB_OK | MB_ICONERROR);
	return false;
}

// information.
extern "C" __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable()
{
	return &plugin_table;
}

// config.
extern "C" __declspec(dllexport) void InitializeConfig(::CONFIG_HANDLE* config)
{
	AviUtl2::config = config;
}

// register.
extern "C" __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host)
{
	using namespace InlineScene_S;
	AviUtl2::edit_handle = host->create_edit_handle();

	// フィルタを登録．
	for (auto* table : {
		&Filters::push_obj,
		&Filters::push_anm,
		&Filters::pop_obj,
		&Filters::next_obj,
		&Filters::next_anm,
		&Filters::load_obj,
		&Filters::save_anm,
		}) host->register_filter_plugin(table);

	// キャッシュ破棄のコールバック登録．
	host->register_clear_cache_handler([](auto) static {
		AviUtl2::finalizing::Free();
	});

	// プロジェクトロードのコールバック登録．
	host->register_project_load_handler([](auto) static {
		// clear the scene-dependent cache.
		Cache::Image::scene_cache.clear();
		Cache::Image::scene_stacks.clear();
		Cache::Sound::scene_cache.clear();
		Cache::Sound::scene_stacks.clear();
	});

	// 設定メニュー登録.
	host->register_config_menu(PLUGIN_NAME, [](HWND hwnd, HINSTANCE dll_hinst) static {
		auto [confirmed, updated] = Settings::GUI::show_setting_dialog(hwnd, dll_hinst, Settings::settings);
		if (confirmed) {
			Settings::settings = updated.clamp();
			apply_settings(Settings::settings);
		}
	});

	// other initializations.
	constexpr auto ext = L".json";
	d3d_service::D3D::init(AviUtl2::edit_handle->get_direct3d_device());
	Settings::settings = Settings::load(setting_file_path(ext)).clamp();
	apply_settings(Settings::settings);
	AviUtl2::finalizing::RegisterFinalize([] static {
		Settings::save(setting_file_path(ext), Settings::settings);
	});
}
