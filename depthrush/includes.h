#pragma once
#include <Windows.h>
#include <Windowsx.h>
#include <libloaderapi.h>
#include <winnt.h>
#include <iostream>
#include <NuiApi.h>
#include "kiero/kiero.h"
#include "kiero/minhook/include/MinHook.h"
#include "kiero/injector/injector.hpp"
#include "d3d9.h"
#include "drs.h"

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;