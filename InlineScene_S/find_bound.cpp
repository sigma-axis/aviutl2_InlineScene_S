/*
The MIT License (MIT)

Copyright (c) 2026 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <memory>

#include "logging.hpp"
namespace logging = AviUtl2::logging;
#include "finalizing.hpp"

#include "d3d_service.hpp"
using D3D = d3d_service::D3D;

#include "find_bound.hpp"

#define ANON_NS_B namespace {
#define ANON_NS_E }


ANON_NS_B
////////////////////////////////
// shaders.
////////////////////////////////
constexpr char cs_src_find[] = R"(
RWStructuredBuffer<uint> dst : register(u0);
Texture2D<half4> src : register(t0);
cbuffer constant0 : register(b0) { uint2 size; };
[numthreads(64, 1, 1)]
void csmain(uint2 id : SV_DispatchThreadID)
{
	if (id.x >= size.y) return;

	int l = int(size.x), r = -1;
	for (int x = 0; x < int(size.x); x++) {
		if (src[uint2(x, id.x)].a > 0) {
			l = min(l, x); r = max(r, x);
		}
	}
	dst[id.x] = r < 0 ? 0 :
		(l & 0xffff) | (((r + 1) & 0xffff) << 16);
}
)";
struct cs_cbuff_find {
	uint32_t size_x, size_y;

	uint8_t _pad[8];
};
static_assert(sizeof(cs_cbuff_find) % 16 == 0);

////////////////////////////////
// Resource managements.
////////////////////////////////
constinit AviUtl2::finalizing::helpers::init_state init_state{};
D3D::ComPtr<::ID3D11ComputeShader> cs_find;
void quit()
{
	cs_find.Reset();

	init_state.clear();
}
bool init()
{
	init_state.init(&quit, [] {
		return

		#define cs_src(name)	cs_src_##name, "bin::cs_" #name
		(cs_find = D3D::create_compute_shader(cs_src(find))) != nullptr &&
		#undef cs_src

			true;
	});
	return init_state;
}
ANON_NS_E

////////////////////////////////
// Exported functions.
////////////////////////////////
InlineScene_S::FindBound::bounds InlineScene_S::FindBound::Find(::ID3D11Texture2D* source, int width, int height)
{
	if (!init()) return {};

	// create target buffer.
	constexpr uint32_t size_elem = sizeof(uint32_t);
	uint32_t const size_buff = size_elem * ((height + 3) & (~3));
	D3D::ComPtr<::ID3D11Buffer> const
		lr_bounds = D3D::create_structured_buffer(size_elem, size_buff),
		lr_bounds_copied = D3D::create_structured_buffer(size_elem, size_buff, true);

	// create constant buffer.
	auto cbuff = D3D::create_const_buffer(cs_cbuff_find{
		.size_x = static_cast<uint32_t>(width),
		.size_y = static_cast<uint32_t>(height),
	});
	if (cbuff == nullptr) return {};

	// execute shader.
	D3D::cxt->CSSetShader(cs_find.Get(), nullptr, 0);
	D3D::cxt->CSSetShaderResources(0, 1, D3D::to_shader_resource_view(source).GetAddressOf());
	D3D::cxt->CSSetUnorderedAccessViews(0, 1, D3D::to_unordered_access_view(lr_bounds.Get()).GetAddressOf(), nullptr);
	D3D::cxt->CSSetConstantBuffers(0, 1, cbuff.GetAddressOf());
	D3D::cxt->Dispatch((height + ((1 << 6) - 1)) >> 6, 1, 1);

	// retrieve data from lr_bounds.
	D3D::cxt->CopyResource(lr_bounds_copied.Get(), lr_bounds.Get());
	bounds ret{ width + 1, -1, height + 1, -1 };
	if (::D3D11_MAPPED_SUBRESOURCE mapped{};
		S_OK != D3D::cxt->Map(lr_bounds_copied.Get(), 0, D3D11_MAP_READ, 0, &mapped)) {
		logging::error(L"Failed to read from structured buffer!");
		return {};
	}
	else {
		auto const* buff = static_cast<int16_t const*>(mapped.pData);
		for (int y = 0; y < height; y++) {
			int const l = buff[2 * y], r = buff[2 * y + 1];
			if (l < r) {
				ret.left = std::min(ret.left, l); ret.right = std::max(ret.right, r);
				ret.top = std::min(ret.top, y); ret.bottom = std::max(ret.bottom, y + 1);
			}
		}
		D3D::cxt->Unmap(lr_bounds_copied.Get(), 0);
	}

	// cleanup.
	D3D::cxt->ClearState();

	return ret;
}
