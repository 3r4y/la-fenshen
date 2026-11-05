#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <format>
#include <filesystem>

namespace launcher::cleanup
{
    static auto get_game_path( ) -> std::string
    {
        char buf[ 260 ]{ };
        GetPrivateProfileStringA( "sz", "lj", "", buf, sizeof( buf ),
            "C:\\Windows\\@TWFZ@.ini" );
        return buf;
    }

    static auto get_localappdata_low( ) -> std::filesystem::path
    {
        wchar_t* p = nullptr;
        if ( SUCCEEDED( SHGetKnownFolderPath( FOLDERID_LocalAppDataLow, 0, nullptr, &p ) ) )
        {
            auto result = std::filesystem::path{ p };
            CoTaskMemFree( p );
            return result;
        }

        char buf[ MAX_PATH ]{ };
        GetEnvironmentVariableA( "LOCALAPPDATA", buf, MAX_PATH );
        return std::filesystem::path{ buf } / ".." / "LocalLow";
    }

    auto delete_profile( std::uint32_t index ) -> void
    {
        auto game_path = get_game_path( );
        if ( game_path.empty( ) )
            return;

        auto game_dir = std::filesystem::path{ game_path }
            .parent_path( ).parent_path( ).parent_path( );

        auto token_file = game_dir / "Saved" /
            std::format( "58f3ace3c6c4f2ab29b4bfff9e97e09551ec4888dba663543d60b5ed04da2780.txt.{}", index );

        std::error_code ec;
        std::filesystem::remove( token_file, ec );

        auto low = get_localappdata_low( );

        auto firebase = low / "My Play" / "LUNA _ MOBILE" / "FirebaseWebAuth" /
            std::format( "firebase.json.{}", index );
        std::filesystem::remove( firebase, ec );

        auto prefs = low / "My Play" / "LUNA _ MOBILE" / "Vuplex.WebView" /
            "chromium-cache" / std::format( "LocalPrefs.json.{}", index );
        std::filesystem::remove( prefs, ec );
    }

    auto delete_all_profiles( std::uint32_t max_index ) -> void
    {
        for ( std::uint32_t i = 1; i <= max_index; ++i )
            delete_profile( i );
    }

    auto clear_google_cache( ) -> void
    {
        auto game_path = get_game_path( );
        if ( game_path.empty( ) )
            return;

        auto game_dir = std::filesystem::path{ game_path }
            .parent_path( ).parent_path( ).parent_path( );

        auto cache_dir = game_dir / "Saved" / "webcache_4430";

        std::error_code ec;
        std::filesystem::remove_all( cache_dir, ec );
    }
}
