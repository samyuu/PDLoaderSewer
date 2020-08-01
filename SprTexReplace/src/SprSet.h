#pragma once
#include "Types.h"
#include "Addresses.h"
#include <string>

namespace SprTexReplace
{
	enum class TextureFormat : u32
	{
		A8 = 0,
		RGB8 = 1,
		RGBA8 = 2,
		RGB5 = 3,
		RGB5_A1 = 4,
		RGBA4 = 5,
		DXT1 = 6,
		DXT1a = 7,
		DXT3 = 8,
		DXT5 = 9,
		RGTC1 = 10,
		RGTC2 = 11,
		L8 = 12,
		L8A8 = 13,
	};

	struct SprSet : NonCopyable
	{
		SprSet() = delete;
		~SprSet() = delete;

		struct FileHandleThingy : NonCopyable
		{
			FileHandleThingy() = delete;
			~FileHandleThingy() = delete;

			u32 field_0;
			u32 field_4;
			const void* field_8;
			u32 field_10;
			u32 field_14;
			const char* FArcPath;
#if 0
			u32 field_1C;
			u32 field_20;
			u32 field_24;
			u32 field_28;
			const char* FileName;
#endif
		};

		const void* vftable;
		u32 field_8;
		u32 SprDBIndexThingy;
		u32 field_10;
		u32 FileHandle;
		u32 field_18;
		u32 field_1C;
		u32 field_20;
		u32 field_24;
		u32 field_28;
		u32 field_2C;
		FileHandleThingy* FileContentHandleThingy;
		u8 HasBeenParsed;
		u8 field_39;
		u8 field_3A;
		u8 field_3B;
		u8 field_3C;
		u8 field_40;
		u8 field_44;
		u8 field_48;
		u8 field_4C;
#if 0
		const char* FileName;
#endif

		// NOTE: ./rom/2d/spr_{file_name}.farc
		std::string_view GetFArcPath() const;

		// NOTE: spr_{file_name}.farc
		std::string_view GetFArcName() const;

		// NOTE: spr_{file_name}
		std::string_view GetSprName() const;

		u8* GetFileContent();
		const u8* GetFileContent() const;

		size_t GetTextureCount() const;
		std::string_view GetTextureName(size_t index) const;

		// NOTE: These represent the data layout of the in memory file content and should therefore only ever be pointer casted
		struct FileHeaderData : NonCopyable
		{
			FileHeaderData() = delete;
			~FileHeaderData() = delete;

			u32 Flags;
			u32 TexSetOffset;
			u32 TextureCount;
			u32 SpriteCount;
			u32 SpritesOffset;
			u32 TextureNamesOffset;
			u32 SpriteNamesOffset;
			u32 SpriteExtraDataOffset;
		};

		struct TextureData : NonCopyable
		{
			TextureData() = delete;
			~TextureData() = delete;

			u32 Signature;
			u32 MipMapCount;
			u8 MipLevels;
			u8 ArraySize;
			u8 Depth;
			u8 Dimension;
		};

		// NOTE: "nonstandard extension used: zero-sized array in struct/union"
#pragma warning(push)
#pragma warning(disable : 4200) 
		struct MipMapData : NonCopyable
		{
			MipMapData() = delete;
			~MipMapData() = delete;

			u32 Signature;
			u32 Width;
			u32 Height;
			TextureFormat Format;
			u32 MipIndex;
			u32 ArrayIndex;
			u32 DataSize;
			u8 Data[/*DataSize*/];
		};
#pragma warning(pop)

		template <typename Func>
		void ForEachMipMap(size_t textureIndex, Func perMipFunc)
		{
			auto fileContent = GetFileContent();
			if (fileContent == nullptr)
				return;

			auto fileHeader = reinterpret_cast<FileHeaderData*>(fileContent);
			if (textureIndex >= fileHeader->TextureCount)
				return;

			auto texSetContent = (fileContent + fileHeader->TexSetOffset);

			auto texOffsets = reinterpret_cast<u32*>(texSetContent + (sizeof(u32) * 3));
			auto texOffset = texOffsets[textureIndex];

			auto texContent = (texSetContent + texOffset);
			auto texPtr = reinterpret_cast<TextureData*>(texContent);

			auto mipOffsets = reinterpret_cast<u32*>(texContent + (sizeof(u32) * 3));
			for (u8 mipIndex = 0; mipIndex < texPtr->MipLevels; mipIndex++)
			{
				auto mipContent = (texContent + mipOffsets[mipIndex]);
				auto mipPtr = reinterpret_cast<MipMapData*>(mipContent);

				perMipFunc(*texPtr, *mipPtr);
			}
		}
	};
}
