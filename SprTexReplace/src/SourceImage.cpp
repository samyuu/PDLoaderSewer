#include "SourceImage.h"
#include "Path.h"
#include "UTF8.h"
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
			loadFuture = std::async(std::launch::async, [this]()
			{
				// NOTE: Unlike pngs DDS are expected to be stored flipped in the OpenGL texture coordinate convention.
				//		 Manually flipping the texture blocks would decrease performance and'd be quite tricky in the case of BC7
				if (FAILED(::DirectX::LoadFromDDSFile(UTF8::WideArg(filePath).c_str(), ::DirectX::DDS_FLAGS_NONE, &dds.Metatdata, dds.Image)))
					dds = {};
			});
		}
	}

	SourceImage::~SourceImage()
	{
		if (loadFuture.valid())
			loadFuture.get();

		if (stbImage.RGBAPixels != nullptr)
			stbi_image_free(stbImage.RGBAPixels);
	}

	SourceImage::ImageView SourceImage::GetImageView()
	{
		if (loadFuture.valid())
			loadFuture.get();

		auto imageView = ImageView {};

		if (stbImage.RGBAPixels != nullptr)
		{
			imageView.Width = stbImage.Width;
			imageView.Height = stbImage.Height;
			imageView.Data = stbImage.RGBAPixels;
			imageView.DataSize = (imageView.Width * imageView.Height * 4);
			imageView.Format = ::DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (auto baseImage = dds.Image.GetImage(0, 0, 0); baseImage != nullptr)
		{
			imageView.Width = static_cast<i32>(baseImage->width);
			imageView.Height = static_cast<i32>(baseImage->height);
			imageView.Data = baseImage->pixels;
			imageView.DataSize = static_cast<u32>(baseImage->slicePitch);
			imageView.Format = baseImage->format;
		}

		return imageView;
	}

	std::string_view SourceImage::GetFilePath() const
	{
		return filePath;
	}
}
