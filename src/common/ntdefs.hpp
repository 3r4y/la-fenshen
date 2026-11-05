#pragma once

#include <cstdint>
#include <windows.h>
#include <winternl.h>

namespace nt
{
    constexpr auto status_unsuccessful = static_cast< NTSTATUS >( 0xC0000001 );

    struct client_id_t
    {
        HANDLE unique_process;
        HANDLE unique_thread;
    };

    struct system_process_info_t
    {
        ULONG next_entry_offset;
        ULONG number_of_threads;
        std::uint8_t reserved1[ 48 ];
        PVOID unique_process_id;
    };

    constexpr ULONG system_process_information = 5;

    using create_mutant_fn = NTSTATUS( NTAPI* )(
        PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, BOOLEAN );

    using open_mutant_fn = NTSTATUS( NTAPI* )(
        PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );

    using create_file_fn = NTSTATUS( NTAPI* )(
        PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
        PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG );

    using write_file_fn = NTSTATUS( NTAPI* )(
        HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK,
        PVOID, ULONG, PLARGE_INTEGER, PULONG );

    using read_virtual_memory_fn = NTSTATUS( NTAPI* )(
        HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T );

    using query_system_information_fn = NTSTATUS( NTAPI* )(
        ULONG, PVOID, ULONG, PULONG );

    using open_process_fn = NTSTATUS( NTAPI* )(
        PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, client_id_t* );

    inline auto resolve( const char* name ) -> void*
    {
        static auto ntdll = GetModuleHandleA( "ntdll.dll" );
        return reinterpret_cast< void* >( GetProcAddress( ntdll, name ) );
    }
}
