#include <Windows.h>
#include <psapi.h>

#pragma comment (lib, "psapi.lib")

MODULEINFO GetModuleData(const char* moduleName);
bool DataCompare(const BYTE* pData, const BYTE* pattern, const char* mask);
DWORD FindPattern(DWORD start_address, DWORD length, BYTE* pattern, char* mask);

typedef void(__stdcall* /*__cdecl*/ _Log)(char* format, ...); // uCore.log

int DetectGame();
_Log GetLog(int game);

DWORD WINAPI DllThread(HMODULE hModule)
{
	_Log Log = GetLog(DetectGame());
	if (Log != NULL)
	{
		Beep(1000, 200);

		while (true)
		{
			if (GetAsyncKeyState(VK_OEM_PLUS)) {
				Beep(1000, 200);
				Log("[TSNest] test!");
			}

			Sleep(100);
		}
	}

	FreeLibraryAndExitThread(hModule, 0);
}

int DetectGame()
{
	start:
	HWND hWnd = FindWindow("_uengine_", "Metro 2033");
	if (hWnd != NULL)
	{
		return 1; // 2033
	}
	else
	{
		hWnd = FindWindow("_uengine_", "Metro LL");
		if (hWnd != NULL)
		{
			return 2; // LL
		}
		else
		{
			goto start;
		}
	}
}

_Log GetLog(int game)
{
	_Log Log = NULL;
	MODULEINFO mi = GetModuleData(NULL);

	if (game == 1)
	{
		// 81 EC ? ? ? ? 8B 8C 24 ? ? ? ? 53 56
		Log = (_Log) FindPattern(
			(DWORD)mi.lpBaseOfDll,
			mi.SizeOfImage,
			(BYTE*) "\x81\xEC\x00\x00\x00\x00\x8B\x8C\x24\x00\x00\x00\x00\x53\x56",
			"xx????xxx????xx");
	}
	else if (game == 2)
	{
		// 55 8B EC 83 E4 F8 81 EC ? ? ? ? 8B 4D 08 56
		Log = (_Log) FindPattern(
			(DWORD)mi.lpBaseOfDll,
			mi.SizeOfImage,
			(BYTE*) "\x55\x8B\xEC\x83\xE4\xF8\x81\xEC\x00\x00\x00\x00\x8B\x4D\x08\x56",
			"xxxxxxxx????xxxx");
	}

	return Log;
}

MODULEINFO GetModuleData(const char* moduleName)
{
	MODULEINFO currentModuleInfo = { 0 };
	HMODULE moduleHandle = GetModuleHandle(moduleName);
	if (moduleHandle == NULL)
	{
		return currentModuleInfo;
	}
	GetModuleInformation(GetCurrentProcess(), moduleHandle, &currentModuleInfo, sizeof(MODULEINFO));
	return currentModuleInfo;
}

bool DataCompare(const BYTE* pData, const BYTE* pattern, const char* mask)
{
	for (; *mask; mask++, pData++, pattern++)
		if (*mask == 'x' && *pData != *pattern)
			return false;
	return (*mask) == NULL;
}

DWORD FindPattern(DWORD start_address, DWORD length, BYTE* pattern, char* mask)
{
	for (DWORD i = 0; i < length; i++)
		if (DataCompare((BYTE*)(start_address + i), pattern, mask))
			return (DWORD)(start_address + i);
	return NULL;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH: {
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)DllThread, hModule, NULL, NULL);
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

