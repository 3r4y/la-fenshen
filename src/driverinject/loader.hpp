#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <filesystem>

namespace driverinject
{
    constexpr auto device_name = "\\\\.\\CrashDumpUpload";
    constexpr auto ioctl_x86_dll = 0x222001u;
    constexpr auto ioctl_x64_dll = 0x222005u;

    auto generate_service_name( ) -> std::string;
    auto drop_driver( const std::filesystem::path& sys_path, const void* data, std::size_t size ) -> bool;
    auto load_driver( const std::string& service_name, const std::filesystem::path& sys_path ) -> bool;
    auto unload_driver( const std::string& service_name ) -> bool;
    auto open_device( ) -> HANDLE;
    auto send_dll( HANDLE device, const void* dll_data, std::uint32_t dll_size, bool x64 ) -> bool;
    auto inject( const void* driver_sys, std::size_t driver_size,
                 const void* payload_dll, std::uint32_t payload_size, bool x64 ) -> bool;
}
