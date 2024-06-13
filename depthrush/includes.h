#pragma once
#include <Windows.h>
#include <Windowsx.h>
#include <libloaderapi.h>
#include <winnt.h>
#include <iostream>
#include <NuiApi.h>
#include <stdint.h>
#include <algorithm>

#include "kiero/kiero.h"
#include "kiero/minhook/include/MinHook.h"
#include "kiero/injector/injector.hpp"
#include "kiero/injector/calling.hpp"
#include "d3d9.h"
#include "ini.h"
#include "drs.h"

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;

#define HOOK(addr, detour) MH_CreateHook((LPVOID)(addr), \
	Hook_##detour, \
	reinterpret_cast<LPVOID*>(&Orig_##detour))

inline void safeJMP(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true)
{
	MH_Initialize();
	MH_CreateHook((void*)at.as_int(), (void*)dest.as_int(), nullptr);
	MH_EnableHook((void*)at.as_int());
}

inline void safeUNJMP(injector::memory_pointer_tr pTarget) {
	MH_DisableHook((void*)pTarget.as_int());
	MH_RemoveHook((void*)pTarget.as_int());
}