#pragma once

#include <MinHook.h>
#include <cstdint>
#include <utility>

namespace mc
{
    inline auto hooks_init( ) -> bool
    {
        static bool once = [ ] { return MH_Initialize( ) == MH_OK; }( );
        return once;
    }

    template< typename fn_t >
    struct hook_t
    {
        fn_t original = nullptr;
        void* target = nullptr;

        template< typename detour_t >
        auto install( void* tgt, detour_t detour ) -> bool
        {
            target = tgt;

            if ( MH_CreateHook( target, reinterpret_cast< void* >( detour ),
                    reinterpret_cast< void** >( &original ) ) != MH_OK )
                return false;

            return MH_EnableHook( target ) == MH_OK;
        }

        auto remove( ) -> void
        {
            if ( !target )
                return;

            MH_DisableHook( target );
            MH_RemoveHook( target );
            target = nullptr;
            original = nullptr;
        }

        template< typename... args_t >
        auto call( args_t... args ) -> decltype( auto )
        {
            return original( std::forward< args_t >( args )... );
        }

        explicit operator bool( ) const { return original != nullptr; }

        ~hook_t( ) { remove( ); }

        hook_t( ) = default;
        hook_t( const hook_t& ) = delete;
        auto operator=( const hook_t& ) -> hook_t& = delete;
    };
}
