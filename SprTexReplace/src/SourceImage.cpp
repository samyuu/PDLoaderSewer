#include "SourceImage.h"
#include "Path.h"
#include "UTF8.h"
#include "StringUtil.h"
#include <stb_image.h>

namespace SprTexReplace
{
	bool SourceImage::IsValidFileName(std::string_view sourcePath)
	{
		const auto extension = Path::GetExtension(sourcePath);
		return Util::MatchesInsensitive(extension, PNGExtension) || Util::MatchesInsensitive(extension, DDSExtension);
	}

	SourceImage::SourceImage(std::string_view sourcePath) : filePath(sourcePath)
	{
		const auto extension = Path::GetExtension(sourcePath);
		if (Util::MatchesInsensitive(extension, PNGExtension))
		{
			loadFuture = std::async(std::launch::async, [this]()
			{
				stbi_set_flip_vertically_on_load(true);
				stbImage.RGBAPixels = stbi_load(filePath.c_str(), &stbImage.Width, &stbImage.Height, &stbImage.Components, 4);
			});
		}
		else if (Util::MatchesInsensitive(extension, DDSExtension))
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
