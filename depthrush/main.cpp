#include "includes.h"

static uintptr_t imageBase;

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

static int returnTrue() {
	return 1;
}

static int returnFalse() {
	return 0;
}




BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hMod);
	imageBase = (uintptr_t)GetModuleHandleA(0);

	hookDancepad(); // drs.cpp
	return true;
}