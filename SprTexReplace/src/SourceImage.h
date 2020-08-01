#pragma once
#include "Types.h"
#include <string>
#include <future>
#include <DirectXTex.h>

namespace SprTexReplace
{
	class SourceImage : NonCopyable
	{
	public:
		static bool IsValidFileName(std::string_view sourcePath);

	public:
		SourceImage(std::string_view sourcePath);
		~SourceImage();

	public:
		struct ImageView
		{
			i32 Width;
			i32 Height;
			const void* Data;
			u32 DataSize;
			::DXGI_FORMAT Format;
		};

		ImageView GetImageView();
		std::string_view GetFilePath() const;

	private:
		std::future<void> loadFuture;
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
