#include "ImageFile.h"
#include "Path.h"
#include "UTF8.h"
#include "StringUtil.h"
#include <stb_image.h>

namespace SprTexReplace
{
	namespace
	{
		constexpr auto RGBAChannelCount = 4;
		constexpr auto RGBAPixelFormat = ::DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	bool ImageFile::IsValidFileName(std::string_view sourcePath)
	{
		const auto extension = Path::GetExtension(sourcePath);

		return Util::MatchesInsensitive(extension, PNGExtension)
			|| Util::MatchesInsensitive(extension, DDSExtension);
	}

	ImageFile::ImageFile(std::string_view sourcePath) : filePath(sourcePath)
	{
		assert(IsValidFileName(sourcePath));
		loadFuture = std::async(std::launch::async, [this]()
		{
			const auto extension = Path::GetExtension(filePath);

			if (Util::MatchesInsensitive(extension, PNGExtension))
			{
				stbi_set_flip_vertically_on_load(true);
				stbImage.RGBAPixels = stbi_load(filePath.c_str(), &stbImage.Width, &stbImage.Height, &stbImage.Components, RGBAChannelCount);
			}
			else if (Util::MatchesInsensitive(extension, DDSExtension))
			{
				// NOTE: Unlike pngs DDS are expected to be stored flipped in the OpenGL texture coordinate convention.
				//		 Manually flipping the texture blocks would decrease performance and'd be quite tricky in the case of BC7
				if (FAILED(::DirectX::LoadFromDDSFile(UTF8::WideArg(filePath).c_str(), ::DirectX::DDS_FLAGS_NONE, &dds.Metatdata, dds.Image)))
					dds = {};
			}
		});
	}

	ImageFile::ImageFile(ImageFile&& other) :
		loadFuture(std::move(other.loadFuture)), filePath(std::move(other.filePath)), stbImage(std::move(other.stbImage)), dds(std::move(other.dds))
	{
	}

	ImageFile::~ImageFile()
	{
		if (loadFuture.valid())
			loadFuture.get();

		if (stbImage.RGBAPixels != nullptr)
			stbi_image_free(stbImage.RGBAPixels);
	}

	ImageFile::ImageView ImageFile::GetImageView() const
	{
		if (loadFuture.valid())
			loadFuture.get();

		auto imageView = ImageView {};

		if (stbImage.RGBAPixels != nullptr)
		{
			imageView.Width = stbImage.Width;
			imageView.Height = stbImage.Height;
			imageView.Data = stbImage.RGBAPixels;
			imageView.DataSize = (imageView.Width * imageView.Height * RGBAChannelCount);
			imageView.Format = RGBAPixelFormat;
		}
		else if (const auto baseImage = dds.Image.GetImage(0, 0, 0); baseImage != nullptr)
		{
			imageView.Width = static_cast<i32>(baseImage->width);
			imageView.Height = static_cast<i32>(baseImage->height);
			imageView.Data = baseImage->pixels;
			imageView.DataSize = static_cast<u32>(baseImage->slicePitch);
			imageView.Format = baseImage->format;
		}

		return imageView;
	}

	std::string_view ImageFile::GetFilePath() const
	{
		return filePath;
	}
}
