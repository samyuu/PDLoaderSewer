#include "SprTexReplace.h"
#include "Path.h"
#include "UTF8.h"
#include "Log.h"
#include <Windows.h>
#include <gl/GL.h>
#include <filesystem>

#pragma comment(lib, "opengl32.lib")

namespace SprTexReplace
{
	PluginState EvilGlobalState = {};

	PluginState::PluginState()
	{
		constexpr auto reasonablePerSprSetMaxTextureCount = 128;
		InterceptedGLTextureIDs.reserve(reasonablePerSprSetMaxTextureCount);
	}

	namespace
	{
		constexpr bool StartsWith(std::string_view string, std::string_view prefix)
		{
			return (string.size() >= prefix.size() && string.substr(0, prefix.size()) == prefix);
		}

		void TryRegisterSprTexReplacePath(const std::filesystem::path& texturePath, std::vector<SprTexInfo>& outResults)
		{
			const auto textureFileName = texturePath.filename().u8string();
			if (!SourceImage::IsValidFileName(textureFileName))
				return;

			auto& sprTexInfo = outResults.emplace_back();
			sprTexInfo.TextureName = std::string(Path::TrimExtension(textureFileName));
			sprTexInfo.ImageSourcePath = texturePath.u8string();
		}

		void TryRegisterSprSetReplaceDirectory(const std::filesystem::path& sprSetDirectory, std::vector<SprSetInfo>& outResults)
		{
			auto directoryName = sprSetDirectory.filename().u8string();
			if (!StartsWith(directoryName, "spr_") && !StartsWith(directoryName, "SPR_"))
				return;

			auto& sprSetInfo = outResults.emplace_back(SprSetInfo { std::move(directoryName) });
			for (const auto sprTexPath : std::filesystem::directory_iterator(sprSetDirectory))
				TryRegisterSprTexReplacePath(sprTexPath.path(), sprSetInfo.Textures);
		}
	}

	void PluginState::WaitForAsyncDirectoryUpdate()
	{
		if (EvilGlobalState.RegisteredReplaceInfoFuture.valid())
			EvilGlobalState.RegisteredReplaceInfo = std::move(EvilGlobalState.RegisteredReplaceInfoFuture.get());
	}

	void PluginState::UpdateWorkingDirectoryFilesAsync()
	{
		EvilGlobalState.RegisteredReplaceInfoFuture = std::async(std::launch::async, []
		{
			std::vector<SprSetInfo> results;

			const auto directoryToSerach = (EvilGlobalState.WorkingDirectory + "/" + EvilGlobalState.SprTexReplaceSubDirectory);
			const auto directoryToSerachPath = std::filesystem::path(UTF8::WideArg(directoryToSerach).c_str());

			try
			{
				for (const auto sprSetDirectory : std::filesystem::directory_iterator(directoryToSerachPath))
					TryRegisterSprSetReplaceDirectory(sprSetDirectory.path(), results);
			}
			catch (const std::exception& ex)
			{
				LOG_WRITELINE("Exception thrown: %s", ex.what());
			}

			return results;
		});
	}
}

namespace SprTexReplace
{
	namespace
	{
		const SprSetInfo* FindSprSetInfo(const SprSet& sprSet)
		{
			const auto sprSetName = sprSet.GetSprName();
			const auto found = std::find_if(
				EvilGlobalState.RegisteredReplaceInfo.begin(),
				EvilGlobalState.RegisteredReplaceInfo.end(),
				[&](const auto& info) { return (info.SetName == sprSetName); });

			return (found != EvilGlobalState.RegisteredReplaceInfo.end()) ? &(*found) : nullptr;
		}

		const SprTexInfo* FindSprTexInfo(const SprSet& sprSet, const size_t texIndex, const SprSetInfo& setInfo)
		{
			const auto textureName = sprSet.GetTextureName(texIndex);
			const auto found = std::find_if(
				setInfo.Textures.begin(),
				setInfo.Textures.end(),
				[&](const auto& info) { return (info.TextureName == textureName); });

			return (found != setInfo.Textures.end()) ? &(*found) : nullptr;
		}

		bool UpdateTempSourceImageBuffer(const SprSet& sprSet, const SprSetInfo& setInfo)
		{
			const auto textureCount = sprSet.GetTextureCount();

			EvilGlobalState.TempSourceImageBuffer.clear();
			EvilGlobalState.TempSourceImageBuffer.reserve(textureCount);

			bool anyFound = false;
			for (size_t i = 0; i < textureCount; i++)
			{
				const auto replaceInfo = FindSprTexInfo(sprSet, i, setInfo);
				if (replaceInfo == nullptr)
				{
					EvilGlobalState.TempSourceImageBuffer.emplace_back(nullptr);
				}
				else
				{
					EvilGlobalState.TempSourceImageBuffer.emplace_back(std::make_unique<SourceImage>(replaceInfo->ImageSourcePath));
					anyFound = true;
				}
			}

			return anyFound;
		}

		void DoSprSetTexReplacementsPreLoad(SprSet& sprSet)
		{
			for (size_t i = 0; i < sprSet.GetTextureCount(); i++)
			{
				if (EvilGlobalState.TempSourceImageBuffer[i] == nullptr)
					continue;

				sprSet.ForEachMipMap(i, [&](SprSet::TextureData& texture, SprSet::MipMapData& mipMap)
				{
					// NOTE: Prevent YCbCr detection messing up the overwriten textures
					if (texture.MipLevels == 2 && mipMap.Format == TextureFormat::RGTC2)
					{
						// NOTE: Change to DXT5 because it uses the same block size
						mipMap.Format = TextureFormat::DXT5;

						if (mipMap.MipIndex == 1)
							texture.MipMapCount = texture.MipLevels = 1;
					}
				});
			}
		}

		void UploadSourceImageToGLTexture(SourceImage& sourceImage, const u32 glTextureID)
		{
			// BUG: Potential bug if the source image failes to load but the YCbCr textures have already been pre-load edited
			const auto imageView = sourceImage.GetImageView();

			if (imageView.Data == nullptr || imageView.Width < 1 || imageView.Height < 1)
			{
				const auto sourcePath = sourceImage.GetFilePath();
				LOG_WRITELINE("Failed to load '%.*s'! Corrupted YCbCr textures expected.", static_cast<int>(sourcePath.size()), sourcePath.data());

				return;
			}

			::glBindTexture(GL_TEXTURE_2D, glTextureID);
			if (const auto error = ::glGetError(); error != GL_NO_ERROR) {}

			// TODO: Support DDS together with BC7 :PogU:
			::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageView.Width, imageView.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageView.Data);
			if (const auto error = ::glGetError(); error != GL_NO_ERROR) {}

			// NOTE: No need to store the previously bound texture as the original upload function should reset to 0 itself
			::glBindTexture(GL_TEXTURE_2D, 0);
			if (const auto error = ::glGetError(); error != GL_NO_ERROR) {}
		}

		void DoSprSetTexReplacementsPostLoad(const SprSet& sprSet)
		{
			const auto textureCount = sprSet.GetTextureCount();
			for (size_t i = 0; i < textureCount; i++)
			{
				auto sourceImage = EvilGlobalState.TempSourceImageBuffer[i].get();
				if (sourceImage == nullptr)
					continue;

				const auto glTextureIndex = EvilGlobalState.InterceptedGLTextureIDs.size() - textureCount + i;
				if (glTextureIndex >= EvilGlobalState.InterceptedGLTextureIDs.size())
					continue;

				const auto glTextureID = EvilGlobalState.InterceptedGLTextureIDs[glTextureIndex];
				if (glTextureID == 0)
					continue;

				UploadSourceImageToGLTexture(*sourceImage, glTextureID);
			}
		}
	}
}

namespace SprTexReplace::Hooks
{
	void __stdcall GLGenTextures(i32 count, u32* outGLTexIDs)
	{
		assert(EvilGlobalState.OriginalGLGenTextures != nullptr);
		EvilGlobalState.OriginalGLGenTextures(count, outGLTexIDs);

		if (EvilGlobalState.GenTexturesHookEnabled)
		{
			for (size_t i = 0; i < count; i++)
				EvilGlobalState.InterceptedGLTextureIDs.push_back((outGLTexIDs != nullptr) ? outGLTexIDs[i] : 0);
		}
	}

	int __cdecl SprSetParseTexSet(SprSet* thisSprSet)
	{
		if (thisSprSet == nullptr)
			return EvilGlobalState.OriginalParseSprSetTexSet(thisSprSet);

		EvilGlobalState.WaitForAsyncDirectoryUpdate();

		const auto setReplaceInfo = FindSprSetInfo(*thisSprSet);
		if (setReplaceInfo == nullptr || setReplaceInfo->Textures.empty())
			return EvilGlobalState.OriginalParseSprSetTexSet(thisSprSet);

		const auto wasAnyReplacementFound = UpdateTempSourceImageBuffer(*thisSprSet, *setReplaceInfo);
		if (!wasAnyReplacementFound)
			return EvilGlobalState.OriginalParseSprSetTexSet(thisSprSet);

		DoSprSetTexReplacementsPreLoad(*thisSprSet);

		EvilGlobalState.InterceptedGLTextureIDs.clear();
		EvilGlobalState.GenTexturesHookEnabled = true;
		const auto originalReturnValue = EvilGlobalState.OriginalParseSprSetTexSet(thisSprSet);
		EvilGlobalState.GenTexturesHookEnabled = false;

		DoSprSetTexReplacementsPostLoad(*thisSprSet);

		return originalReturnValue;
	}
}
