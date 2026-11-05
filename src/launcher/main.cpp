#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <format>
#include <filesystem>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>

#include "loader.hpp"
namespace launcher::inject
{
    auto via_loadlibrary( std::uint32_t pid, const std::filesystem::path& dll ) -> bool;
}

namespace launcher::layout
{
    auto arrange_grid( std::uint32_t w, std::uint32_t h, std::uint32_t count ) -> void;
}

namespace launcher::cleanup
{
    auto delete_profile( std::uint32_t index ) -> void;
    auto delete_all_profiles( std::uint32_t max_index ) -> void;
    auto clear_google_cache( ) -> void;
}

namespace
{
    auto count_processes( const char* name ) -> std::uint32_t
    {
        auto snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
        if ( snap == INVALID_HANDLE_VALUE )
            return 0;

        PROCESSENTRY32 pe{ };
        pe.dwSize = sizeof( pe );

        std::uint32_t n = 0;

        if ( Process32First( snap, &pe ) )
        {
            do
            {
                if ( _stricmp( pe.szExeFile, name ) == 0 )
                    ++n;
            }
            while ( Process32Next( snap, &pe ) );
        }

        CloseHandle( snap );
        return n;
    }

    auto launch_instance( const std::string& exe, std::uint32_t index,
        std::uint32_t width, std::uint32_t height ) -> std::uint32_t
    {
        auto args = std::format( "\"{}\" -screen-fullscreen 0 -screen-width {} -screen-height {} -@={}",
            exe, width, height, index );

        STARTUPINFOA si{ };
        si.cb = sizeof( si );
        PROCESS_INFORMATION pi{ };

        if ( !CreateProcessA( exe.c_str( ), args.data( ), nullptr, nullptr,
            FALSE, 0, nullptr, nullptr, &si, &pi ) )
            return 0;

        auto pid = pi.dwProcessId;
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
        return pid;
    }

    struct find_ctx { DWORD pid; HWND result; };

    auto CALLBACK enum_proc( HWND hwnd, LPARAM lp ) -> BOOL
    {
        auto* ctx = reinterpret_cast< find_ctx* >( lp );
        DWORD wnd_pid = 0;
        GetWindowThreadProcessId( hwnd, &wnd_pid );

        if ( wnd_pid != ctx->pid )
            return TRUE;

        char cls[ 128 ]{ };
        GetClassNameA( hwnd, cls, sizeof( cls ) );

        if ( std::string_view{ cls }.find( "neox" ) != std::string_view::npos
          || std::string_view{ cls }.find( "UnrealWindow" ) != std::string_view::npos )
        {
            ctx->result = hwnd;
            return FALSE;
        }

        return TRUE;
    }

    auto rename_window( std::uint32_t pid, std::uint32_t index ) -> void
    {
        auto target = std::format( "LUNA MOBILE({})", index );

        for ( int attempt = 0; attempt < 60; ++attempt )
        {
            auto ctx = find_ctx{ pid, nullptr };
            EnumWindows( enum_proc, reinterpret_cast< LPARAM >( &ctx ) );

            if ( !ctx.result )
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
                continue;
            }

            SetWindowTextA( ctx.result, target.c_str( ) );
            return;
        }
    }

    auto read_file( const std::filesystem::path& path ) -> std::vector< std::uint8_t >
    {
        auto file = std::ifstream{ path, std::ios::binary | std::ios::ate };
        if ( !file )
            return { };

        auto size = file.tellg( );
        file.seekg( 0 );

        auto buf = std::vector< std::uint8_t >( size );
        file.read( reinterpret_cast< char* >( buf.data( ) ), size );
        return buf;
    }

    auto inject_usermode( std::uint32_t pid, const std::filesystem::path& dll ) -> bool
    {
        return launcher::inject::via_loadlibrary( pid, dll );
    }

    auto inject_via_driver( const std::filesystem::path& dll,
        const std::filesystem::path& sys ) -> bool
    {
        auto dll_data = read_file( dll );
        auto sys_data = read_file( sys );

        if ( dll_data.empty( ) || sys_data.empty( ) )
        {
            std::printf( "  failed to read dll or sys file\n" );
            return false;
        }

        return driverinject::inject(
            sys_data.data( ), sys_data.size( ),
            dll_data.data( ), static_cast< std::uint32_t >( dll_data.size( ) ),
            true );
    }
}

auto main( int argc, char** argv ) -> int
{
    if ( argc >= 2 && std::string_view{ argv[ 1 ] } == "--clean" )
    {
        auto max = argc > 2 ? std::atoi( argv[ 2 ] ) : 50;
        std::printf( "deleting profiles 1-%d...\n", max );
        launcher::cleanup::delete_all_profiles( max );
        std::printf( "done.\n" );
        return 0;
    }

    if ( argc >= 2 && std::string_view{ argv[ 1 ] } == "--clear-cache" )
    {
        std::printf( "clearing webcache...\n" );
        launcher::cleanup::clear_google_cache( );
        std::printf( "done.\n" );
        return 0;
    }

    if ( argc < 3 )
    {
        std::printf( "usage: launcher <game> <count> [width] [height] [dll] [mode] [sys]\n" );
        std::printf( "       launcher --clean [max_index]\n" );
        std::printf( "       launcher --clear-cache\n" );
        std::printf( "\n" );
        std::printf( "  mode: usermode  — CreateRemoteThread + LoadLibraryW (default)\n" );
        std::printf( "        driver    — DriverInjectDll kernel driver (needs signed .sys)\n" );
        return 1;
    }

    auto exe = std::string{ argv[ 1 ] };
    auto count = static_cast< std::uint32_t >( std::atoi( argv[ 2 ] ) );
    auto width = argc > 3 ? static_cast< std::uint32_t >( std::atoi( argv[ 3 ] ) ) : 800u;
    auto height = argc > 4 ? static_cast< std::uint32_t >( std::atoi( argv[ 4 ] ) ) : 600u;
    auto dll = argc > 5 ? std::filesystem::path{ argv[ 5 ] } : std::filesystem::path{ "64.dll" };
    auto driver_mode = argc > 6 && std::string_view{ argv[ 6 ] } == "driver";
    auto sys = argc > 7 ? std::filesystem::path{ argv[ 7 ] } : std::filesystem::path{ };

    if ( driver_mode )
    {
        if ( sys.empty( ) )
        {
            std::printf( "driver mode requires .sys path as last argument\n" );
            return 1;
        }

        std::printf( "loading driver %s...\n", sys.string( ).c_str( ) );

        if ( !inject_via_driver( dll, sys ) )
        {
            std::printf( "driver injection failed\n" );
            return 1;
        }

        std::printf( "driver loaded, payload sent\n" );
    }

    for ( std::uint32_t i = 1; i <= count; ++i )
    {
        std::printf( "[%u/%u] launching...\n", i, count );

        auto pid = launch_instance( exe, i, width, height );
        if ( !pid )
        {
            std::printf( "  failed to create process\n" );
            continue;
        }

        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        if ( !driver_mode )
        {
            if ( !inject_usermode( pid, dll ) )
                std::printf( "  inject failed (pid %u)\n", pid );
            else
                std::printf( "  injected (pid %u)\n", pid );
        }

        std::thread( rename_window, pid, i ).detach( );
    }

    std::printf( "\narranging %u windows (%ux%u)\n", count, width, height );
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    launcher::layout::arrange_grid( width, height, count );

    std::printf( "done. %u instances running.\n", count_processes( "LOSTARK.exe" ) );
    return 0;
}
