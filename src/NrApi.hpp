#pragma once
#include <Windows.h>
#include <cstdint>

// Private local ABI between FrameLab and its own adapter DLL (not an NVIDIA DLL).
// Shared handles stay owned by the caller until NrDestroy returns.
struct NrConfig {
    uint32_t size = sizeof(NrConfig);
    LUID adapter{};
    unsigned width{}, height{}, inferenceWidth{}, inferenceHeight{};
    HANDLE input{}, output{}, inputFence{};
    const wchar_t* runtimeDirectory{};
    const wchar_t* cacheDirectory{};
    unsigned passes = 1;
};
struct NrFrame {
    uint64_t inputFenceValue{};
    bool reset = false;
    bool useMotion = true;
};
using NrCreateFn = void* (__cdecl*)(const NrConfig*, char*, unsigned);
using NrProcessFn = bool (__cdecl*)(void*, const NrFrame*, float*, char*, unsigned);
using NrDestroyFn = void (__cdecl*)(void*);
