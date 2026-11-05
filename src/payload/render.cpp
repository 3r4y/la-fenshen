#include "hook.hpp"
#include "config.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include <cstdlib>

using present_fn = HRESULT( STDMETHODCALLTYPE* )( IDXGISwapChain*, UINT, UINT );
using draw_indexed_fn = void( STDMETHODCALLTYPE* )( ID3D11DeviceContext*, UINT, UINT, INT );
using draw_indexed_instanced_fn = void( STDMETHODCALLTYPE* )( ID3D11DeviceContext*, UINT, UINT, UINT, INT, UINT );
using draw_indexed_instanced_indirect_fn = void( STDMETHODCALLTYPE* )( ID3D11DeviceContext*, ID3D11Buffer*, UINT );
using ps_set_samplers_fn = void( STDMETHODCALLTYPE* )( ID3D11DeviceContext*, UINT, UINT, ID3D11SamplerState* const* );
using ds_set_constant_buffers_fn = void( STDMETHODCALLTYPE* )( ID3D11DeviceContext*, UINT, UINT, ID3D11Buffer* const* );
using vs_set_constant_buffers_fn = void( STDMETHODCALLTYPE* )( ID3D11DeviceContext*, UINT, UINT, ID3D11Buffer* const* );
using clear_depth_stencil_fn = void( STDMETHODCALLTYPE* )( ID3D11DeviceContext*, ID3D11DepthStencilView*, UINT, FLOAT, UINT8 );

static mc::hook_t< present_fn > o_present;
static mc::hook_t< draw_indexed_fn > o_draw_indexed;
static mc::hook_t< draw_indexed_instanced_fn > o_draw_indexed_instanced;
static mc::hook_t< draw_indexed_instanced_indirect_fn > o_draw_indexed_instanced_indirect;
static mc::hook_t< ps_set_samplers_fn > o_ps_set_samplers;
static mc::hook_t< ds_set_constant_buffers_fn > o_ds_set_constant_buffers;
static mc::hook_t< vs_set_constant_buffers_fn > o_vs_set_constant_buffers;
static mc::hook_t< clear_depth_stencil_fn > o_clear_depth_stencil;

static char g_cpu[ 260 ] = "0";
static char g_hpyh[ 260 ] = "0";

static auto hpyh_val( ) -> int { return std::atoi( g_hpyh ); }
static auto cpu_val( ) -> int { return std::atoi( g_cpu ); }

static auto STDMETHODCALLTYPE hk_present( IDXGISwapChain* sc, UINT sync, UINT flags ) -> HRESULT
{
    auto v = cpu_val( );
    if ( v > 1 )
        Sleep( v );

    return o_present.call( sc, sync, flags );
}

static auto STDMETHODCALLTYPE hk_draw_indexed( ID3D11DeviceContext* ctx, UINT count, UINT start, INT base ) -> void
{
    if ( hpyh_val( ) == 1 ) return;
    o_draw_indexed.call( ctx, count, start, base );
}

static auto STDMETHODCALLTYPE hk_draw_indexed_instanced( ID3D11DeviceContext* ctx,
    UINT idx_count, UINT inst_count, UINT start_idx, INT base_vtx, UINT start_inst ) -> void
{
    auto v = hpyh_val( );
    if ( v == 1 || v == 3 ) return;
    o_draw_indexed_instanced.call( ctx, idx_count, inst_count, start_idx, base_vtx, start_inst );
}

static auto STDMETHODCALLTYPE hk_draw_indexed_instanced_indirect( ID3D11DeviceContext* ctx,
    ID3D11Buffer* buf, UINT offset ) -> void
{
    auto v = hpyh_val( );
    if ( v == 1 || v == 3 ) return;
    o_draw_indexed_instanced_indirect.call( ctx, buf, offset );
}

static auto STDMETHODCALLTYPE hk_ps_set_samplers( ID3D11DeviceContext* ctx,
    UINT start, UINT count, ID3D11SamplerState* const* samplers ) -> void
{
    auto v = hpyh_val( );
    if ( v == 1 || v == 3 ) return;
    o_ps_set_samplers.call( ctx, start, count, samplers );
}

static auto STDMETHODCALLTYPE hk_ds_set_constant_buffers( ID3D11DeviceContext* ctx,
    UINT start, UINT count, ID3D11Buffer* const* buffers ) -> void
{
    auto v = hpyh_val( );
    if ( v == 1 || v == 3 ) return;
    o_ds_set_constant_buffers.call( ctx, start, count, buffers );
}

static auto STDMETHODCALLTYPE hk_vs_set_constant_buffers( ID3D11DeviceContext* ctx,
    UINT start, UINT count, ID3D11Buffer* const* buffers ) -> void
{
    auto v = hpyh_val( );
    if ( v == 1 ) return;
    if ( v == 2 )
    {
        GetPrivateProfileStringA( "sz", "hpyh", "0", g_hpyh, sizeof( g_hpyh ),
            mc::config::ini_path );
        return;
    }
    o_vs_set_constant_buffers.call( ctx, start, count, buffers );
}

static auto STDMETHODCALLTYPE hk_clear_depth_stencil( ID3D11DeviceContext* ctx,
    ID3D11DepthStencilView* view, UINT flags, FLOAT depth, UINT8 stencil ) -> void
{
    if ( hpyh_val( ) == 1 ) return;
    o_clear_depth_stencil.call( ctx, view, flags, depth, stencil );
}

static auto config_poll( LPVOID ) -> DWORD
{
    for ( ;; )
    {
        GetPrivateProfileStringA( "sz", "CPU", "0", g_cpu, sizeof( g_cpu ), mc::config::ini_path );
        GetPrivateProfileStringA( "sz", "hpyh", "0", g_hpyh, sizeof( g_hpyh ), mc::config::ini_path );
        Sleep( 5000 );
    }
}

namespace payload::render
{
    auto install( HWND window ) -> bool
    {
        DXGI_SWAP_CHAIN_DESC sd{ };
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.Width = 4;
        sd.BufferDesc.Height = 4;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = window;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* dev = nullptr;
        IDXGISwapChain* sc = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        D3D_FEATURE_LEVEL level;

        if ( FAILED( D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &level, &ctx ) ) )
            return false;

        auto** sc_vt = *reinterpret_cast< void*** >( sc );
        auto** ctx_vt = *reinterpret_cast< void*** >( ctx );

        o_present.install( sc_vt[ 8 ], hk_present );

        auto gpt = mc::config::read_int( "gpt" );

        if ( gpt != 1 )
        {
            o_draw_indexed.install( ctx_vt[ 0x0C ], hk_draw_indexed );
            o_draw_indexed_instanced.install( ctx_vt[ 0x14 ], hk_draw_indexed_instanced );
            o_draw_indexed_instanced_indirect.install( ctx_vt[ 0x27 ], hk_draw_indexed_instanced_indirect );
            o_ps_set_samplers.install( ctx_vt[ 0x0A ], hk_ps_set_samplers );
            o_ds_set_constant_buffers.install( ctx_vt[ 0x42 ], hk_ds_set_constant_buffers );
            o_vs_set_constant_buffers.install( ctx_vt[ 0x07 ], hk_vs_set_constant_buffers );
            o_clear_depth_stencil.install( ctx_vt[ 0x35 ], hk_clear_depth_stencil );
        }

        ctx->Release( );
        sc->Release( );
        dev->Release( );
        return true;
    }
}
