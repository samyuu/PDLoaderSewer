#pragma once
#include "Types.h"
#include "ASCII.h"
#include <string>

namespace Comfy::Util
{
	[[nodiscard]] constexpr bool Matches(std::string_view stringA, std::string_view stringB)
	{
		return (stringA == stringB);
	}

	// NOTE: The primary purpose of this is to check for file extensions and maybe DB resource names
	//		 so no need to add extra complexcity for UTF-8 and the mess that different languages are
	[[nodiscard]] constexpr bool MatchesInsensitive(std::string_view stringA, std::string_view stringB)
	{
		if (stringA.size() != stringB.size())
			return false;

		for (size_t i = 0; i < stringA.size(); i++)
		{
			if (ASCII::ToLowerCase(stringA[i]) != ASCII::ToLowerCase(stringB[i]))
				return false;
		}

		return true;
	}

	[[nodiscard]] constexpr bool StartsWith(std::string_view string, std::string_view prefix)
	{
		return (string.size() >= prefix.size() && Matches(string.substr(0, prefix.size()), prefix));
	}

	[[nodiscard]] constexpr bool StartsWithInsensitive(std::string_view string, std::string_view prefix)
	{
		return (string.size() >= prefix.size() && MatchesInsensitive(string.substr(0, prefix.size()), prefix));
	}
}

namespace Util = Comfy::Util;
