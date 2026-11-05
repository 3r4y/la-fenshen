# la-fenshen

Multi-instance launcher for Lost Ark. Hooks mutex creation so each copy gets its own namespace, forces file sharing, hides processes from each other, spoofs MACs. Can throttle D3D11 draws on background windows to save GPU.

分身 (fēnshēn, "split body") is what Chinese app-cloning and multi-instance tools call themselves: 微信分身, 应用分身, 多开分身. The tool category is usually called 多开器 (duōkāiqì, "multi-open tool"). I reverse engineered one of these closed-source 多开器 to see how it worked.

The binary was packed with VMProtect 2.x on top of an 易语言 runtime, so I had to devirtualize 2566 VM blocks before anything made sense. The embedded kernel driver was signed with HookSignTool, which backdates a certificate through a fake timestamp authority, a common way these tools got kernel drivers loaded without test signing. The original source was never public. I rebuilt the whole thing from the disassembly so the technique is documented: the nt hooks, the 驱动注入 (driver injection) via ZwContinue, the anti-detection setup. Research only, don't point this at live games.

## How it works

Game creates a GUID mutex on startup. Second copy sees `ERROR_ALREADY_EXISTS` and quits. `64.dll` hooks `NtCreateMutant` and `NtOpenMutant`, appends PID to the name if it looks like a GUID. Each copy gets its own mutex.

`NtCreateFile` hook forces shared access on every open so multiple clients can read the same files. `NtQuerySystemInformation` hook strips the process list down to just PID 0 and self. `NtOpenProcess` hook blocks opens to any PID other than self. `GetAdaptersInfo`/`GetAdaptersAddresses` hooks randomize MAC bytes after each call.

For GPU, the DLL hooks `Present` and 7 context methods on D3D11. Config is polled from an INI every 5 seconds:

- `hpyh=1` blocks all draws. black screen, GPU near zero, game still connected
- `hpyh=2` blocks VS constant buffers only. geometry breaks but window stays alive
- `hpyh=3` blocks instanced draws, keeps single draw and depth clear
- `CPU=N` sleeps N ms after each Present

## Driver mode

The original tool embedded a modified build of [strivexjun/DriverInjectDll](https://github.com/strivexjun/DriverInjectDll) (`driver_inject_x64.sys`, PDB path `C:\Users\nice\Desktop\DriverInjectDll-master\bin\driver_inject_x64.pdb`). The driver creates `\\.\CrashDumpUpload`, hooks `ZwContinue` in every new process, and reflectively loads the DLL payload through that hook.

The embedded binary uses modified IOCTL codes compared to the public source:

| | GitHub source | Embedded binary |
|---|---|---|
| x86 DLL | `CTL_CODE(0x22, 0x900, 1, 0)` = 0x222401 | `CTL_CODE(0x22, 0x800, 1, 0)` = 0x222001 |
| x64 DLL | `CTL_CODE(0x22, 0x901, 1, 0)` = 0x222405 | `CTL_CODE(0x22, 0x801, 1, 0)` = 0x222005 |

Our `loader.cpp` uses the embedded binary's values (0x222001/0x222005).

The IOCTL dispatch decompiles to:

```c
// 0x140001700 — IRP_MJ_DEVICE_CONTROL handler
ULONG ioctl = irpSp->Parameters.DeviceIoControl.IoControlCode;

if ( ioctl == 0x222001 )                            // x86
{
    if ( g_x86dll || g_x86dllsize )  goto done;
    if ( *(USHORT*)inputBuffer != 0x5A4D )  goto done;
    g_x86dll = ExAllocatePoolWithTag( 0, inputLen, 'x86d' );
    memcpy( g_x86dll, inputBuffer, inputLen );
}
else if ( ioctl == 0x222005 )                       // x64
{
    if ( g_x64dll || g_x64dllsize )  goto done;
    if ( *(USHORT*)inputBuffer != 0x5A4D )  goto done;
    g_x64dll = ExAllocatePoolWithTag( 0, inputLen, 'x64d' );
    memcpy( g_x64dll, inputBuffer, inputLen );
}
```

Injection flow per process (`INJECT_ROUTINE_X64`, decompiled):

```c
PsLookupProcessByProcessId( pid, &process );
KeStackAttachProcess( process, &apc );

pfnZwContinue = GetProcAddressR( ntdll_base, "ZwContinue", TRUE );
NtReadVirtualMemory( ..., pfnZwContinue, &payload.oldData, 14, ... );

alloc_size = sizeof(PAYLOAD) + sizeof(MemLoadShellcode_x64) + g_x64dllsize;
NtAllocateVirtualMemory( ..., &alloc, ..., MEM_COMMIT, PAGE_EXECUTE_READWRITE );

// shellcode: save regs → unhook ZwContinue → reflective load DLL → erase DLL → restore regs → jmp ZwContinue
NtWriteVirtualMemory( ..., alloc, &payload, sizeof(payload), ... );
NtWriteVirtualMemory( ..., dllPos, g_x64dll, g_x64dllsize, ... );
NtWriteVirtualMemory( ..., shellcodePos, MemLoadShellcode_x64, sizeof(MemLoadShellcode_x64), ... );

// hook: ff 25 00 00 00 00 [alloc_ptr]  (jmp qword ptr [rip])
NtProtectVirtualMemory( ..., &pfnZwContinue, 5, PAGE_EXECUTE_READWRITE, ... );
NtWriteVirtualMemory( ..., pfnZwContinue, hookbuf, 14, ... );

KeUnstackDetachProcess( &apc );
```

The driver was signed with a JemmyLoveJenny/HookSignTool fake timestamp over a VeriSign Class 3 certificate. Microsoft revoked these in July 2023. The driver loaded without test signing before that date.

## Lost Ark

Lost Ark uses EAC now. This was built from a tool that targeted an older anti-cheat. EAC will catch it.

## Building

```
cmake -B build -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
cmake --build build
```

Outputs `64.dll` and `launcher.exe`. MinHook and DriverInjectDll vendored in `third_party/`. `64.dll` needs `libwinpthread-1.dll` next to it (mingw posix thread model, ships with the toolchain).

## Usage

```
launcher.exe <game> <count> [width] [height] [dll] [mode] [sys]
launcher.exe --clean [max_index]
launcher.exe --clear-cache
```

Modes: `usermode` (CreateRemoteThread + LoadLibraryW, default), `driver` (DriverInjectDll kernel driver, needs signed .sys).

## Credits

- [DriverInjectDll](https://github.com/strivexjun/DriverInjectDll)
- [MinHook](https://github.com/TsudaKageyu/minhook)
- [Unicorn](https://github.com/unicorn-engine/unicorn)
- [Capstone](https://github.com/capstone-engine/capstone)
