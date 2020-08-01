#pragma once
#include "Types.h"
#include "SourceImage.h"

namespace SprTexReplace
{
	void GLBindUploadImageViewTextureData(const SourceImage& sourceImage, const SourceImage::ImageView& imageView, const u32 textureID);
}
