#include <windows.h>
#include <filesystem>
#include <cstdint>

namespace launcher::inject
{
    auto via_loadlibrary( std::uint32_t pid, const std::filesystem::path& dll ) -> bool
    {
        auto full = std::filesystem::absolute( dll ).wstring( );

        auto process = OpenProcess(
            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
            FALSE, pid );

        if ( !process )
            return false;

        auto size = ( full.size( ) + 1 ) * sizeof( wchar_t );
        auto remote = VirtualAllocEx( process, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

        if ( !remote )
        {
            CloseHandle( process );
            return false;
        }

        WriteProcessMemory( process, remote, full.c_str( ), size, nullptr );

        auto kernel32 = GetModuleHandleA( "kernel32.dll" );
        auto load = reinterpret_cast< LPTHREAD_START_ROUTINE >(
            GetProcAddress( kernel32, "LoadLibraryW" ) );

        auto thread = CreateRemoteThread( process, nullptr, 0, load, remote, 0, nullptr );

        if ( !thread )
        {
            VirtualFreeEx( process, remote, 0, MEM_RELEASE );
            CloseHandle( process );
            return false;
        }

        WaitForSingleObject( thread, 5000 );
        VirtualFreeEx( process, remote, 0, MEM_RELEASE );
        CloseHandle( thread );
        CloseHandle( process );
        return true;
    }
}
