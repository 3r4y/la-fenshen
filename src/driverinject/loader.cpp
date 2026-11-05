#include "loader.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace driverinject
{
    auto generate_service_name( ) -> std::string
    {
        constexpr auto alphabet = "QWERTYUIPPASDDFGFDGXZCBVCBZXasdqwdasdczxcvvbdfgrtuuyklui";
        constexpr auto alpha_len = 55;

        std::srand( GetTickCount( ) );

        auto name = std::string{ };

        for ( int i = 0; i < 3; ++i )
            name += alphabet[ std::rand( ) % alpha_len ];

        name += std::to_string( std::rand( ) % 50 + 1 );

        for ( int i = 0; i < 3; ++i )
            name += alphabet[ std::rand( ) % alpha_len ];

        return name;
    }

    auto drop_driver( const std::filesystem::path& sys_path, const void* data, std::size_t size ) -> bool
    {
        auto out = std::ofstream{ sys_path, std::ios::binary };
        if ( !out )
            return false;

        out.write( static_cast< const char* >( data ), size );
        return out.good( );
    }

    auto load_driver( const std::string& service_name, const std::filesystem::path& sys_path ) -> bool
    {
        auto scm = OpenSCManagerA( nullptr, nullptr, SC_MANAGER_CREATE_SERVICE );
        if ( !scm )
            return false;

        auto path_str = sys_path.string( );

        auto svc = CreateServiceA( scm, service_name.c_str( ), service_name.c_str( ),
            SERVICE_START, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
            path_str.c_str( ), nullptr, nullptr, nullptr, nullptr, nullptr );

        if ( !svc )
        {
            svc = OpenServiceA( scm, service_name.c_str( ), SERVICE_START );
            if ( !svc )
            {
                CloseServiceHandle( scm );
                return false;
            }
        }

        auto ok = StartServiceA( svc, 0, nullptr );
        if ( !ok && GetLastError( ) != ERROR_SERVICE_ALREADY_RUNNING )
        {
            CloseServiceHandle( svc );
            CloseServiceHandle( scm );
            return false;
        }

        CloseServiceHandle( svc );
        CloseServiceHandle( scm );
        return true;
    }

    auto unload_driver( const std::string& service_name ) -> bool
    {
        auto scm = OpenSCManagerA( nullptr, nullptr, SC_MANAGER_CONNECT );
        if ( !scm )
            return false;

        auto svc = OpenServiceA( scm, service_name.c_str( ),
            SERVICE_STOP | DELETE );

        if ( !svc )
        {
            CloseServiceHandle( scm );
            return false;
        }

        SERVICE_STATUS status{ };
        ControlService( svc, SERVICE_CONTROL_STOP, &status );
        auto ok = DeleteService( svc );

        CloseServiceHandle( svc );
        CloseServiceHandle( scm );
        return ok;
    }

    auto open_device( ) -> HANDLE
    {
        return CreateFileA( device_name, GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr );
    }

    auto send_dll( HANDLE device, const void* dll_data, std::uint32_t dll_size, bool x64 ) -> bool
    {
        if ( device == INVALID_HANDLE_VALUE || !dll_data || dll_size < 2 )
            return false;

        auto mz = static_cast< const std::uint8_t* >( dll_data );
        if ( mz[ 0 ] != 'M' || mz[ 1 ] != 'Z' )
            return false;

        auto ioctl = x64 ? ioctl_x64_dll : ioctl_x86_dll;

        DWORD returned = 0;
        std::uint8_t out_buf[ 4 ]{ };

        return DeviceIoControl( device, ioctl,
            const_cast< void* >( dll_data ), dll_size,
            out_buf, sizeof( out_buf ), &returned, nullptr );
    }

    auto inject( const void* driver_sys, std::size_t driver_size,
                 const void* payload_dll, std::uint32_t payload_size, bool x64 ) -> bool
    {
        auto name = generate_service_name( );
        auto sys_path = std::filesystem::path{ "C:\\Windows" } / ( name + ".sys" );

        if ( !drop_driver( sys_path, driver_sys, driver_size ) )
            return false;

        if ( !load_driver( name, sys_path ) )
        {
            std::filesystem::remove( sys_path );
            return false;
        }

        auto device = open_device( );
        if ( device == INVALID_HANDLE_VALUE )
        {
            unload_driver( name );
            std::filesystem::remove( sys_path );
            return false;
        }

        auto ok = send_dll( device, payload_dll, payload_size, x64 );

        CloseHandle( device );
        return ok;
    }
}
