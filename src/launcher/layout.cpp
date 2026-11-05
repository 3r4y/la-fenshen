#include <windows.h>
#include <string>
#include <vector>
#include <format>
#include <cstdint>

namespace launcher::layout
{
    auto find_game_windows( std::uint32_t count ) -> std::vector< HWND >
    {
        auto windows = std::vector< HWND >{ };

        for ( std::uint32_t i = 1; i <= count; ++i )
        {
            auto title = std::format( "LUNA MOBILE({})", i );
            auto hwnd = FindWindowExA( nullptr, nullptr, nullptr, title.c_str( ) );

            if ( hwnd )
                windows.push_back( hwnd );
        }

        return windows;
    }

    auto arrange_grid( std::uint32_t cell_w, std::uint32_t cell_h, std::uint32_t count ) -> void
    {
        auto screen_w = static_cast< std::uint32_t >( GetSystemMetrics( SM_CXSCREEN ) );
        auto windows = find_game_windows( count );

        std::uint32_t x = 0, y = 0;

        for ( auto hwnd : windows )
        {
            if ( x + cell_w > screen_w )
            {
                x = 0;
                y += cell_h - 4;
            }

            MoveWindow( hwnd, x + 2, y + 2, cell_w, cell_h, TRUE );
            x += cell_w - 10;
        }
    }
}
