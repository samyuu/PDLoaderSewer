#pragma once
#include "Types.h"
#include <string>
#include <array>

// NOTE: Following the "UTF-8 Everywhere" guidelines
namespace Comfy::UTF8
{
	// NOTE: Convert UTF-16 to UTF-8
	[[nodiscard]] std::string Narrow(std::wstring_view);

	// NOTE: Convert UTF-8 to UTF-16
	[[nodiscard]] std::wstring Widen(std::string_view);

	// NOTE: To avoid needless heap allocations for temporary wchar_t C-API function arguments
	//		 Example: DummyU16FuncW(UTF8::WideArg(stringU8).c_str(), ...)
	class WideArg : NonCopyable
	{
	public:
		WideArg(std::string_view);
		[[nodiscard]] const wchar_t* c_str() const;

	private:
		std::unique_ptr<wchar_t[]> heapBuffer;
		std::array<wchar_t, 260> stackBuffer;
		int convertedLength;
	};
}

namespace UTF8 = Comfy::UTF8;
