#include "SprSet.h"
#include "SprTexReplace.h"
#include "Path.h"

namespace SprTexReplace
{
	std::string_view SprSet::GetFArcPath() const
	{
		return (FileContentHandleThingy != nullptr && FileContentHandleThingy->FArcPath != nullptr) ? FileContentHandleThingy->FArcPath : "";
	}

	std::string_view SprSet::GetFArcName() const
	{
		return Path::GetFileName(GetFArcPath());
	}

	std::string_view SprSet::GetSprName() const
	{
		return Path::TrimExtension(GetFArcName());
	}

	u8* SprSet::GetFileContent()
	{
		if (FileContentHandleThingy == nullptr)
			return nullptr;

		return reinterpret_cast<GetFileContent_t*>(Addresses::GetFileContent)(reinterpret_cast<const void*>(&FileContentHandleThingy));
	}
	
	const u8* SprSet::GetFileContent() const
	{
		return const_cast<SprSet*>(this)->GetFileContent();
	}
	
	size_t SprSet::GetTextureCount() const
	{
		const auto fileContent = GetFileContent();
		if (fileContent == nullptr)
			return 0;

		const auto fileHeader = reinterpret_cast<const FileHeaderData*>(fileContent);
		return fileHeader->TextureCount;
	}

	std::string_view SprSet::GetTextureName(size_t index) const
	{
		const auto fileContent = GetFileContent();
		if (fileContent == nullptr)
			return "";

		const auto fileHeader = reinterpret_cast<const FileHeaderData*>(fileContent);
		if (index >= fileHeader->TextureCount)
			return "";

		const auto textureNameOffsets = (fileContent + fileHeader->TextureNamesOffset);
		const auto textureNameOffset = reinterpret_cast<const u32*>(textureNameOffsets)[index];

		return std::string_view(reinterpret_cast<const char*>(fileContent + textureNameOffset));
	}
}
