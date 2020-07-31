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

		const auto glGenTexturesHook = &Hooks::GLGenTextures;
		WriteToProtectedMemory(Addresses::GLGenTexturesExtern, sizeof(glGenTexturesHook), &glGenTexturesHook);
	}

	void OnDLLAttach(const HMODULE moduleHandle)
	{
		EvilGlobalState.WorkingDirectory = []
		{
			WCHAR moduleFileName[MAX_PATH];
			::GetModuleFileNameW(NULL, moduleFileName, MAX_PATH);

			return std::string(Path::GetDirectoryName(UTF8::Narrow(moduleFileName)));
		}();

		EvilGlobalState.UpdateWorkingDirectoryFilesAsync();
		InstallAllHooks(moduleHandle);
	}

	void OnDLLDetach(HMODULE moduleHandle)
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
