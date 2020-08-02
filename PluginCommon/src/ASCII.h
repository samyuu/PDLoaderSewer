#pragma once
#include "Types.h"
#include <string>
#include <array>

namespace Comfy::ASCII
{
	constexpr std::string_view WhiteSpaceCharactersString = " \t\r\n";
	constexpr std::array<char, 4> WhiteSpaceCharactersArray = { ' ', '\t', '\r', '\n' };

	constexpr char CaseDifference = ('A' - 'a');

	constexpr char LowerCaseMin = 'a', LowerCaseMax = 'z';
	constexpr char UpperCaseMin = 'A', UpperCaseMax = 'Z';

	[[nodiscard]] constexpr bool IsWhiteSpace(char character)
	{
		return
			character == WhiteSpaceCharactersArray[0] ||
			character == WhiteSpaceCharactersArray[1] ||
			character == WhiteSpaceCharactersArray[2] ||
			character == WhiteSpaceCharactersArray[3];
	}

	[[nodiscard]] constexpr bool IsLowerCase(char character)
	{
		return (character >= LowerCaseMin && character <= LowerCaseMax);
	}

	[[nodiscard]] constexpr bool IsUpperCase(char character)
	{
		return (character >= UpperCaseMin && character <= UpperCaseMax);
	}

	[[nodiscard]] constexpr char ToLowerCase(char character)
	{
		return IsUpperCase(character) ? (character - CaseDifference) : character;
	}

	[[nodiscard]] constexpr char ToUpperCase(char character)
	{
		return IsLowerCase(character) ? (character + CaseDifference) : character;
	}
}

namespace ASCII = Comfy::ASCII;
