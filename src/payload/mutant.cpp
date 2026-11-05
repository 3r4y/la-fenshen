#include "ntdefs.hpp"
#include "hook.hpp"
#include <string>

static mc::hook_t< nt::create_mutant_fn > o_create;
static mc::hook_t< nt::open_mutant_fn > o_open;

static auto is_guid( PUNICODE_STRING us ) -> bool
{
    if ( !us || !us->Buffer || !us->Length )
        return false;

    bool has_brace = false, has_dash = false;
    auto len = us->Length / sizeof( wchar_t );

    for ( USHORT i = 0; i < len; ++i )
    {
        if ( us->Buffer[ i ] == L'{' || us->Buffer[ i ] == L'}' ) has_brace = true;
        if ( us->Buffer[ i ] == L'-' ) has_dash = true;
    }

    return has_brace && has_dash;
}

static auto rename_mutant( POBJECT_ATTRIBUTES oa, UNICODE_STRING& patched, std::wstring& buf ) -> PUNICODE_STRING
{
    if ( !oa || !oa->ObjectName || !is_guid( oa->ObjectName ) )
        return nullptr;

    auto* orig = oa->ObjectName;
    buf.assign( orig->Buffer, orig->Length / sizeof( wchar_t ) );
    buf += std::to_wstring( GetCurrentProcessId( ) );

    patched.Length = static_cast< USHORT >( buf.size( ) * sizeof( wchar_t ) );
    patched.MaximumLength = patched.Length + sizeof( wchar_t );
    patched.Buffer = buf.data( );

    oa->ObjectName = &patched;
    return orig;
}

static auto NTAPI hk_create( PHANDLE h, ACCESS_MASK a, POBJECT_ATTRIBUTES oa, BOOLEAN owner ) -> NTSTATUS
{
    UNICODE_STRING us{ }; std::wstring buf;
    auto* saved = rename_mutant( oa, us, buf );
    auto status = o_create.call( h, a, oa, owner );
    if ( saved ) oa->ObjectName = saved;
    return status;
}

static auto NTAPI hk_open( PHANDLE h, ACCESS_MASK a, POBJECT_ATTRIBUTES oa ) -> NTSTATUS
{
    UNICODE_STRING us{ }; std::wstring buf;
    auto* saved = rename_mutant( oa, us, buf );
    auto status = o_open.call( h, a, oa );
    if ( saved ) oa->ObjectName = saved;
    return status;
}

namespace payload::mutant
{
    auto install( ) -> void
    {
        o_create.install( nt::resolve( "NtCreateMutant" ), hk_create );
        o_open.install( nt::resolve( "NtOpenMutant" ), hk_open );
    }
}
