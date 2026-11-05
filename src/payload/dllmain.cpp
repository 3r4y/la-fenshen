#include "hook.hpp"
#include <windows.h>
#include <string>

namespace payload::mutant { auto install( ) -> void; }
namespace payload::share { auto install( ) -> void; }
namespace payload::stealth { auto install( ) -> void; auto install_process_guard( ) -> void; }
namespace payload::hwid { auto install( ) -> void; }
namespace payload::profile { auto set_instance( std::uint32_t ) -> void; }
namespace payload::render { auto install( HWND ) -> bool; }

namespace
{
    constexpr const char* targets[] = {
        "LOSTARK.exe",
        "auncher.exe",
        "XACProject.exe",
    };

    constexpr const char* window_classes[] = {
        "UnrealWindow",
        "class neox::toolkit::Win32WindowNt",
    };

    auto get_exe_name( ) -> std::string
    {
        char buf[ MAX_PATH ]{ };
        GetModuleFileNameA( nullptr, buf, MAX_PATH );
        return buf;
    }

    auto matches_any( const std::string& path ) -> bool
    {
        for ( auto& t : targets )
        {
            if ( path.find( t ) != std::string::npos )
                return true;
        }
        return false;
    }

    auto has_sibling_window( ) -> bool
    {
        for ( auto& cls : window_classes )
        {
            if ( FindWindowExA( nullptr, nullptr, cls, nullptr ) )
                return true;
        }
        return false;
    }

    auto parse_instance_id( ) -> std::uint32_t
    {
        auto cmdline = std::string{ GetCommandLineA( ) };
        auto pos = cmdline.find( "-@=" );
        if ( pos == std::string::npos )
            return 0;

        auto start = pos + 3;
        std::uint32_t id = 0;
        for ( auto i = start; i < cmdline.size( ) && cmdline[ i ] >= '0' && cmdline[ i ] <= '9'; ++i )
            id = id * 10 + ( cmdline[ i ] - '0' );

        return id;
    }
}

auto APIENTRY DllMain( HMODULE instance, DWORD reason, LPVOID ) -> BOOL
{
    if ( reason != DLL_PROCESS_ATTACH )
        return TRUE;

    DisableThreadLibraryCalls( instance );

    auto exe = get_exe_name( );
    if ( !matches_any( exe ) )
        return TRUE;

    if ( !has_sibling_window( ) )
        return TRUE;

    mc::hooks_init( );

    payload::profile::set_instance( parse_instance_id( ) );
    payload::hwid::install( );
    payload::stealth::install( );

    if ( exe.find( "LOSTARK.exe" ) != std::string::npos )
    {
        payload::mutant::install( );
        payload::share::install( );

        auto fg = GetForegroundWindow( );
        if ( fg )
            payload::render::install( fg );
    }

    if ( exe.find( "auncher.exe" ) != std::string::npos
      || exe.find( "XACProject.exe" ) != std::string::npos )
        payload::stealth::install_process_guard( );

    return TRUE;
}
