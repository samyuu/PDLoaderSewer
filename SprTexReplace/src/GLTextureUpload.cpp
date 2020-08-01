#include "GLTextureUpload.h"
#include "SprTexReplace.h"
#include "Log.h"
#include <Windows.h>
#include <gl/GL.h>

// TODO: Instead of linking to the opengl lib all functions could be directly read from the already imported in-game function pointers
//		 like is already currently the case for glCompressedTexImage2D().
#pragma comment(lib, "opengl32.lib")

// NOTE: https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_compression_s3tc.txt
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT						0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT					0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT					0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT					0x83F3

// NOTE: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_rgtc.txt
#define GL_COMPRESSED_RED_RGTC1								0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1						0x8DBC
#define GL_COMPRESSED_RG_RGTC2								0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2						0x8DBE

// NOTE: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB					0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB				0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB				0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB			0x8E8F

// NOTE: https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h
#define GL_RG												0x8227

namespace SprTexReplace
{
	struct GLTextureFormat
	{
		GLint InternalFormat;
		GLint Format;
		GLenum DataType;
	};

	constexpr GLTextureFormat DXGIFormatToGLFormat(::DXGI_FORMAT format)
	{
		switch (format)
		{
		case ::DXGI_FORMAT_R8G8B8A8_UNORM:	return { GL_RGBA8,							GL_RGBA,			GL_UNSIGNED_BYTE };
		case ::DXGI_FORMAT_BC1_UNORM:		return { GL_COMPRESSED_RGB_S3TC_DXT1_EXT,	GL_RGB,				GL_UNSIGNED_BYTE };
		case ::DXGI_FORMAT_BC2_UNORM_SRGB:	return { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,	GL_RGBA,			GL_UNSIGNED_BYTE };
		case ::DXGI_FORMAT_BC3_UNORM:		return { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,	GL_RGBA,			GL_UNSIGNED_BYTE };
		case ::DXGI_FORMAT_BC4_UNORM:		return { GL_COMPRESSED_RED_RGTC1,			GL_RED,				GL_UNSIGNED_BYTE };
		case ::DXGI_FORMAT_BC5_UNORM:		return { GL_COMPRESSED_RG_RGTC2,			GL_RG,				GL_UNSIGNED_BYTE };
		case ::DXGI_FORMAT_BC7_UNORM:		return { GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, GL_RGBA,			GL_UNSIGNED_BYTE };
		default:							return { GL_INVALID_VALUE,					GL_INVALID_VALUE,	GL_INVALID_ENUM };
		}
	}

	void GLBindUploadImageViewTextureData(const SourceImage& sourceImage, const SourceImage::ImageView& imageView, const u32 glTextureID)
	{
		assert(imageView.Data != nullptr);

		// NOTE: Since this only applies to sprite textures we only ever care about the first mipmap
		constexpr auto baseMipLevel = 0, border = 0;

		auto[internalFormat, format, dataType] = DXGIFormatToGLFormat(imageView.Format);
		if (internalFormat == GL_INVALID_VALUE || format == GL_INVALID_VALUE)
		{
			LOG_WRITELINE("Unsupported DDS pixel format: 0x%X", imageView.Format);
			return;
		}

		::glBindTexture(GL_TEXTURE_2D, glTextureID);
		if (const auto error = ::glGetError(); error != GL_NO_ERROR)
			LOG_WRITELINE("Failed to bind texture ID. GL Error: 0x%X", error);

		if (!::DirectX::IsCompressed(imageView.Format))
		{
			::glTexImage2D(GL_TEXTURE_2D, baseMipLevel, internalFormat, imageView.Width, imageView.Height, border, format, dataType, imageView.Data);
			if (const auto error = ::glGetError(); error != GL_NO_ERROR)
				LOG_WRITELINE("Failed to upload uncompressed image data. GL Error: 0x%X", error);
		}
		else
		{
			// HACK: So what about glPixelStorei() ..? Yeah, I don't know either. I guess whatever was set by the original TexSet upload function should be fine.

			const auto glCompressedTexImage2D = *reinterpret_cast<GLCompressedTexImage2D_t**>(Addresses::GLCompressedTexImage2DPtr);
			glCompressedTexImage2D(GL_TEXTURE_2D, baseMipLevel, internalFormat, imageView.Width, imageView.Height, border, imageView.DataSize, imageView.Data);

			if (const auto error = ::glGetError(); error != GL_NO_ERROR)
				LOG_WRITELINE("Failed to upload compressed image data. GL Error: 0x%X", error);
		}

		// NOTE: No need to store the previously bound texture as the original upload function should reset to 0 itself
		::glBindTexture(GL_TEXTURE_2D, 0);
		if (const auto error = ::glGetError(); error != GL_NO_ERROR)
			LOG_WRITELINE("Resetting the bound texture should never fail - were previous errors not cleared? GL Error: 0x%X", error);
	}
}
