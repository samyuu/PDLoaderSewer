#pragma once
#include "Types.h"
#include "Image/ImageFile.h"

namespace SprTexReplace
{
	constexpr u32 InvalidGLTextureID = 0;

	void GLBindUploadImageViewTextureData(const ImageFile& imageFile, const ImageFile::ImageView& imageView, const u32 textureID);
}
