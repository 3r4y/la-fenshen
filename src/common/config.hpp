#pragma once

#include <windows.h>
#include <string>

namespace mc::config
{
    constexpr auto ini_path = "C:\\Windows\\@TWFZ@.ini";
    constexpr auto ini_section = "sz";

    inline auto read_int( const char* key, int fallback = 0 ) -> int
    {
        char buf[ 32 ]{ };
        GetPrivateProfileStringA( ini_section, key,
            std::to_string( fallback ).c_str( ), buf, sizeof( buf ), ini_path );
        try { return std::stoi( buf ); }
        catch ( ... ) { return fallback; }
    }
}
