#pragma once
#include "Types.h"
#include <string>
#include <future>
#include <DirectXTex.h>

namespace SprTexReplace
{
	class ImageFile : NonCopyable
	{
	public:
		static constexpr std::string_view PNGExtension = ".png";
		static constexpr std::string_view DDSExtension = ".dds";

		static bool IsValidFileName(std::string_view sourcePath);

	public:
		ImageFile(std::string_view sourcePath);
		ImageFile(ImageFile&& other);
		~ImageFile();

	public:
		struct ImageView
		{
			i32 Width;
			i32 Height;
			const void* Data;
			u32 DataSize;
			::DXGI_FORMAT Format;
		};

		ImageView GetImageView() const;
		std::string_view GetFilePath() const;

	private:
		mutable std::future<void> loadFuture;
		std::string filePath;

		struct STBImageData
		{
			u8* RGBAPixels;
			i32 Width, Height, Components;
		} stbImage = {};

		struct DDSImageData
		{
			::DirectX::ScratchImage Image;
			::DirectX::TexMetadata Metatdata;
		} dds = {};
	};
}
