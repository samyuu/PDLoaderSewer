#pragma once
#include "Types.h"
#include <Windows.h>
#include <detours.h>

namespace SprTexReplace
{
	template <typename Func>
	void MakeMemoryWritable(const u64 address, const size_t byteSize, Func onWritable)
	{
		DWORD oldProtect, newProtect;
		if (::VirtualProtect(reinterpret_cast<LPVOID>(address), static_cast<SIZE_T>(byteSize), PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			onWritable();
			::VirtualProtect(reinterpret_cast<LPVOID>(address), static_cast<SIZE_T>(byteSize), oldProtect, &newProtect);
		}
	}

	inline void WriteToProtectedMemory(const u64 address, const size_t dataSize, const void* dataToCopy)
	{
		MakeMemoryWritable(address, dataSize, [&]
		{
			std::memcpy(reinterpret_cast<void*>(address), dataToCopy, dataSize);
		});
	}

	struct FunctionHooker
	{
		FunctionHooker(const HMODULE moduleHandle)
		{
			::DisableThreadLibraryCalls(moduleHandle);
			::DetourTransactionBegin();
			::DetourUpdateThread(::GetCurrentThread());
		}

		~FunctionHooker()
		{
			::DetourTransactionCommit();
		}

		template <typename Func>
		void Hook(const u64 address, Func*& outOriginalFunc, Func& onHookFunc)
		{
			outOriginalFunc = reinterpret_cast<Func*>(address);
			::DetourAttach(reinterpret_cast<PVOID*>(&outOriginalFunc), &onHookFunc);
		}
	};
}
