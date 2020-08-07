#include "Types.h"
#include "SprTexReplace.h"
#include "Addresses.h"
#include "FunctionHooker.h"
#include "UTF8.h"
#include "Path.h"
#include "Log.h"

namespace SprTexReplace
{
	void InstallAllHooks(const HMODULE moduleHandle)
	{
		auto functionHooker = FunctionHooker(moduleHandle);
		functionHooker.Hook(Addresses::SprSetParseTexSet, EvilGlobalState.OriginalParseSprSetTexSet, Hooks::SprSetParseTexSet);
		functionHooker.HookPointer(Addresses::GLGenTexturesExtern, EvilGlobalState.OriginalGLGenTextures, Hooks::GLGenTextures);
	}

	void OnDLLAttach(const HMODULE moduleHandle)
	{
		EvilGlobalState.ThisDLLDirectory = [moduleHandle]
		{
			WCHAR moduleFileName[MAX_PATH];
			::GetModuleFileNameW(moduleHandle, moduleFileName, MAX_PATH);

			return std::string(Path::GetDirectoryName(UTF8::Narrow(moduleFileName)));
		}();

		EvilGlobalState.IterateRegisterReplaceDirectoryFilePathsAsync();
		InstallAllHooks(moduleHandle);
	}

	void OnDLLDetach(const HMODULE moduleHandle)
	{
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		SprTexReplace::OnDLLAttach(hModule);
		break;

	case DLL_PROCESS_DETACH:
		SprTexReplace::OnDLLDetach(hModule);
		break;
	}

	return TRUE;
}

static const auto PluginName = UTF8::WideArg(
	"SprTexReplace"
);

static const auto PluginDescription = UTF8::WideArg(
	"SprTexReplace by samyuu\n"
	"\n"
	"Loads resolution independent sprite sheet replacement image files."
);

extern "C"
{
	__declspec(dllexport) LPCWSTR GetPluginName()
	{
		return PluginName.c_str();
	}

	__declspec(dllexport) LPCWSTR GetPluginDescription()
	{
		return PluginDescription.c_str();
	}
}
