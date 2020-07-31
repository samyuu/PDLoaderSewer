#pragma once
#include "Types.h"
#include <array>

namespace Comfy::Path
{
	constexpr char ExtensionSeparator = '.';

	constexpr char DirectorySeparator = '/', DirectorySeparatorAlt = '\\';
	constexpr const char* DirectorySeparators = "/\\";

	constexpr std::array InvalidPathCharacters = { '\"', '<', '>', '|', '\0', };
	constexpr std::array InvalidFileNameCharacters = { '\"', '<', '>', '|', ':', '*', '?', '\\', '/', '\0', };

	[[nodiscard]] constexpr std::string_view GetExtension(std::string_view filePath)
	{
		const auto lastDirectoryIndex = filePath.find_last_of(DirectorySeparators);
		const auto directoryTrimmed = (lastDirectoryIndex == std::string_view::npos) ? filePath : filePath.substr(lastDirectoryIndex);

		const auto lastIndex = directoryTrimmed.find_last_of(ExtensionSeparator);
		return (lastIndex == std::string_view::npos) ? "" : directoryTrimmed.substr(lastIndex);
	}

	[[nodiscard]] constexpr std::string_view TrimExtension(std::string_view filePath)
	{
		const auto extension = GetExtension(filePath);
		return filePath.substr(0, filePath.size() - extension.size());
	}

	[[nodiscard]] constexpr std::string_view GetFileName(std::string_view filePath, bool includeExtension = true)
	{
		const auto lastIndex = filePath.find_last_of(DirectorySeparators);
		const auto fileName = (lastIndex == std::string_view::npos) ? filePath : filePath.substr(lastIndex + 1);
		return (includeExtension) ? fileName : TrimExtension(fileName);
	}

	[[nodiscard]] constexpr std::string_view GetDirectoryName(std::string_view filePath)
	{
		const auto fileName = GetFileName(filePath);
		return fileName.empty() ? filePath : filePath.substr(0, filePath.size() - fileName.size() - 1);
	}
}

namespace Path = Comfy::Path;
