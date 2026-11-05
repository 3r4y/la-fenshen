#include <winsock2.h>
#include "hook.hpp"
#include <iphlpapi.h>
#include <cstdlib>

namespace
{
    using get_adapters_info_fn = DWORD( WINAPI* )( PIP_ADAPTER_INFO, PULONG );
    using get_adapters_addresses_fn = ULONG( WINAPI* )( ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES_LH, PULONG );

    mc::hook_t< get_adapters_info_fn > g_info;
    mc::hook_t< get_adapters_addresses_fn > g_addrs;

    auto randomize_bytes( BYTE* data, DWORD length ) -> void
    {
        for ( DWORD i = 0; i < length; ++i )
            data[ i ] = static_cast< BYTE >( std::rand( ) % 255 + 1 );
    }

    auto WINAPI hooked_get_adapters_info( PIP_ADAPTER_INFO info, PULONG size ) -> DWORD
    {
        auto result = g_info.call( info, size );
        if ( result != NO_ERROR )
            return result;

        std::srand( GetTickCount( ) );
        for ( auto* p = info; p; p = p->Next )
            randomize_bytes( p->Address, p->AddressLength );

        return result;
    }

    auto WINAPI hooked_get_adapters_addresses( ULONG family, ULONG flags, PVOID reserved,
        PIP_ADAPTER_ADDRESSES_LH addrs, PULONG size ) -> ULONG
    {
        auto result = g_addrs.call( family, flags, reserved, addrs, size );
        if ( result != NO_ERROR || !addrs )
            return result;

        std::srand( GetTickCount( ) );
        for ( auto* p = addrs; p; p = p->Next )
            randomize_bytes( p->PhysicalAddress, p->PhysicalAddressLength );

        return result;
    }
}

namespace payload::hwid
{
    auto install( ) -> void
    {
        auto iphlpapi = GetModuleHandleA( "iphlpapi.dll" );
        if ( !iphlpapi )
            iphlpapi = LoadLibraryA( "iphlpapi.dll" );

        g_info.install(
            reinterpret_cast< void* >( GetProcAddress( iphlpapi, "GetAdaptersInfo" ) ),
            &hooked_get_adapters_info );

        g_addrs.install(
            reinterpret_cast< void* >( GetProcAddress( iphlpapi, "GetAdaptersAddresses" ) ),
            &hooked_get_adapters_addresses );
    }
}
