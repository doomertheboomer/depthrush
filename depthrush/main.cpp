#include "includes.h"

static uintptr_t imageBase;


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