#include "ntdefs.hpp"
#include "hook.hpp"
#include <string>

namespace payload::profile
{
    auto redirect( POBJECT_ATTRIBUTES oa, UNICODE_STRING& patched, std::wstring& buf ) -> PUNICODE_STRING;
}

static mc::hook_t< nt::create_file_fn > o_create_file;
static mc::hook_t< nt::write_file_fn > o_write_file;
static mc::hook_t< nt::read_virtual_memory_fn > o_read_vm;
static int g_xh_state = 0;

static auto path_of( POBJECT_ATTRIBUTES oa ) -> std::wstring
{
    if ( !oa || !oa->ObjectName || !oa->ObjectName->Buffer )
        return { };
    return { oa->ObjectName->Buffer, oa->ObjectName->Length / sizeof( wchar_t ) };
}

static auto has( const std::wstring& s, const wchar_t* sub ) -> bool
{
    return s.find( sub ) != s.npos;
}

static auto NTAPI hk_write_file( HANDLE a, HANDLE b, PVOID c, PVOID d, PIO_STATUS_BLOCK e,
    PVOID f, ULONG g, PLARGE_INTEGER h, PULONG i ) -> NTSTATUS
{
    return 0;
}

static auto NTAPI hk_read_vm( HANDLE p, PVOID base, PVOID buf, SIZE_T n, PSIZE_T out ) -> NTSTATUS
{
    return o_read_vm.call( p, base, buf, 0, nullptr );
}

static auto NTAPI hk_create_file( PHANDLE fh, ACCESS_MASK access, POBJECT_ATTRIBUTES oa,
    PIO_STATUS_BLOCK iosb, PLARGE_INTEGER alloc, ULONG attrs, ULONG share,
    ULONG disp, ULONG opts, PVOID ea, ULONG ea_len ) -> NTSTATUS
{
    UNICODE_STRING patched{ }; std::wstring buf;
    auto* saved = payload::profile::redirect( oa, patched, buf );

    auto p = path_of( oa );
    NTSTATUS status;

    if ( has( p, L"xhunter1" ) || has( p, L"1.sys" ) || has( p, L"XHUNTER1" ) )
    {
        o_read_vm.install( nt::resolve( "NtReadVirtualMemory" ), hk_read_vm );

        if ( has( p, L"xhunter1" ) && !has( p, L"." ) && g_xh_state == 1 )
        {
            o_write_file.remove( );
            g_xh_state = 2;
        }

        if ( ( has( p, L"indows\\" ) || has( p, L"INDOWS\\" ) ) && has( p, L"1.sys" ) )
        {
            if ( g_xh_state == 0 )
            {
                o_write_file.install( nt::resolve( "NtWriteFile" ), hk_write_file );
                g_xh_state = 1;
            }
            status = static_cast< NTSTATUS >( 0xC0000034 );
        }
        else
        {
            status = static_cast< NTSTATUS >( 0xC0000034 );
        }
    }
    else
    {
        status = o_create_file.call( fh, access, oa, iosb, alloc, attrs,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            disp, opts, ea, ea_len );
    }

    if ( saved ) oa->ObjectName = saved;
    return status;
}

namespace payload::share
{
    auto install( ) -> void
    {
        o_create_file.install( nt::resolve( "NtCreateFile" ), hk_create_file );
    }
}
