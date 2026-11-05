#include "ntdefs.hpp"
#include <string>

static std::uint32_t g_instance_id = 0;

static auto needs_redirect( const std::wstring& path ) -> bool
{
    if ( g_instance_id == 0 )
        return false;

    return path.find( L"58f3ac" ) != path.npos
        || path.find( L"firebase.json" ) != path.npos
        || path.find( L"LocalPrefs.json" ) != path.npos;
}

namespace payload::profile
{
    auto set_instance( std::uint32_t id ) -> void
    {
        g_instance_id = id;
    }

    auto redirect( POBJECT_ATTRIBUTES oa, UNICODE_STRING& patched, std::wstring& buf ) -> PUNICODE_STRING
    {
        if ( !oa || !oa->ObjectName || !oa->ObjectName->Buffer )
            return nullptr;

        auto path = std::wstring{ oa->ObjectName->Buffer, oa->ObjectName->Length / sizeof( wchar_t ) };
        if ( !needs_redirect( path ) )
            return nullptr;

        buf = path + L"." + std::to_wstring( g_instance_id );

        patched.Length = static_cast< USHORT >( buf.size( ) * sizeof( wchar_t ) );
        patched.MaximumLength = patched.Length + sizeof( wchar_t );
        patched.Buffer = buf.data( );

        auto* orig = oa->ObjectName;
        oa->ObjectName = &patched;
        return orig;
    }
}
