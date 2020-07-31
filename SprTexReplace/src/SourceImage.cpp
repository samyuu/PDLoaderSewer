#include "SourceImage.h"
#include "Path.h"
#include <stb_image.h>

namespace SprTexReplace
{
	bool SourceImage::IsValidFileName(std::string_view sourcePath)
	{
		const auto extension = Path::GetExtension(sourcePath);
		return (extension == ".png" || extension == ".dds");
	}

	SourceImage::SourceImage(std::string_view sourcePath) : filePath(sourcePath)
	{
		const auto extension = Path::GetExtension(sourcePath);
		if (extension == ".png")
		{
			loadFuture = std::async(std::launch::async, [this]()
			{
				stbi_set_flip_vertically_on_load(true);
				stbImage.RGBAPixels = stbi_load(filePath.c_str(), &stbImage.Width, &stbImage.Height, &stbImage.Components, 4);
			});
		}
		else if (extension == ".dds")
		{
			// TODO: DDS support
		}
	}

	SourceImage::~SourceImage()
	{
		if (loadFuture.valid())
			loadFuture.get();

		if (stbImage.RGBAPixels != nullptr)
		{
			stbi_image_free(stbImage.RGBAPixels);
		}
		else
		{
			// TODO: DDS support
		}
	}

	SourceImage::ImageView SourceImage::GetImageView()
	{
		if (loadFuture.valid())
			loadFuture.get();

		auto imageData = ImageView {};

		if (stbImage.RGBAPixels != nullptr)
		{
			imageData.Width = stbImage.Width;
			imageData.Height = stbImage.Height;
			imageData.Data = stbImage.RGBAPixels;
			imageData.Format = {};
		}
		else
		{
			// TODO: DDS support
		}

		return imageData;
	}

	std::string_view SourceImage::GetFilePath() const
	{
		return filePath;
	}
}
